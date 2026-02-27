#pragma once

#include <concepts>
#include <functional>
#include <utility>

#include "csics/executor/Types.hpp"
namespace csics::executor {

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
