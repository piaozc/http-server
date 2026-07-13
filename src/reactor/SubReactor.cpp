#include "SubReactor.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/eventfd.h>
#include <unistd.h>

SubReactor::SubReactor(ThreadPool* threadPool, BusinessClient* businessClient)
    : epoll_fd(-1),
      wake_fd(-1),
      running(false),
      connection_count(0),
      thread_pool(threadPool),
      business_client(businessClient) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "sub epoll_create failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    wake_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wake_fd == -1) {
        std::cerr << "eventfd create failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    epoll_event ev {};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = wake_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wake_fd, &ev) == -1) {
        std::cerr << "epoll_ctl add wake fd failed: " << strerror(errno) << std::endl;
        exit(1);
    }
}

SubReactor::~SubReactor() {
    running = false;
    if (reactor_thread.joinable()) {
        reactor_thread.join();
    }
    connections.clear();
    close(wake_fd);
    close(epoll_fd);
}

void SubReactor::addClient(int client_fd) {
    auto connection = std::make_shared<Connection>(
        client_fd,
        thread_pool,
        business_client,
        [this](int fd, bool want_read, bool want_write) { updateClientEvents(fd, want_read, want_write); },
        [this](std::function<void()> callback) { postToLoop(std::move(callback)); });

    epoll_event ev {};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
        std::cerr << "sub epoll_ctl add failed: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    connections[client_fd] = std::move(connection);
    connection_count.fetch_add(1);
}

std::size_t SubReactor::load() const {
    return connection_count.load();
}

void SubReactor::handdleEvent(struct epoll_event* events, int num_events) {
    for (int i = 0; i < num_events; ++i) {
        epoll_event& ev = events[i];
        int client_fd = ev.data.fd;
        if (client_fd == wake_fd) {
            handleWake();
            continue;
        }

        auto it = connections.find(client_fd);
        if (it == connections.end()) {
            continue;
        }

        if (ev.events & (EPOLLHUP | EPOLLERR)) {
            closeClient(client_fd);
            continue;
        }

        if (ev.events & EPOLLIN) {
            it->second->handleReadable();
        }

        if (it->second->closed()) {
            closeClient(client_fd);
            continue;
        }

        auto current = connections.find(client_fd);
        if (current != connections.end() && (ev.events & EPOLLOUT)) {
            current->second->handleWritable();
            if (current->second->closed()) {
                closeClient(client_fd);
            }
        }
    }
}

void SubReactor::thread_ew() {
    epoll_event events[1024];
    while (running) {
        int n = epoll_wait(epoll_fd, events, 1024, -1);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (n > 0) {
            handdleEvent(events, n);
        }
    }
}

void SubReactor::start() {
    running = true;
    reactor_thread = std::thread(&SubReactor::thread_ew, this);
}

void SubReactor::stop() {
    running = false;
}

void SubReactor::postToLoop(std::function<void()> callback) {
    {
        std::lock_guard<std::mutex> lock(pending_mutex);
        pending_callbacks.push(std::move(callback));
    }

    uint64_t one = 1;
    ssize_t ignored = write(wake_fd, &one, sizeof(one));
    (void)ignored;
}

void SubReactor::updateClientEvents(int client_fd, bool want_read, bool want_write) {
    if (connections.find(client_fd) == connections.end()) {
        return;
    }

    epoll_event ev {};
    ev.events = EPOLLET;
    if (want_read) {
        ev.events |= EPOLLIN;
    }
    if (want_write) {
        ev.events |= EPOLLOUT;
    }
    ev.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev) == -1) {
        closeClient(client_fd);
    }
}

void SubReactor::closeClient(int client_fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    std::size_t erased = connections.erase(client_fd);
    if (erased > 0) {
        connection_count.fetch_sub(1);
    }
    close(client_fd);
}

void SubReactor::handleWake() {
    uint64_t value = 0;
    while (read(wake_fd, &value, sizeof(value)) > 0) {
    }

    std::queue<std::function<void()>> callbacks;
    {
        std::lock_guard<std::mutex> lock(pending_mutex);
        callbacks.swap(pending_callbacks);
    }

    while (!callbacks.empty()) {
        callbacks.front()();
        callbacks.pop();
    }
}
