#pragma once

#include "../business/BusinessClient.h"
#include "../connection/Connection.h"
#include "../threadpool/Threadpool.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <sys/epoll.h>
#include <thread>
#include <unordered_map>

class SubReactor {
public:
    SubReactor(ThreadPool* threadPool, BusinessClient* businessClient);
    ~SubReactor();

    void start();
    void stop();

    void addClient(int client_fd);
    std::size_t load() const;

private:
    int epoll_fd;
    std::atomic<bool> running;
    std::atomic<std::size_t> connection_count;
    ThreadPool* thread_pool;
    BusinessClient* business_client;
    std::thread reactor_thread;
    std::unordered_map<int, std::unique_ptr<Connection>> connections;

    void handdleEvent(struct epoll_event* events, int num_events);
    void thread_ew();
    void updateClientEvents(int client_fd, bool want_write);
    void closeClient(int client_fd);
};
