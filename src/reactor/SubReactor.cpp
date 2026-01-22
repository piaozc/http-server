#pragma once
#include<iostream>
#include<fcntl.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<cstring>
#include<netdb.h>
#include<string>
#include"../threadpool/Threadpool.h"
#include"SubReactor.h"
#include"../net/ServerSocket.h"

SubReactor::SubReactor(ThreadPool* threadPool=nullptr){
    //创建自己的epoll
    epoll_fd=epoll_create1(0);
    if(epoll_fd==-1){
        std::cerr<<"sub epoll_creat failed"<<strerror(errno)<<std::endl;
        exit(1);
    }

    //初始化运行状态running=false
    running=false;
    
    thread_pool=threadPool;
}

SubReactor::~SubReactor(){
    if(reactor_thread.joinable()){
        reactor_thread.join();
    }
    running=false;
    close(epoll_fd);
    thread_pool=nullptr;
}

void SubReactor::addClient(int client_fd){
    epoll_event ev;
    ev.events=EPOLLIN|EPOLLET;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&ev)==-1){
        std::cerr<<"sub epoll_ctl error"<<strerror(errno)<<std::endl;
        exit(1);
    }
}

void SubReactor::handdleEvent(struct epoll_event* events,int num_events){
    for(int i=0;i<num_events;i++){
        struct epoll_event &ev=events[i];
        int client_fd=ev.data.fd;
        if(ev.events&EPOLLIN){
            auto task=[client_fd](){
                //读逻辑
            };
            if(thread_pool){
                thread_pool->submit(task);
            }else{
                std::cerr<<"thread_pool is nullptr"<<std::endl;
            }
        }
        if(ev.events&EPOLLOUT){
            auto task=[client_fd](){
                //写逻辑
            };
            if(thread_pool){
                thread_pool->submit(task);
            }else{
                std::cerr<<"thread_pool is nullptr"<<std::endl;
            }
        }
        if(ev.events&(EPOLLHUP|EPOLLERR)){
            close(client_fd);
            epoll_ctl(epoll_fd,EPOLL_CTL_DEL,client_fd,&ev);
        }
        
    }
}
