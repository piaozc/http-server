#include<iostream>
#include<fcntl.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<cstring>
#include<netdb.h>
#include<string>
#include"ServerSocket.h"

int setupServerSocket(int port){
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE;

    std::string portnum=std::to_string(port);
    int status=getaddrinfo(NULL,portnum.c_str(),&hints,&res);
    if(status!=0){
        std::cerr<<"getinfo error"<<gai_strerror(status)<<std::endl;
        exit(1);
    }

    int server_fd=-1;
    for(struct addrinfo* p=res;p;p=p->ai_next){
        //创建socket
        int fd=socket(p->ai_family,p->ai_socktype,0);
        if(fd==-1){
            std::cerr<<"socket error"<<strerror(errno)<<std::endl;
            continue;
        }

        int opt=1;
        if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1){
            std::cerr<<"setsockopt error"<<strerror(errno)<<std::endl;
            close(fd);
            continue;
        }

        //bind
        if(bind(fd,p->ai_addr,p->ai_addrlen)==-1){
            std::cerr<<"bind error"<<strerror(errno)<<std::endl;
            close(fd);
            continue;
        }
        server_fd=fd;
        break;
    }
    if(server_fd==-1){
        std::cerr<<"failed to bind"<<std::endl;
        exit(1);
    }

    //listen
    if(listen(server_fd,SOMAXCONN)==-1){
        std::cerr<<"listen error"<<strerror(errno)<<std::endl;
        exit(1);
    }

    //set nonblocking
    if(setnonblocking(server_fd)==-1){
        std::cerr<<"set nonblock error"<<strerror(errno)<<std::endl;
        exit(1);
    }

    freeaddrinfo(res);

    return server_fd;
}

int setnonblocking(int fd){
    int flag=fcntl(fd,F_GETFL,0);
    if(flag==-1) return -1;
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) return -1;
    return 0;
}