
#pragma once
// general utility types

#include <functional>
#include <utility>

#include "csics/Concepts.hpp"
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
    Expected(const unexpected_type& error)
        : error_(error.error_), has_value_(false) {}
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

template <Numeric T>
class Range {
   public:
    using value_type = T;
    Range(T start, T end) : start_(start), end_(end) {}

    auto bottom() const { return start_; }
    auto top() const { return end_; }
    auto size() const { return end_ - start_; }

    Range<T> operator+(T offset) const {
        return Range<T>(start_ + offset, end_ + offset);
    }
    Range<T> operator-(T offset) const {
        return Range<T>(start_ - offset, end_ - offset);
    }
    Range<T> union_with(const Range<T>& other) const {
        T new_start = std::min(start_, other.start_);
        T new_end = std::max(end_, other.end_);
        return Range<T>(new_start, new_end);
    }
    Range<T> intersection_with(const Range<T>& other) const {
        T new_start = std::max(start_, other.start_);
        T new_end = std::min(end_, other.end_);
        if (new_start >= new_end) {
            return Range<T>(0, 0);  // or throw an exception for no intersection
        }
        return Range<T>(new_start, new_end);
    }

   private:
    T start_;
    T end_;

   public:
    class StepIterator {
       public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        using iterator_category = std::input_iterator_tag;

        StepIterator(T start, T step, T end)
            : start_(start), current_(0), step_(step), end_(end) {}

        value_type operator*() const { return current_ * step_ + start_; }

        StepIterator& operator++() {
            current_++;
            return *this;
        }

        StepIterator operator++(int) {
            StepIterator temp = *this;
            current_++;
            return temp;
        }

        bool operator==(const StepIterator& o) const {
            return current_ * step_ + start_ >= end_ &&
                   o.current_ * o.step_ + o.start_ >= o.end_;
        }

       private:
        std::size_t current_;
        T start_;
        T step_;
        T end_;
        friend class StepSentinel;
    };

    class StepSentinel {
       public:
        bool operator==(const StepIterator& it) const {
            return it.current_ * it.step_ + it.start_ >= it.end_;
        }
    };

    class StepView {
       public:
        StepView(T start, T step, T end)
            : start_(start), step_(step), end_(end) {}

        auto begin() const { return StepIterator(start_, step_, end_); }
        auto end() const { return StepSentinel(); }

       private:
        T start_;
        T step_;
        T end_;
    };

    auto step(T step_size) const { return StepView(start_, step_size, end_); }
};

};  // namespace csics
