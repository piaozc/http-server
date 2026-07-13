#include "MainReactor.h"

#include "../net/ServerSocket.h"
#include "SubReactor.h"

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

MainReactor::MainReactor(std::vector<SubReactor*>& subs, int port)
    : server_fd(-1), epoll_fd(-1), subReactors(subs), next_sub(0) {
    server_fd = setupServerSocket(port);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "epoll_create failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    epoll_event ev {};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        std::cerr << "epoll_ctl add server failed: " << strerror(errno) << std::endl;
        exit(1);
    }
}

MainReactor::~MainReactor() {
    close(server_fd);
    close(epoll_fd);
}

void MainReactor::start() {
    std::cout << "main reactor start" << std::endl;
    epoll_event events[1024];
    while (true) {
        int n = epoll_wait(epoll_fd, events, 1024, -1);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == server_fd) {
                handdle_accept();
            }
        }
    }
}

void MainReactor::handdle_accept() {
    while (true) {
        sockaddr_in client_addr {};
        socklen_t clientaddr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &clientaddr_len);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            continue;
        }

        if (setnonblocking(client_fd) == -1) {
            std::cerr << "set client nonblocking failed: " << strerror(errno) << std::endl;
            close(client_fd);
            continue;
        }

        chooseSubReactor()->addClient(client_fd);
    }
}

SubReactor* MainReactor::chooseSubReactor() {
    SubReactor* selected = subReactors[next_sub];
    std::size_t selected_load = selected->load();

    for (SubReactor* sub : subReactors) {
        std::size_t load = sub->load();
        if (load < selected_load) {
            selected = sub;
            selected_load = load;
        }
    }

    next_sub = (next_sub + 1) % subReactors.size();
    return selected;
}
