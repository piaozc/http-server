
#include"./reactor/MainReator.h"
#include"./reactor/SubReactor.h"
#include"./threadpool/Threadpool.h"
#include<vector>
#include<iostream>
#include<mutex>

int main(){
    int thread_num=12;
    ThreadPool pool(thread_num);
    std::cout<<"thread pool start"<<std::endl;

    int sub_reactor_num=3;
    std::vector<SubReactor*> subReactors;
    for(int i=0;i<sub_reactor_num;i++){
        subReactors.push_back(new SubReactor(&pool));
    }
    std::cout<<"subR init"<<std::endl;

    for(SubReactor* sub:subReactors){
        sub->start();
    }
    std::cout<<"subR start"<<std::endl;
    
    int port=10888;
    MainReactor mainReactor(subReactors,port);
    mainReactor.start();

    return 0;
}