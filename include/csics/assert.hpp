
#pragma once

#include <concepts>
#include <cstdio>
#include <cstdlib>

namespace csics {

#define CSICS_STATIC_ASSERT(condition, msg) static_assert(condition, msg)

#ifdef CSICS_DEBUG
constexpr bool csics_debug = true;
#define CSICS_RUNTIME_ASSERT(condition, msg) \
    ::csics::csics_runtime_assert(condition, msg)
#else
constexpr bool csics_debug = false;
#define CSICS_RUNTIME_ASSERT(c,m) do {} while (0)
#endif

template <typename T>
concept DebugAssertable = requires(T t) {
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.data() } -> std::convertible_to<const char*>;
};

template <DebugAssertable T>
inline void csics_runtime_assert(bool condition, T msg) {
    if constexpr (csics_debug) {
        if (!condition) {
            std::fprintf(stderr, "Assertion failed: %.*s\n", int(msg.size()),
                         msg.data());
            std::abort();
        }
    }
}

template <size_t N>
inline void csics_runtime_assert(bool condition, const char (&msg)[N]) {
    if constexpr (csics_debug) {
        if (!condition) {
            std::fprintf(stderr, "Assertion failed: %.*s\n", int(N), msg);
            std::abort();
        }
    }
}

};  // namespace csics
