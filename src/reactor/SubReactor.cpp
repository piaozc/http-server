
#include<iostream>
#include<fcntl.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<unistd.h>
#include<cstring>
#include<netdb.h>
#include<string>
#include"../threadpool/Threadpool.h"
#include"SubReactor.h"
#include"../net/ServerSocket.h"
#include"../Task/task.h"

SubReactor::SubReactor(ThreadPool* threadPool){
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
    ev.data.fd=client_fd;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&ev)==-1){
        std::cerr<<"sub epoll_ctl error"<<strerror(errno)<<std::endl;
        std::cout<<"addClient error"<<std::endl;
        exit(1);
    }
}

void SubReactor::handdleEvent(struct epoll_event* events,int num_events){
    for(int i=0;i<num_events;i++){
        struct epoll_event &ev=events[i];
        int client_fd=ev.data.fd;
        if(ev.events&EPOLLIN){
            ClientTask* task=new ClientTask(client_fd);
            thread_pool->submit(task);
        }
        if(ev.events&EPOLLOUT){
            std::cout<<"IO event:OUT"<<std::endl;
        }
        if(ev.events&(EPOLLHUP|EPOLLERR)){
            std::cout<<"IO event:Closed"<<std::endl;
            close(client_fd);
            epoll_ctl(epoll_fd,EPOLL_CTL_DEL,client_fd,&ev);
        }
        
    }
}

void SubReactor::thread_ew(){
    int n;
    epoll_event events[1024];
    while(running){
        n=epoll_wait(epoll_fd,events,1024,-1);
        if(n==-1){
            if(errno==EINTR) continue;
            else break;
        }
        if(n>0){
            handdleEvent(events,n);
        }
    }
}

void SubReactor::start(){
    running=true;
    reactor_thread=std::thread(&SubReactor::thread_ew,this);
}

void SubReactor::stop(){
    running=false;
}

