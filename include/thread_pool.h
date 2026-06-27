#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    // Creates num_threads worker threads that immediately start waiting for jobs
    explicit ThreadPool(size_t num_threads);

    // Signals all workers to stop, waits for them to finish their current job
    ~ThreadPool();

    // Add a job to the queue — any callable (lambda, function pointer, etc.)
    void submit(std::function<void()> job);

    // How many threads in the pool
    size_t size() const { return workers_.size(); }

    // How many jobs are currently waiting in the queue
    size_t queueSize();

private:
    std::vector<std::thread>          workers_;    // the fixed set of threads
    std::queue<std::function<void()>> job_queue_;  // pending work
    std::mutex                        queue_mtx_;  // protects job_queue_
    std::condition_variable           cv_;         // workers sleep/wake on this
    std::atomic<bool>                 stop_{false};// set true during shutdown
};