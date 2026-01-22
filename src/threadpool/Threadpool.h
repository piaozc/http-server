#pragma once
#include<functional>
#include<queue>
#include<thread>
#include<mutex>
#include<condition_variable>

class ThreadPool{
    public:
        //构造函数和析构函数
        ThreadPool(int max);
        ~ThreadPool();

        int submit(std::function<void()> task); //由Subreactor调用，将任务提交给线程池

    private:
        int max_thread;         //最大线程数
        std::vector<std::thread> workers;             // 工作线程
        std::queue<std::function<void()>> task_queue; // 任务队列
        std::mutex queue_mutex;
        std::condition_variable cv;
        bool running;

        void thread_func();     //工作函数，由std::thread启动
};