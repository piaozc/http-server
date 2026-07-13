#include "Connection.h"

#include "../http/HttpParser.h"
#include "../http/HttpResponse.h"

#include <cerrno>
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <utility>
#include <unistd.h>

namespace {

std::string queryValue(const HttpRequest& request, const std::string& key, const std::string& fallback = "") {
    auto it = request.query_params.find(key);
    return it == request.query_params.end() ? fallback : it->second;
}

std::size_t fileSize(int fd) {
    struct stat statbuf {};
    if (fstat(fd, &statbuf) == -1 || statbuf.st_size < 0) {
        return 0;
    }
    return static_cast<std::size_t>(statbuf.st_size);
}

} // namespace

Connection::Connection(int fd, BusinessClient* business, EventCallback on_event)
    : client_fd(fd), business_client(business), event_callback(std::move(on_event)) {}

Connection::~Connection() {
    if (upload_fd != -1) {
        close(upload_fd);
    }
    if (download_fd != -1) {
        close(download_fd);
    }
}

int Connection::fd() const {
    return client_fd;
}

bool Connection::wantWrite() const {
    return !write_buffer.empty() || state == State::StreamingFile;
}

bool Connection::closed() const {
    return state == State::Closed;
}

void Connection::handleReadable() {
    char buffer[kIoBufferSize];
    while (true) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            if (state == State::Uploading) {
                writeUploadBytes(buffer, static_cast<std::size_t>(n));
            } else {
                read_buffer.append(buffer, static_cast<std::size_t>(n));
                consumeReadBuffer();
            }
            if (state == State::Closed) {
                return;
            }
        } else if (n == 0) {
            closeNow();
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            closeNow();
            return;
        }
    }
    updateEvents();
}

void Connection::handleWritable() {
    while (!write_buffer.empty()) {
        ssize_t n = send(client_fd, write_buffer.data(), write_buffer.size(), 0);
        if (n > 0) {
            write_buffer.erase(0, static_cast<std::size_t>(n));
        } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            updateEvents();
            return;
        } else {
            closeNow();
            return;
        }
    }

    if (state == State::StreamingFile) {
        char buffer[kIoBufferSize];
        while (write_buffer.empty()) {
            ssize_t n = read(download_fd, buffer, sizeof(buffer));
            if (n > 0) {
                transferred_bytes += static_cast<std::size_t>(n);
                write_buffer.append(buffer, static_cast<std::size_t>(n));
                ssize_t sent = send(client_fd, write_buffer.data(), write_buffer.size(), 0);
                if (sent > 0) {
                    write_buffer.erase(0, static_cast<std::size_t>(sent));
                    if (!write_buffer.empty()) {
                        break;
                    }
                } else if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    break;
                } else {
                    finishDownload(false, "send failed");
                    closeNow();
                    return;
                }
            } else if (n == 0) {
                finishDownload(true, "download done");
                close_after_write = true;
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                finishDownload(false, "file read failed");
                queueResponse(HttpResponse::text(500, "Internal Server Error", "file read failed\n"), true);
                break;
            }
        }
    }

    if (write_buffer.empty() && close_after_write) {
        closeNow();
        return;
    }
    updateEvents();
}

void Connection::consumeReadBuffer() {
    while (state == State::ReadingHeaders) {
        std::size_t header_end = 0;
        ParseResult result = HttpParser::parseHeaders(read_buffer, &request, &header_end);
        if (result == ParseResult::NeedMore) {
            return;
        }
        if (result == ParseResult::BadRequest) {
            queueResponse(HttpResponse::text(400, "Bad Request", "bad request\n"), true);
            return;
        }

        read_buffer.erase(0, header_end);
        processRequest();

        if (state == State::Uploading && !read_buffer.empty()) {
            std::string pending;
            pending.swap(read_buffer);
            writeUploadBytes(pending.data(), pending.size());
        }
    }
}

void Connection::processRequest() {
    transfer_request.request_id = std::to_string(client_fd);
    transfer_request.user = queryValue(request, "user", "default");
    transfer_request.target_user = queryValue(request, "target_user", transfer_request.user);
    transfer_request.path = queryValue(request, "path", "/file.bin");
    transfer_request.filename = queryValue(request, "filename", "file.bin");
    transfer_request.size = request.content_length;

    if (request.method == "PUT" && request.path == "/upload") {
        transfer_request.action = "upload";
        beginUpload();
        return;
    }

    if (request.method == "GET" && request.path == "/download") {
        transfer_request.action = "download";
        beginDownload();
        return;
    }

    queueResponse(HttpResponse::text(404, "Not Found", "not found\n"), true);
}

void Connection::beginUpload() {
    TransferDecision decision = business_client->prepareUpload(transfer_request);
    if (!decision.allow) {
        queueResponse(HttpResponse::text(403, "Forbidden", decision.reason + "\n"), true);
        return;
    }

    upload_fd = open(decision.storage_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (upload_fd == -1) {
        queueResponse(HttpResponse::text(500, "Internal Server Error", "open upload file failed\n"), true);
        return;
    }

    upload_remaining = request.content_length;
    transferred_bytes = 0;
    state = State::Uploading;

    if (upload_remaining == 0) {
        finishUpload(true, "upload done");
        queueResponse(HttpResponse::text(200, "OK", "upload ok\n"), true);
    }
}

void Connection::beginDownload() {
    TransferDecision decision = business_client->prepareDownload(transfer_request);
    if (!decision.allow) {
        queueResponse(HttpResponse::text(403, "Forbidden", decision.reason + "\n"), true);
        return;
    }

    download_fd = open(decision.storage_path.c_str(), O_RDONLY);
    if (download_fd == -1) {
        queueResponse(HttpResponse::text(404, "Not Found", "file not found\n"), true);
        return;
    }

    std::size_t size = fileSize(download_fd);
    transferred_bytes = 0;
    queueResponse(HttpResponse::downloadHeader(decision.filename, decision.content_type, size), false);
    state = State::StreamingFile;
}

void Connection::writeUploadBytes(const char* data, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size && upload_remaining > 0) {
        std::size_t chunk = std::min(size - offset, upload_remaining);
        ssize_t n = write(upload_fd, data + offset, chunk);
        if (n > 0) {
            offset += static_cast<std::size_t>(n);
            upload_remaining -= static_cast<std::size_t>(n);
            transferred_bytes += static_cast<std::size_t>(n);
        } else if (n == -1 && errno == EINTR) {
            continue;
        } else {
            finishUpload(false, "file write failed");
            queueResponse(HttpResponse::text(500, "Internal Server Error", "file write failed\n"), true);
            return;
        }
    }

    if (upload_remaining == 0) {
        finishUpload(true, "upload done");
        queueResponse(HttpResponse::text(200, "OK", "upload ok\n"), true);
    }
}

void Connection::finishUpload(bool success, const std::string& message) {
    if (upload_fd != -1) {
        close(upload_fd);
        upload_fd = -1;
    }

    TransferResult result;
    result.request_id = transfer_request.request_id;
    result.action = "upload";
    result.success = success;
    result.bytes = transferred_bytes;
    result.message = message;
    business_client->reportResult(result);
    state = State::Sending;
}

void Connection::finishDownload(bool success, const std::string& message) {
    if (download_fd != -1) {
        close(download_fd);
        download_fd = -1;
    }

    TransferResult result;
    result.request_id = transfer_request.request_id;
    result.action = "download";
    result.success = success;
    result.bytes = transferred_bytes;
    result.message = message;
    business_client->reportResult(result);
    state = State::Sending;
}

void Connection::queueResponse(const std::string& response, bool close_connection) {
    write_buffer.append(response);
    close_after_write = close_connection;
    if (state != State::StreamingFile) {
        state = State::Sending;
    }
    updateEvents();
}

void Connection::closeNow() {
    if (state == State::Closed) {
        return;
    }
    state = State::Closed;
}

void Connection::updateEvents() {
    if (state != State::Closed) {
        event_callback(client_fd, wantWrite());
    }
}
