#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace MMO::Core
{
    class ThreadPool 
    {
        
    public:
        explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency()) 
        {
            for (size_t i = 0; i < numThreads; ++i)
            {
                m_workers.emplace_back([this](std::stop_token stoken)
                {
                    while (!stoken.stop_requested()) 
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock lock(m_mutex);
                            m_cv.wait(lock, stoken, [this]
                            {
                                return !m_tasks.empty();
                            });
                            
                            if (stoken.stop_requested() && m_tasks.empty()) 
                                return;
                            
                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                        }
                        
                        task();
                    }
                });
            }
        }

        template<class F>
        void Enqueue(F&& f) 
        {
            {
                std::unique_lock lock(m_mutex);
                m_tasks.emplace(std::forward<F>(f));
            }
            m_cv.notify_one();
        }

    private:
        std::vector<std::jthread> m_workers;
        std::queue<std::function<void()>> m_tasks;
        std::mutex m_mutex;
        std::condition_variable_any m_cv;
    };
}