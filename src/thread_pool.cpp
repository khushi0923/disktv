#include "thread_pool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] {

            // Each worker runs this infinite loop
            while (true) {
                std::function<void()> job;

                {
                    // Step 1: lock the queue
                    std::unique_lock<std::mutex> lock(queue_mtx_);

                    // Step 2: sleep until there is a job OR shutdown signal
                    // This RELEASES the lock while sleeping — other threads
                    // can still push jobs into the queue while we sleep
                    cv_.wait(lock, [this] {
                        return !job_queue_.empty() || stop_.load();
                    });

                    // Step 3: if shutting down and no jobs left, exit this thread
                    if (stop_.load() && job_queue_.empty()) return;

                    // Step 4: take the front job from the queue
                    job = std::move(job_queue_.front());
                    job_queue_.pop();
                }
                // Lock is released here automatically (unique_lock destructor)

                // Step 5: execute the job WITHOUT holding the lock
                // This lets other threads pick up jobs concurrently
                job();
            }
        });
    }
    std::cout << "[ThreadPool] Started " << num_threads << " worker threads.\n";
}

ThreadPool::~ThreadPool() {
    // Tell all workers to stop after finishing current job
    stop_.store(true);
    // Wake up ALL sleeping workers so they can see stop_ = true and exit
    cv_.notify_all();
    // Wait for every worker thread to finish
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    std::cout << "[ThreadPool] All workers stopped.\n";
}

void ThreadPool::submit(std::function<void()> job) {
    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        job_queue_.push(std::move(job));
    }
    // Wake ONE sleeping worker — there is exactly one new job
    cv_.notify_one();
}

size_t ThreadPool::queueSize() {
    std::lock_guard<std::mutex> lock(queue_mtx_);
    return job_queue_.size();
}