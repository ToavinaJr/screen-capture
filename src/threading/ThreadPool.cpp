#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    this->condition.wait(lock, [this] { 
                        return this->stop || !this->tasks.empty(); 
                    });
                    
                    if (this->stop && this->tasks.empty()) {
                        return;
                    }
                    
                    auto opt_task = this->tasks.try_pop();
                    if (!opt_task) continue;
                    task = std::move(*opt_task);
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(mutex);
        stop = true;
    }
    condition.notify_all();
    
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}