#pragma once

#include "csics/executor/Types.hpp"
namespace csics::executor {

template <typename T>
concept Executor = requires(T t) {
    {
        t.submit([]() {})
    } -> std::same_as<void>;
    { t.join() } -> std::same_as<void>;
};
};  // namespace csics::executor
