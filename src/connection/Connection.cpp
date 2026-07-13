#include "Connection.h"

#include "../http/HttpParser.h"
#include "../http/HttpResponse.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

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

Connection::Connection(int fd, ThreadPool* pool, BusinessClient* business, EventCallback on_event, LoopCallback on_loop)
    : client_fd(fd),
      io_pool(pool),
      business_client(business),
      event_callback(std::move(on_event)),
      loop_callback(std::move(on_loop)) {}

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

bool Connection::wantRead() const {
    if (state == State::ReadingHeaders) {
        return true;
    }
    if (state == State::Uploading) {
        return pending_upload_bytes < kMaxPendingUploadBytes;
    }
    return false;
}

bool Connection::wantWrite() const {
    return !write_buffer.empty();
}

bool Connection::closed() const {
    return state == State::Closed;
}

void Connection::handleReadable() {
    char buffer[kIoBufferSize];
    while (wantRead()) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            read_buffer.append(buffer, static_cast<std::size_t>(n));
            if (state == State::ReadingHeaders) {
                consumeReadBuffer();
            }
            if (state == State::Uploading) {
                drainUploadBuffer();
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
        scheduleDownloadRead();
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
    state = State::PreparingUpload;
    updateEvents();

    std::weak_ptr<Connection> weak_self = shared_from_this();
    TransferRequest request_copy = transfer_request;
    io_pool->submit([weak_self, request_copy, business = business_client, post = loop_callback]() {
        TransferDecision decision = business->prepareUpload(request_copy);
        int fd = -1;
        std::string error;
        if (decision.allow) {
            fd = open(decision.storage_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                error = "open upload file failed";
            }
        }

        post([weak_self, decision, fd, error]() {
            auto self = weak_self.lock();
            if (!self) {
                if (fd != -1) {
                    close(fd);
                }
                return;
            }
            if (self->state == State::Closed) {
                if (fd != -1) {
                    close(fd);
                }
                return;
            }
            if (!decision.allow) {
                self->queueResponse(HttpResponse::text(403, "Forbidden", decision.reason + "\n"), true);
                return;
            }
            if (fd == -1) {
                self->queueResponse(HttpResponse::text(500, "Internal Server Error", error + "\n"), true);
                return;
            }

            self->upload_fd = fd;
            self->upload_remaining = self->request.content_length;
            self->upload_offset = 0;
            self->pending_upload_bytes = 0;
            self->pending_upload_tasks = 0;
            self->transferred_bytes = 0;
            self->state = State::Uploading;
            self->drainUploadBuffer();
            self->updateEvents();
        });
    });
}

void Connection::beginDownload() {
    state = State::PreparingDownload;
    updateEvents();

    std::weak_ptr<Connection> weak_self = shared_from_this();
    TransferRequest request_copy = transfer_request;
    io_pool->submit([weak_self, request_copy, business = business_client, post = loop_callback]() {
        TransferDecision decision = business->prepareDownload(request_copy);
        int fd = -1;
        std::size_t size = 0;
        std::string error;
        if (decision.allow) {
            fd = open(decision.storage_path.c_str(), O_RDONLY);
            if (fd == -1) {
                error = "file not found";
            } else {
                size = fileSize(fd);
            }
        }

        post([weak_self, decision, fd, size, error]() {
            auto self = weak_self.lock();
            if (!self) {
                if (fd != -1) {
                    close(fd);
                }
                return;
            }
            if (self->state == State::Closed) {
                if (fd != -1) {
                    close(fd);
                }
                return;
            }
            if (!decision.allow) {
                self->queueResponse(HttpResponse::text(403, "Forbidden", decision.reason + "\n"), true);
                return;
            }
            if (fd == -1) {
                self->queueResponse(HttpResponse::text(404, "Not Found", error + "\n"), true);
                return;
            }

            self->download_fd = fd;
            self->download_size = size;
            self->download_offset = 0;
            self->download_read_pending = false;
            self->transferred_bytes = 0;
            self->state = State::StreamingFile;
            self->queueResponse(HttpResponse::downloadHeader(decision.filename, decision.content_type, size), false);
            self->scheduleDownloadRead();
            self->updateEvents();
        });
    });
}

void Connection::drainUploadBuffer() {
    while (state == State::Uploading && upload_remaining > 0 && !read_buffer.empty() && pending_upload_bytes < kMaxPendingUploadBytes) {
        std::size_t chunk_size = std::min(read_buffer.size(), upload_remaining);
        chunk_size = std::min(chunk_size, kIoBufferSize);
        std::string chunk = read_buffer.substr(0, chunk_size);
        read_buffer.erase(0, chunk_size);

        std::size_t offset = upload_offset;
        upload_offset += chunk_size;
        upload_remaining -= chunk_size;
        pending_upload_bytes += chunk_size;
        ++pending_upload_tasks;
        scheduleUploadWrite(std::move(chunk), offset);
    }

    if (state == State::Uploading && upload_remaining == 0 && pending_upload_tasks == 0) {
        finishUpload(true, "upload done");
        queueResponse(HttpResponse::text(200, "OK", "upload ok\n"), true);
    }
}

void Connection::scheduleUploadWrite(std::string data, std::size_t offset) {
    int task_fd = dup(upload_fd);
    if (task_fd == -1) {
        onUploadWriteDone(data.size(), false, "dup upload fd failed");
        return;
    }

    std::weak_ptr<Connection> weak_self = shared_from_this();
    std::size_t bytes = data.size();
    io_pool->submit([weak_self, task_fd, data = std::move(data), offset, bytes, post = loop_callback]() {
        std::size_t written = 0;
        bool success = true;
        std::string message = "write ok";

        while (written < data.size()) {
            ssize_t n = pwrite(task_fd, data.data() + written, data.size() - written, static_cast<off_t>(offset + written));
            if (n > 0) {
                written += static_cast<std::size_t>(n);
            } else if (n == -1 && errno == EINTR) {
                continue;
            } else {
                success = false;
                message = "file write failed";
                break;
            }
        }
        close(task_fd);

        post([weak_self, bytes, success, message]() {
            auto self = weak_self.lock();
            if (self && self->state != State::Closed) {
                self->onUploadWriteDone(bytes, success, message);
            }
        });
    });
}

void Connection::onUploadWriteDone(std::size_t bytes, bool success, const std::string& message) {
    if (pending_upload_bytes >= bytes) {
        pending_upload_bytes -= bytes;
    } else {
        pending_upload_bytes = 0;
    }
    if (pending_upload_tasks > 0) {
        --pending_upload_tasks;
    }

    if (!success) {
        finishUpload(false, message);
        queueResponse(HttpResponse::text(500, "Internal Server Error", message + "\n"), true);
        return;
    }

    transferred_bytes += bytes;
    drainUploadBuffer();
    updateEvents();
}

void Connection::scheduleDownloadRead() {
    if (state != State::StreamingFile || download_read_pending || write_buffer.size() >= kMaxWriteBufferBytes) {
        return;
    }
    if (download_offset >= download_size) {
        finishDownload(true, "download done");
        close_after_write = true;
        updateEvents();
        return;
    }

    int task_fd = dup(download_fd);
    if (task_fd == -1) {
        finishDownload(false, "dup download fd failed");
        queueResponse(HttpResponse::text(500, "Internal Server Error", "dup download fd failed\n"), true);
        return;
    }

    std::size_t offset = download_offset;
    std::size_t to_read = std::min(kIoBufferSize, download_size - download_offset);
    download_offset += to_read;
    download_read_pending = true;

    std::weak_ptr<Connection> weak_self = shared_from_this();
    io_pool->submit([weak_self, task_fd, offset, to_read, post = loop_callback]() {
        std::string data(to_read, '\0');
        std::size_t read_bytes = 0;
        bool success = true;
        bool eof = false;
        std::string message = "read ok";

        while (read_bytes < to_read) {
            ssize_t n = pread(task_fd, &data[read_bytes], to_read - read_bytes, static_cast<off_t>(offset + read_bytes));
            if (n > 0) {
                read_bytes += static_cast<std::size_t>(n);
            } else if (n == 0) {
                eof = true;
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                success = false;
                message = "file read failed";
                break;
            }
        }
        close(task_fd);
        data.resize(read_bytes);

        post([weak_self, data = std::move(data), eof, success, message]() mutable {
            auto self = weak_self.lock();
            if (self && self->state != State::Closed) {
                self->onDownloadReadDone(std::move(data), eof, success, message);
            }
        });
    });
}

void Connection::onDownloadReadDone(std::string data, bool eof, bool success, const std::string& message) {
    download_read_pending = false;
    if (!success) {
        finishDownload(false, message);
        queueResponse(HttpResponse::text(500, "Internal Server Error", message + "\n"), true);
        return;
    }

    if (!data.empty()) {
        transferred_bytes += data.size();
        write_buffer.append(data);
        updateEvents();
        return;
    }

    if (eof || download_offset >= download_size) {
        finishDownload(true, "download done");
        close_after_write = true;
        updateEvents();
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
        event_callback(client_fd, wantRead(), wantWrite());
    }
}

