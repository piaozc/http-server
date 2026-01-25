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
        SubReactor(ThreadPool* threadPool=nullptr);
        ~SubReactor();
        
        void start();
        void stop();

        //MainReactor调用，在accept后，注册epoll实例
        void addClient(int client_fd);

    private:
        int epoll_fd;                       //客户端epoll实例
        std::atomic<bool> running;          //控制事件循环是否进行
        ThreadPool* thread_pool;
        std::thread reactor_thread;         //事件循环线程

        //处理epoll_wait返回的就绪事件
        void handdleEvent(struct epoll_event* events,int num_events);
        //线程函数
        void thread_ew();
};