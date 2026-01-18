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
        std::vector<SubReactor*>& subReactors;
        int next_sub;

        void handdle_accept();
        
};