#pragma once

#include <cerrno>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <utility>

class Task {
public:
    virtual ~Task() = default;
    virtual void run() = 0;
};

class FunctionTask : public Task {
public:
    explicit FunctionTask(std::function<void()> fn) : fn(std::move(fn)) {}

    void run() override {
        fn();
    }

private:
    std::function<void()> fn;
};

class ClientTask : public Task {
public:
    explicit ClientTask(int fd) : client_fd(fd) {}

    void run() override {
        char buf[1024];
        while (true) {
            ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
            if (n > 0) {
                std::cout.write(buf, n);
                std::cout << std::endl;
            } else if (n == 0) {
                closed = true;
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                closed = true;
                break;
            }
        }
    }

    bool needClose() const { return closed; }
    int fd() const { return client_fd; }

private:
    int client_fd;
    bool closed = false;
};
