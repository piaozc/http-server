#include "Threadpool.h"

#include <utility>

ThreadPool::ThreadPool(int max) {
    max_thread = max;
    running = true;
    for (int i = 0; i < max_thread; ++i) {
        std::thread worker(&ThreadPool::thread_func, this);
        workers.push_back(std::move(worker));
    }
}

ThreadPool::~ThreadPool() {
    running = false;
    cv.notify_all();
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

int ThreadPool::submit(Task* task) {
    if (!running) {
        return -1;
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        task_queue.push(task);
    }
    cv.notify_one();
    return 0;
}

int ThreadPool::submit(std::function<void()> task) {
    Task* wrapped = new FunctionTask(std::move(task));
    int result = submit(wrapped);
    if (result != 0) {
        delete wrapped;
    }
    return result;
}

std::size_t ThreadPool::queueSize() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return task_queue.size();
}

int ThreadPool::workerCount() const {
    return max_thread;
}

void ThreadPool::thread_func() {
    while (running) {
        Task* task = nullptr;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            while (task_queue.empty() && running) {
                cv.wait(lock);
            }
            if (!running) {
                break;
            }
            task = task_queue.front();
            task_queue.pop();
        }

        task->run();
        delete task;
    }
}
