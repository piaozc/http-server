#pragma once

#include "../business/BusinessClient.h"
#include "../http/HttpRequest.h"

#include <cstddef>
#include <functional>
#include <string>

class Connection {
public:
    enum class State {
        ReadingHeaders,
        Uploading,
        Sending,
        StreamingFile,
        Closed
    };

    using EventCallback = std::function<void(int fd, bool want_write)>;

    Connection(int fd, BusinessClient* business, EventCallback event_callback);
    ~Connection();

    void handleReadable();
    void handleWritable();

    int fd() const;
    bool wantWrite() const;
    bool closed() const;

private:
    static constexpr std::size_t kIoBufferSize = 64 * 1024;

    int client_fd;
    int upload_fd = -1;
    int download_fd = -1;
    State state = State::ReadingHeaders;
    BusinessClient* business_client;
    EventCallback event_callback;

    std::string read_buffer;
    std::string write_buffer;
    HttpRequest request;
    TransferRequest transfer_request;
    std::size_t upload_remaining = 0;
    std::size_t transferred_bytes = 0;
    bool close_after_write = false;

    void consumeReadBuffer();
    void processRequest();
    void beginUpload();
    void beginDownload();
    void writeUploadBytes(const char* data, std::size_t size);
    void finishUpload(bool success, const std::string& message);
    void finishDownload(bool success, const std::string& message);
    void queueResponse(const std::string& response, bool close_connection);
    void closeNow();
    void updateEvents();
};
