#pragma once

#include <concepts>
#include <functional>
#include <thread>
#include <utility>

#include "csics/executor/Types.hpp"
namespace csics::executor {

inline void pin_to_core(std::thread& t, int core_id) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        throw std::runtime_error("Error calling pthread_setaffinity_np: " + std::to_string(rc));
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
    if (pthread_setschedparam(t.native_handle(), SCHED_FIFO, &sch_params) != 0) {
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
};
};  // namespace csics::executor
