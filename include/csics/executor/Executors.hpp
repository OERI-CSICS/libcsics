#pragma once

#include <concepts>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

#include "csics/executor/Types.hpp"
namespace csics::executor {

inline void pin_to_core(std::thread& t, int core_id) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int rc =
        pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        throw std::runtime_error("Error calling pthread_setaffinity_np: " +
                                 std::to_string(rc));
    }
#else
    // Core pinning not supported on this platform
    (void)t;
    (void)core_id;
#endif
}

inline void set_priority(std::thread& t, int priority) {
#ifdef __linux__
    struct sched_param sch_params;
    sch_params.sched_priority = priority;
    if (pthread_setschedparam(t.native_handle(), SCHED_FIFO, &sch_params) !=
        0) {
        pthread_setschedparam(t.native_handle(), SCHED_OTHER, &sch_params);
    }
#else
    // Thread priority not supported on this platform
    (void)t;
    (void)priority;
#endif
}

inline void set_highest_priority(std::thread& t) {
#ifdef __linux__
    int max_priority = sched_get_priority_max(SCHED_FIFO);
    set_priority(t, max_priority);
#else
    // Thread priority not supported on this platform
    (void)t;
#endif
}

class SingleThreadedExecutor {
   public:
    template <typename F, typename... Args>
        requires std::invocable<F, Args...> && std::is_invocable_v<F, Args...>
    void submit(F&& f, Args&&... args) {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    }

    void submit(Job&& job) { job(); }

    void join() {}
    void wait_for_all() {}
};

class ThreadPoolExecutor {
   public:
    explicit ThreadPoolExecutor(
        size_t nThreads = std::thread::hardware_concurrency())
        : m_stop(false) {
        m_workers.reserve(nThreads);
        for (size_t i = 0; i < nThreads; i++) {
            m_workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(m_mutex);
                        m_cv.wait(lock, [this] {
                            return m_stop || !m_tasks.empty();
                        });
                        if (m_stop && m_tasks.empty()) return;
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPoolExecutor() {
        {
            std::scoped_lock lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto& w : m_workers) w.join();
    }

    // Non-copyable, non-movable
    ThreadPoolExecutor(const ThreadPoolExecutor&) = delete;
    ThreadPoolExecutor& operator=(const ThreadPoolExecutor&) = delete;

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto future = task->get_future();
        {
            std::scoped_lock lock(m_mutex);
            if (m_stop)
                throw std::runtime_error("submit on stopped ThreadPool");
            m_tasks.emplace([task] { (*task)(); });
        }
        m_cv.notify_one();
        return future;
    }

   private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop;
};  
};  // namespace csics::executor
