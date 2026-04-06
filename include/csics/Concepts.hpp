#pragma once
#include <concepts>

namespace csics {
    template <typename T>
        concept Numeric = std::is_arithmetic_v<T> && (!std::is_same_v<T, bool>);
};
