#pragma once

#include "../Task/task.h"

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(int max);
    ~ThreadPool();

    int submit(Task* task);
    int submit(std::function<void()> task);
    std::size_t queueSize();
    int workerCount() const;

private:
    int max_thread;
    std::vector<std::thread> workers;
    std::queue<Task*> task_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool running;

    void thread_func();
};
