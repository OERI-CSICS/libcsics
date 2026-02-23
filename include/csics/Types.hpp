
#pragma once
// general utility types

#include <functional>
#include <utility>
namespace csics {

template <typename E>
struct Unexpected {
    using error_type = E;
    E error_;

    Unexpected(const E& error) : error_(error) {}
    Unexpected(E&& error) : error_(std::move(error)) {}
};

template <typename E>
using unexpected = Unexpected<E>;

struct default_unexpected {
    explicit default_unexpected() = default;
};

inline constexpr default_unexpected unexpect{};

template <typename T, typename E>
class Expected {
   public:
    using value_type = T;
    using error_type = E;
    using unexpected_type = Unexpected<E>;

    Expected(const value_type& value) : value_(value), has_value_(true) {}
    Expected(value_type&& value) : value_(std::move(value)), has_value_(true) {}
    Expected(unexpected_type&& error)
        : error_(error.error_), has_value_(false) {}
    Expected(error_type&& error)
        : error_(std::move(error)), has_value_(false) {}

    Expected(default_unexpected) : error_(), has_value_(false) {}

    bool has_value() const { return has_value_; }
    const value_type& value() const { return value_; }
    const error_type& error() const { return error_; }

    operator bool() const { return has_value_; }

    template <typename F>
    auto and_then(F&& func) const {
        if (has_value_) {
            return std::invoke(std::forward<F>(func), std::forward<T>(value_));
        } else {
            return unexpect;
        }
    }

   private:
    union {
        value_type value_;
        error_type error_;
    };
    bool has_value_;
};

// using alias for eventual swap to C++23 std::expected
// if desired
template <typename T, typename E>
using expected = Expected<T, E>;
};  // namespace csics
