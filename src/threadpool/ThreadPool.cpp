
#include<iostream>
#include"../reactor/SubReactor.h"
#include"../Task/task.h"
#include<mutex>
#include<condition_variable>
#include<atomic>
#include<thread>
#include<queue>

ThreadPool::ThreadPool(int max){
    max_thread=max;
    running=true;
    for(int i=0;i<max_thread;i++){
        std::thread worker(&ThreadPool::thread_func,this);
        workers.push_back(std::move(worker));
    }
}

ThreadPool::~ThreadPool(){
    running=false;
    cv.notify_all();
    for(int i=0;i<max_thread;i++){
        if(workers[i].joinable()){
            workers[i].join();
        }
    }
}

int ThreadPool::submit(Task* task){
    if(!running) return -1;
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        task_queue.push(task);
    }
    cv.notify_one();
    return 0;
}

void ThreadPool::thread_func() {
    while (running) {
        Task* task = nullptr;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            while (task_queue.empty() && running) {
                cv.wait(lock);  // 等任务
            }
            if (!running) break;
            task = task_queue.front();
            task_queue.pop();
        }  // 🔑 立刻释放锁

        task->run();
        delete task;
    }
}
