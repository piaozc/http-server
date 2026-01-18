#pragma once
#include"../net/ServerSocket.h"
#include"../threadpool/Threadpool.h"
#include<vector>
#include<sys/epoll.h>
#include<thread>
#include<atomic>

class SubReactor{
    public:
        //构造函数和析构函数
        SubReactor();
        ~SubReactor();

        void start();
        void stop();

        //MainReactor调用，在accept后
        void addClient(int client_fd);

    private:
        int epoll_fd;                       //客户端epoll实例
        std::vector<int> client_fd;         //当前管理的客户端fd
        std::atomic<bool> running;          //控制事件循环是否进行
        ThreadPool* thread_pool;
        std::thread reactor_thread;         //事件循环线程

        //处理epoll_wait返回的就绪事件
        void handdleEvent(struct epoll_event* events,int num_events);

        //内部注册客户端fd到epoll
        void registerClient(int client_fd);

};