#pragma once
#include<vector>

class SubReactor;

class MainReactor{
    public:
        MainReactor(std::vector<SubReactor*>& subs,int port);
        ~MainReactor();

        void start();
    private:
        int server_fd;
        int epoll_fd;
        std::vector<SubReactor*>& subReactors;  //可分配的SubReactor
        int next_sub;                           //最近可分配SubReactor下标

        void handdle_accept();      //处理连接，accept循环
};