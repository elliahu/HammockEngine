#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <queue>
#include <functional>

namespace Hmck {
    // initially based of https://stackoverflow.com/questions/15752659/thread-pooling-in-c11
    // @brief Class representing a thread pool, allowing for job submition
    class ThreadPool {
    public:
        // @brief Start a threadpool
        inline void start() {
            const uint32_t num_threads = std::thread::hardware_concurrency();
            for (uint32_t ii = 0; ii < num_threads; ++ii) {
                threads.emplace_back(std::thread(&threadLoop, this));
            }
        }

        // @brief Submit a job
        // @param job instance of a job
        inline void queueJob(const std::function<void()> &job) { {
                std::unique_lock<std::mutex> lock(queue_mutex);
                jobs.push(job);
            }
            mutex_condition.notify_one();
        }

        // @brief Stop the threadpool
        inline void stop() { {
                std::unique_lock<std::mutex> lock(queue_mutex);
                should_terminate = true;
            }
            mutex_condition.notify_all();
            for (std::thread &active_thread: threads) {
                active_thread.join();
            }
            threads.clear();
        }

        // @brief  The busy function can be used in a while loop, 
        //         such that the main thread can wait the threadpool 
        //         to complete all the tasks before calling the threadpool destructor
        // @return true if busy else false
        inline bool busy() {
            bool poolbusy; {
                std::unique_lock<std::mutex> lock(queue_mutex);
                poolbusy = !jobs.empty();
            }
            return poolbusy;
        }

    private:
        inline void threadLoop() {
            while (true) {
                std::function<void()> job; {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    mutex_condition.wait(lock, [this] { return !jobs.empty() || should_terminate; });
                    if (should_terminate) {
                        return;
                    }
                    job = jobs.front();
                    jobs.pop();
                }
                job();
            }
        }

        bool should_terminate = false;
        std::mutex queue_mutex;
        std::condition_variable mutex_condition;
        std::vector<std::thread> threads;
        std::queue<std::function<void()> > jobs;
    };
}
