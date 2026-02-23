#pragma once

#include <cmath>
#include "csics/linalg/Concepts.hpp"
#include <tuple>

namespace csics::linalg {

template <typename T>
class Complex {
   public:
    using value_type = T;
    constexpr Complex() = default;
    constexpr Complex(T real, T imag) : real_(real), imag_(imag) {}

    constexpr const T real() const noexcept { return real_; }
    constexpr const T imag() const noexcept { return imag_; }
    constexpr T& real() noexcept { return real_; }
    constexpr T& imag() noexcept { return imag_; }

    constexpr void real(T r) noexcept { real_ = r; }
    constexpr void imag(T i) noexcept { imag_ = i; }

   private:
    T real_;
    T imag_;
};

static_assert(ComplexLike<Complex<float>>);

}  // namespace csics

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

};  // namespace csics

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
