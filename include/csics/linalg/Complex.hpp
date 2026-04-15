#pragma once

#include <cmath>
#include <tuple>

#include "csics/Buffer.hpp"
#include "csics/linalg/Concepts.hpp"
#include "csics/linalg/Coordinates.hpp"

namespace csics::linalg {

template <typename T>
class Complex {
   public:
    using value_type = T;
    constexpr Complex() = default;
    constexpr Complex(T real, T imag) : real_(real), imag_(imag) {}

    constexpr Complex(const Complex& other) = default;
    constexpr Complex(Complex&& other) = default;
    constexpr Complex& operator=(const Complex& other) = default;
    constexpr Complex& operator=(Complex&& other) = default;

    constexpr Complex(const Polar<T>& polar)
        : real_(polar.radius() * std::cos(polar.angle())),
          imag_(polar.radius() * std::sin(polar.angle())) {}

    constexpr const T real() const noexcept { return real_; }
    constexpr const T imag() const noexcept { return imag_; }
    constexpr T& real() noexcept { return real_; }
    constexpr T& imag() noexcept { return imag_; }

    constexpr void real(T r) noexcept { real_ = r; }
    constexpr void imag(T i) noexcept { imag_ = i; }

    friend std::ostream& operator<<(std::ostream& os, const Complex& c) {
        os << "(" << c.real_ << " + j" << c.imag_ << ")";
        return os;
    }

   private:
    T real_;
    T imag_;
};

static_assert(ComplexLike<Complex<float>>);

template <typename T>
class ComplexBufferView {
   public:
    using value_type = T;
    ComplexBufferView(const Buffer<T>& real_buffer,
                      const Buffer<T>& imag_buffer)
        : real_buffer_(real_buffer), imag_buffer_(imag_buffer) {}
    const T& real(std::size_t index) const { return real_buffer_[index]; }
    const T& imag(std::size_t index) const { return imag_buffer_[index]; }

    template <typename U = T>
    Complex<const U> operator[](std::size_t index) const {
        return Complex<U>(real_buffer_[index], imag_buffer_[index]);
    }

   private:
    const Buffer<T>& real_buffer_;
    const Buffer<T>& imag_buffer_;
};

template <typename T, std::size_t Alignment = alignof(T),
          CapacityPolicy Policy = CapacityPolicy::FiftyPercent()>
class ComplexBuffer {
   public:
    using value_type = T;
    ComplexBuffer(std::size_t size) : real_buffer_(size), imag_buffer_(size) {}
    T& real(std::size_t index) { return real_buffer_[index]; }
    T& imag(std::size_t index) { return imag_buffer_[index]; }

    template <typename U = T>
    Complex<const U> operator[](std::size_t index) const {
        return Complex<U>(real_buffer_[index], imag_buffer_[index]);
    }

    template <typename U = T>
    Complex<U> operator[](std::size_t index) {
        return Complex<U>(real_buffer_[index], imag_buffer_[index]);
    }

    Complex<T&> push_back(const Complex<T>& value) {
        real_buffer_.push_back(value.real());
        imag_buffer_.push_back(value.imag());
        return Complex<T&>(real_buffer_.back(), imag_buffer_.back());
    }

    operator ComplexBufferView<T>() const {
        return ComplexBufferView<T>(real_buffer_, imag_buffer_);
    }

   private:
    Buffer<T, Alignment, Policy> real_buffer_;
    Buffer<T, Alignment, Policy> imag_buffer_;
};

}  // namespace csics::linalg

// Operator implementations
namespace csics::linalg {
template <ComplexLike T>
constexpr inline T operator+(T&& a, T&& b) {
    return T(a.real() + b.real(), a.imag() + b.imag());
}

template <ComplexLike T>
constexpr inline T operator-(T&& a, T&& b) {
    return T(a.real() - b.real(), a.imag() - b.imag());
}

template <ComplexLike T>
constexpr inline T operator*(const T& a, const T& b) {
    return T(std::fma(-a.imag(), b.imag(), a.real() * b.real()),
             std::fma(a.real(), b.imag(), a.imag() * b.real()));
}

template <ComplexLike T>
constexpr inline T operator/(const T& a, const T& b) {
    auto denom = b.real() * b.real() + b.imag() * b.imag();
    return T((a.real() * b.real() + a.imag() * b.imag()) / denom,
             (a.imag() * b.real() - a.real() * b.imag()) / denom);
}

template <ComplexLike T>
constexpr inline T operator*(const T& c, const typename T::value_type& s) {
    return T(c.real() * s, c.imag() * s);
}

template <ComplexLike T>
constexpr inline T operator*(const typename T::value_type& s, const T& c) {
    return c * s;
}

template <ComplexLike T>
constexpr inline T operator/(const T& c, const typename T::value_type& s) {
    return T(c.real() / s, c.imag() / s);
}

template <ComplexLike T>
constexpr inline T operator/(const typename T::value_type& s, const T& c) {
    return T(s / c.real(), -s / c.imag());
}

template <ComplexLike T>
constexpr inline bool operator==(const T& a, const T& b) {
    return a.real() == b.real() && a.imag() == b.imag();
}

template <ComplexLike T>
constexpr inline bool operator!=(const T& a, const T& b) {
    return !(a == b);
}

template <ComplexLike T>
constexpr inline bool operator<(const T& a, const T& b) {
    return std::tie(a.real(), a.imag()) < std::tie(b.real(), b.imag());
}

template <ComplexLike T>
constexpr inline bool operator>(const T& a, const T& b) {
    return std::tie(a.real(), a.imag()) > std::tie(b.real(), b.imag());
}

template <ComplexLike T>
constexpr inline bool operator<=(const T& a, const T& b) {
    return !(a > b);
}

template <ComplexLike T>
constexpr inline bool operator>=(const T& a, const T& b) {
    return !(a < b);
}

};  // namespace csics::linalg

// std specializations
namespace std {
template <std::size_t I, csics::linalg::ComplexLike T>
constexpr T::value_type& get(const T& c) noexcept {
    if constexpr (I == 0)
        return c.real();
    else if constexpr (I == 1)
        return c.imag();
}

template <std::size_t I, csics::linalg::ComplexLike T>
constexpr T::value_type& get(T& c) noexcept {
    if constexpr (I == 0)
        return c.real();
    else if constexpr (I == 1)
        return c.imag();
}

template <std::size_t I, csics::linalg::ComplexLike T>
constexpr T::value_type&& get(T&& c) noexcept {
    if constexpr (I == 0)
        return std::forward<T>(c).real();
    else if constexpr (I == 1)
        return std::forward<T>(c).imag();
}

template <csics::linalg::ComplexLike T>
constexpr void swap(T& a, T& b) noexcept {
    using std::swap;
    swap(a.real(), b.real());
    swap(a.imag(), b.imag());
}

template <csics::linalg::ComplexLike T>
struct tuple_size<T> : std::integral_constant<std::size_t, 2> {};
template <std::size_t I, csics::linalg::ComplexLike T>
struct tuple_element<I, T> {
    using type = typename T::value_type;
};

template <csics::linalg::ComplexLike T>
struct is_arithmetic<T> : std::true_type {};

template <csics::linalg::ComplexLike T>
struct is_floating_point<T> : std::is_floating_point<typename T::value_type> {};

template <csics::linalg::ComplexLike T>
struct is_integral<T> : std::is_integral<typename T::value_type> {};

template <csics::linalg::ComplexLike T>
struct is_signed<T> : std::is_signed<typename T::value_type> {};

};  // namespace std
