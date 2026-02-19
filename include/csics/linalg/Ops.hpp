#pragma once
#include <cmath>
#include <csics/linalg/Concepts.hpp>
#include <tuple>

namespace csics::linalg {

// multiply and accumulate
// a * b + c
class Mac {
   public:
    template <typename T>
    inline constexpr void operator()(T& a, const T& b,
                                     const T& c) const noexcept {
        return apply(a, b, c);
    }

    template <ScalarLike Scalar>
    inline constexpr static void apply(Scalar& a, const Scalar& b,
                                       const Scalar& c) noexcept {
        if constexpr (std::floating_point<Scalar>) {
            a = std::fma(b, c, a);
        } else {
            a += b * c;
        }
    }

    template <ComplexLike ComplexU>
    inline constexpr static void apply(ComplexU& a, const ComplexU& b,
                                       const ComplexU& c) noexcept {
        auto i = a.imag();
        auto r = a.real();  // store old accumulator
        a.real() = std::fma(b.real(), c.real(), r) - b.imag() * c.imag();
        a.imag() = std::fma(b.real(), c.imag(), i) + b.imag() * c.real();
    }
};

constexpr Mac mac;

class MacConj {
   public:
    template <typename T>
    inline constexpr void operator()(T& a, const T& b,
                                     const T& c) const noexcept {
        return apply(a, b, c);
    }

    template <ScalarLike Scalar>
    inline constexpr static void apply(Scalar& a, const Scalar& b,
                                       const Scalar& c) noexcept {
        a += b * c;
    }

    template <FloatScalarLike Scalar>
    inline constexpr static void apply(Scalar& a, const Scalar& b,
                                       const Scalar& c) noexcept {
        a = std::fma(b, c, a);
    }

    template <ComplexLike ComplexU>
        requires std::floating_point<typename ComplexU::value_type>
    inline constexpr static void apply(ComplexU& a, const ComplexU& b,
                                       const ComplexU& c) noexcept {
        auto i = a.i;
        auto r = a.r;  // store old accumulator
        a.r = std::fma(b.r, c.r, r) + b.i * c.i;
        a.i = std::fma(b.r, -c.i, i) + b.i * c.r;
    }
};

class Dot {
   public:
    template <typename T>
    inline constexpr auto operator()(const T& a, const T& b) const noexcept {
        return apply(a, b);
    }

    template <SmallMatrix MatA, SmallMatrix MatB>
        requires VecCompatible<MatA, MatB>
    inline constexpr static auto apply(const MatA& a, const MatB& b) noexcept {
        using T = typename MatA::value_type;
        T result = T{};
        std::apply(
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (mac(result, a.template get<Is>(a), b.template get<Is>(
                            b)),
                  ...);
                },
                std::make_index_sequence<MatA::rows_v * MatA::cols_v>{});
        return result;
    }
};

constexpr Dot dot;

class Cross {
   public:
    template <typename T>
    inline constexpr auto operator()(T&& a, T&& b) const noexcept {
        return apply(std::forward<T>(a), std::forward<T>(b));
    }

    template <StaticVecOfSize<3> Vec3U>
    inline constexpr static auto apply(Vec3U&& a, Vec3U&& b) noexcept {
        return Vec3U(a.y() * b.z() - a.z() * b.y(),
                     a.z() * b.x() - a.x() * b.z(),
                     a.x() * b.y() - a.y() * b.x());
    };
};

constexpr Cross cross;

class Mag {
   public:
    template <typename T>
    inline constexpr auto operator()(const T& v) const noexcept {
        return apply(v);
    }

    template <StaticVecLike VecU>
    inline constexpr static auto apply(const VecU& v) noexcept {
        return std::sqrt(std::apply(
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return ((v.template get<Is>(v) * v.template get<Is>(v)) + ...);
            },
            std::make_index_sequence<VecU::size_v>{}));
    }
};

constexpr Mag mag;

class Abs {
   public:
    template <typename T>
    inline constexpr auto operator()(const T& v) const noexcept {
        return apply(v);
    }

    template <StaticVecLike VecU>
    inline constexpr static auto apply(const VecU& v) noexcept {
        return std::sqrt(std::apply(
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return ((v.template get<Is>(v) * v.template get<Is>(v)) + ...);
            },
            std::make_index_sequence<VecU::size_v>{}));
    }
};

constexpr Abs abs;

class Conj {
   public:
    template <typename T>
    inline constexpr auto operator()(const T& v) const noexcept {
        return apply(v);
    }

    template <ComplexLike ComplexU>
    inline constexpr static auto apply(const ComplexU& c) noexcept {
        return ComplexU(c.real(), -c.imag());
    }
};

constexpr Conj conj;

};  // namespace csics::linalg
