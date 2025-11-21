#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "SafeQueue.h"

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    template<class F>
    void enqueue(F&& f) {
        tasks.push(std::forward<F>(f));
        condition.notify_one();
    }
    
    size_t pending_tasks() const {
        return tasks.size();
    }

private:
    std::vector<std::thread> workers;
    SafeQueue<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};

#endif // THREADPOOL_H