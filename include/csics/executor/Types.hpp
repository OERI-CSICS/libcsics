#pragma once

#include <functional>
#include <utility>
namespace csics::executor {

class Job {
   public:
    template <typename F>
        requires std::invocable<F> && std::is_invocable_v<F, void>
    Job(F&& f) : func_(std::forward<F>(f)) {}

    template <typename F, typename... Args>
        requires std::invocable<F, Args...> && std::is_invocable_v<F, Args...>
    Job(F&& f, Args&&... args)
        : func_(std::bind(std::forward<F>(f), std::forward<Args>(args)...)) {}

    void operator()() { func_(); }

   private:
    std::function<void()> func_;
};
};  // namespace csics::executor
