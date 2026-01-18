#pragma once
#include<functional>

class ThreadPool{
    public:
        //构造函数和析构函数
        ThreadPool(int max);
        ~ThreadPool();

        int submit(std::function<void()> task); //由Subreactor调用，将任务提交给线程池

    private:
        int max_thread;         //最大线程数
        void thread_func();     //工作函数，由std::thread启动
};