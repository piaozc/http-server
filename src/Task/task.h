#pragma once
#include<sys/socket.h>
#include<sys/epoll.h>
#include<iostream>
#include<queue>
#include<mutex>
#include<cstring>
#include<string>

class Task {
public:
    virtual ~Task() {}
    virtual void run() = 0;
};

class ClientTask : public Task {
public:
    explicit ClientTask(int fd) : client_fd(fd) {}

    void run() override {
        char buf[1024];
        while (true) {
            ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
            if (n > 0) {
                // 这里只处理数据，不关 fd
                std::cout<<buf<<std::endl;
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
