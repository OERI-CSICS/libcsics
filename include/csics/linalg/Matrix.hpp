#pragma once

#include <concepts>
#include <csics/linalg/Concepts.hpp>
#include <csics/linalg/Ops.hpp>
#include <cstddef>
#include <utility>

namespace csics::linalg {

template <typename T, std::size_t Rows, std::size_t Cols>
class Matrix {
   public:
    using value_type = T;
    static constexpr std::size_t rows_v = Rows;
    static constexpr std::size_t cols_v = Cols;
    using col_vec = Matrix<T, Rows, 1>;
    using row_vec = Matrix<T, 1, Cols>;

    Matrix() = default;
    template <typename... Args>
        requires(sizeof...(Args) == Rows * Cols) &&
                (std::same_as<Args, T> && ...)
    Matrix(Args... xs) : data_{xs...} {}

    constexpr Matrix(std::initializer_list<std::initializer_list<T>> init) {
        std::size_t i = 0;
        for (const auto& row : init) {
            std::size_t j = 0;
            for (const auto& val : row) {
                if (i < Rows && j < Cols) {
                    data_[calculate_index(i, j)] = val;
                }
                ++j;
            }
            ++i;
        }
    }

    template <std::size_t I, std::size_t J>
    constexpr const T& get() const noexcept {
        static_assert(I < Rows && J < Cols, "Index out of bounds");
        return data_[I * Cols + J];
    }

    template <std::size_t I>
    constexpr const T& get() const {
        static_assert(I < Rows * Cols, "Index out of bounds");
        return data_[I];
    }

    consteval std::size_t size() { return Rows * Cols; }
    template <std::size_t I>
    constexpr T& get() {
        static_assert(I < Rows * Cols, "Index out of bounds");
        return data_[I];
    }

    template <std::size_t I, std::size_t J>
    constexpr T& get() {
        static_assert(I < Rows && J < Cols, "Index out of bounds");
        return data_[calculate_index(I, J)];
    }

    consteval std::size_t rows() { return Rows; }

    consteval std::size_t cols() { return Cols; }

    constexpr row_vec get_row(std::size_t i) const noexcept {
        return mat_get_row_impl<row_vec>(i, std::make_index_sequence<Cols>{});
    }

    constexpr col_vec get_col(std::size_t j) const noexcept {
        return mat_get_col_impl<col_vec>(j, std::make_index_sequence<Rows>{});
    }

    constexpr T& operator()(std::size_t i, std::size_t j) noexcept {
        return data_[calculate_index(i, j)];
    }
    constexpr const T& operator()(std::size_t i, std::size_t j) const noexcept {
        return data_[calculate_index(i, j)];
    }

   private:
    T data_[Rows * Cols];

    constexpr std::size_t calculate_index(std::size_t i,
                                          std::size_t j) const noexcept {
        return i * Cols + j;
    }

    template <StaticVecLike V, std::size_t... Is>
    constexpr auto mat_get_row_impl(std::size_t i,
                                    std::index_sequence<Is...>) const noexcept {
        return row_vec{
            (data_[calculate_index(i, Is)])...};
    }
    template <StaticVecLike V, std::size_t... Is>
    constexpr auto mat_get_col_impl(std::size_t j,
                                    std::index_sequence<Is...>) const noexcept {
        return col_vec{
            (data_[calculate_index(Is, j)])...};
    }
};

template <SmallMatrix Mat, std::size_t... Is>
constexpr auto mat_add_impl(Mat&& a, Mat&& b, std::index_sequence<Is...>) {
    using Tp = std::remove_cvref_t<Mat>;
    using T = typename Tp::value_type;
    return Tp{(std::forward<Mat>(a).template get<Is>() +
               std::forward<Mat>(b).template get<Is>())...};
}

template <SmallMatrix Mat>
constexpr auto operator+(Mat&& a, Mat&& b) {
    using Tp = std::remove_cvref_t<Mat>;
    return mat_add_impl(std::forward<Mat>(a), std::forward<Mat>(b),
                        std::make_index_sequence<Tp::rows_v * Tp::cols_v>{});
}

template <SmallMatrix Mat, std::size_t... Is>
constexpr auto mat_sub_impl(Mat&& a, Mat&& b, std::index_sequence<Is...>) {
    using Tp = std::remove_cvref_t<Mat>;
    using T = typename Tp::value_type;
    return Tp{(std::forward<Mat>(a).template get<Is>() -
               std::forward<Mat>(b).template get<Is>())...};
}

template <SmallMatrix Mat>
constexpr auto operator-(Mat&& a, Mat&& b) {
    using Tp = std::remove_cvref_t<Mat>;
    return mat_sub_impl(std::forward<Mat>(a), std::forward<Mat>(b),
                        std::make_index_sequence<Tp::rows_v * Tp::cols_v>{});
}

template <SmallMatrix Mat, ScalarLike S, std::size_t... Is>
constexpr auto mat_scalar_mul_impl(Mat&& m, S s, std::index_sequence<Is...>) {
    using Tp = std::remove_cvref_t<Mat>;
    using T = typename Tp::value_type;
    return Tp{(std::forward<Mat>(m).template get<Is>() * s)...};
}

template <SmallMatrix Mat, ScalarLike S>
constexpr auto operator*(Mat&& m, S s) {
    using Tp = std::remove_cvref_t<Mat>;
    return mat_scalar_mul_impl(
        std::forward<Mat>(m), s,
        std::make_index_sequence<Tp::rows_v * Tp::cols_v>{});
}

template <SmallMatrix Mat, ScalarLike S>
constexpr auto operator*(S s, Mat&& m) {
    using Tp = std::remove_cvref_t<Mat>;
    return mat_scalar_mul_impl(
        std::forward<Mat>(m), s,
        std::make_index_sequence<Tp::rows_v * Tp::cols_v>{});
}

template <SmallMatrix Mat, ScalarLike S, std::size_t... Is>
constexpr auto mat_scalar_div_impl(Mat&& m, S s, std::index_sequence<Is...>) {
    using Tp = std::remove_cvref_t<Mat>;
    using T = typename Tp::value_type;
    return Tp{(std::forward<Mat>(m).template get<Is>() / s)...};
}

template <SmallMatrix Mat, ScalarLike S>
constexpr auto operator/(Mat&& m, S s) {
    using Tp = std::remove_cvref_t<Mat>;
    return mat_scalar_div_impl(
        std::forward<Mat>(m), s,
        std::make_index_sequence<Tp::rows_v * Tp::cols_v>{});
}

// now for the hard part...
// matrix multiplication and division (i.e. inverse) are more complex and
// require more careful handling of indices and dimensions.

template <SmallMatrix MatA, SmallMatrix MatB, std::size_t I, std::size_t J,
          std::size_t K, std::size_t... Ks>
constexpr auto mat_mul_element(
    MatA&& a, MatB&& b,
    std::index_sequence<Ks...>) {  // computes the (I,J) element of the product
                                   // of a and b
    using T = typename MatA::value_type;
    T result = T{};
    (mac(result, a(I, Ks), b(Ks, J)), ...);  // dot product of Ith row of a and
                                             // Jth column of b
    return result;
}

template <SmallMatrix MatA, SmallMatrix MatB, std::size_t J, std::size_t K,
          std::size_t... Is, std::size_t... Js>
constexpr auto mat_mul_impl(MatA&& a, MatB&& b, std::index_sequence<Is...>) {
    Matrix<typename MatA::value_type, MatA::rows_v, MatB::cols_v> result;
    (
        [&]() {
            constexpr std::size_t I = Is;
            ((result(I, Js) =
                  mat_mul_element<MatA, MatB, I, Js,
                                  MatA::cols_v>(  // computes Jth column
                      a, b, std::make_index_sequence<MatA::cols_v>{})),
             ...);
        }(),
        ...);  // over all rows
    return result;
}

template <SmallMatrix MatA, SmallMatrix MatB>
    requires MatrixCompatible<MatA, MatB>
constexpr auto operator*(MatA&& a, MatB&& b) {
    return mat_mul_impl(std::forward<MatA>(a), std::forward<MatB>(b),
                        std::make_index_sequence<MatA::rows_v>{});
}

template <SmallMatrix Mat>
constexpr auto operator==(Mat&& a, Mat&& b) {
    using Tp = std::remove_cvref_t<Mat>;
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return ((a.template get<Is>() == b.template get<Is>()) && ...);
    }(std::make_index_sequence<Tp::rows_v * Tp::cols_v>{});
}

template <SmallMatrix Mat>
constexpr auto operator!=(Mat&& a, Mat&& b) {
    return !(a == b);
}
};  // namespace csics::linalg
