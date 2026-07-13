#pragma once

#include "../business/BusinessClient.h"
#include "../http/HttpRequest.h"
#include "../threadpool/Threadpool.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

class Connection : public std::enable_shared_from_this<Connection> {
public:
    enum class State {
        ReadingHeaders,
        PreparingUpload,
        Uploading,
        PreparingDownload,
        StreamingFile,
        Sending,
        Closed
    };

    using EventCallback = std::function<void(int fd, bool want_read, bool want_write)>;
    using LoopCallback = std::function<void(std::function<void()> callback)>;

    Connection(int fd, ThreadPool* io_pool, BusinessClient* business, EventCallback event_callback, LoopCallback loop_callback);
    ~Connection();

    void handleReadable();
    void handleWritable();

    int fd() const;
    bool wantRead() const;
    bool wantWrite() const;
    bool closed() const;

private:
    static constexpr std::size_t kIoBufferSize = 64 * 1024;
    static constexpr std::size_t kMaxPendingUploadBytes = 1024 * 1024;
    static constexpr std::size_t kMaxWriteBufferBytes = 1024 * 1024;

    int client_fd;
    int upload_fd = -1;
    int download_fd = -1;
    State state = State::ReadingHeaders;
    ThreadPool* io_pool;
    BusinessClient* business_client;
    EventCallback event_callback;
    LoopCallback loop_callback;

    std::string read_buffer;
    std::string write_buffer;
    HttpRequest request;
    TransferRequest transfer_request;
    std::size_t upload_remaining = 0;
    std::size_t upload_offset = 0;
    std::size_t pending_upload_bytes = 0;
    std::size_t pending_upload_tasks = 0;
    std::size_t download_offset = 0;
    std::size_t download_size = 0;
    bool download_read_pending = false;
    std::size_t transferred_bytes = 0;
    bool close_after_write = false;

    void consumeReadBuffer();
    void processRequest();
    void beginUpload();
    void beginDownload();
    void drainUploadBuffer();
    void scheduleUploadWrite(std::string data, std::size_t offset);
    void onUploadWriteDone(std::size_t bytes, bool success, const std::string& message);
    void scheduleDownloadRead();
    void onDownloadReadDone(std::string data, bool eof, bool success, const std::string& message);
    void finishUpload(bool success, const std::string& message);
    void finishDownload(bool success, const std::string& message);
    void queueResponse(const std::string& response, bool close_connection);
    void closeNow();
    void updateEvents();
};

