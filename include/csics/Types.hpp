
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
    Expected(const unexpected_type& error) : error_(error.error_), has_value_(false) {}
    Expected(error_type&& error)
        : error_(std::move(error)), has_value_(false) {}
    Expected(const error_type& error) : error_(error), has_value_(false) {}

    Expected(const Expected& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) value_type(other.value_);
        } else {
            new (&error_) error_type(other.error_);
        }
    }

    Expected(Expected&& other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) value_type(std::move(other.value_));
        } else {
            new (&error_) error_type(std::move(other.error_));
        }
    }

    Expected(default_unexpected) : error_(), has_value_(false) {}

    ~Expected() {
        if (has_value_) {
            value_.~value_type();
        } else {
            error_.~error_type();
        }
    }

    value_type& operator*() { return value_; }
    const value_type& operator*() const { return value_; }
    value_type* operator->() { return &value_; }
    const value_type* operator->() const { return &value_; }

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

template <typename E>
class Expected<void, E> {
   public:
    using value_type = void;
    using error_type = E;
    using unexpected_type = Unexpected<E>;

    Expected() : has_value_(true) {}
    Expected(unexpected_type&& error)
        : error_(error.error_), has_value_(false) {}
    Expected(error_type&& error)
        : error_(std::move(error)), has_value_(false) {}

    Expected(default_unexpected) : error_(), has_value_(false) {}

    ~Expected() {
        if (!has_value_) {
            error_.~error_type();
        }
    }

    bool has_value() const { return has_value_; }
    const error_type& error() const { return error_; }

    operator bool() const { return has_value_; }

   private:
    union {
        char dummy_;
        error_type error_;
    };
    bool has_value_;
};

// using alias for eventual swap to C++23 std::expected
// if desired
template <typename T, typename E>
using expected = Expected<T, E>;

};  // namespace csics
