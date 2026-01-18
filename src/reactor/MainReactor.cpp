#include"MainReator.h"
#include"SubReactor.h"
#include"../net/ServerSocket.h"
#include<iostream>
#include<fcntl.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<cstring>
#include<netdb.h>
#include<string>

//构造函数
MainReactor::MainReactor(std::vector<SubReactor*>& subs,int port)
    :subReactors(subs),next_sub(0)
{
    //创建server_fd
    server_fd=setupServerSocket(port);

    //创建epoll实例
    epoll_fd=epoll_create1(0);
    if(epoll_fd==-1){
        std::cerr<<"epoll_creat failed"<<strerror(errno)<<std::endl;
        exit(1);
    }

    //注册server_fd到epoll
    epoll_event ev;
    ev.events=EPOLLIN|EPOLLET;
    ev.data.fd=server_fd;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev)==-1){
        std::cerr<<"epoll_ctl error"<<strerror(errno)<<std::endl;
        exit(1);
    }
}

//析构函数
MainReactor::~MainReactor(){
    close(server_fd);
    close(epoll_fd);
}



//服务器核心逻辑，主循环
void MainReactor::start(){
    int n;
    epoll_event events[1024];
    while(true){
        n=epoll_wait(epoll_fd,events,1024,-1);
        for(int i=0;i<n;i++){
            if(events[i].data.fd==server_fd){
                handdle_accept();
            }else{
                //分发给SubReactor
            }
        }
    }
}

//处理连接，accept循环
void MainReactor::handdle_accept(){
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t clientaddr_len=sizeof(client_addr);
    while(true){
        client_fd=accept(server_fd,(struct sockaddr*)&client_addr,&clientaddr_len);
        //accept失败
        if(client_fd==-1){
            //到头了,退出循环
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                break;
            }else continue;
        }
        //accept成功,设置非阻塞client_fd,分发给SubReactor
        if(setnonblocking(client_fd)==-1){
            std::cerr<<"set nonblock error"<<strerror(errno)<<std::endl;
        }

    }
}

