#pragma once
#include <concepts>

namespace csics::linalg {
template <typename T>
concept ComplexLikeImpl = requires(T a) {
    typename T::value_type;
    { a.real() } -> std::convertible_to<typename T::value_type>;
    { a.imag() } -> std::convertible_to<typename T::value_type>;
    { T(a.real(), a.imag()) } -> std::same_as<T>;
};

template <typename T>
concept ComplexLike = ComplexLikeImpl<std::remove_cvref_t<T>>;

template <typename T>
concept ComplexPrimitive = std::is_arithmetic_v<T> && std::is_signed_v<T>;

template <typename T>
concept VecPrimitive = std::is_arithmetic_v<T>;

template <typename T>
concept ScalarLike = std::is_arithmetic_v<T>;  // maybe expanded later

template <typename T>
concept FloatScalarLike = std::floating_point<T> && ScalarLike<T>;

template <typename T>
concept StaticVecLikeImpl =
    (T::cols_v == 1 && T::rows_v >= 2 &&
    std::is_arithmetic_v<typename T::value_type>) ||
    (T::rows_v == 1 && T::cols_v >= 2 &&
    std::is_arithmetic_v<typename T::value_type>);

template <typename T>
concept StaticVecLike = StaticVecLikeImpl<std::remove_cvref_t<T>>;

template <typename T, std::size_t N>
concept StaticVecOfSizeImpl = StaticVecLike<T> &&
    ((T::cols_v == 1 && T::rows_v == N) ||
     (T::rows_v == 1 && T::cols_v == N));

template <typename T, std::size_t N>
concept StaticVecOfSize = StaticVecOfSizeImpl<std::remove_cvref_t<T>, N>;

template <typename T, typename U>
concept VecCompatible =
StaticVecLike<T> && StaticVecLike<U> &&
std::remove_cvref_t<T>::cols_v == std::remove_cvref_t<U>::rows_v &&
std::same_as<typename std::remove_cvref_t<T>::value_type,
    typename std::remove_cvref_t<U>::value_type>;

template <typename Mat>
concept SmallMatrixImpl = Mat::rows_v <= 4 && Mat::cols_v <= 4;

template <typename Mat>
concept SmallMatrix = SmallMatrixImpl<std::remove_cvref_t<Mat>>;

template <typename Mat>
concept SquareMatrixImpl = Mat::rows_v == Mat::cols_v;

template <typename Mat>
concept SquareMatrix = SquareMatrixImpl<std::remove_cvref_t<Mat>>;

template <typename Mat>
concept SmallSquareMatrixImpl = SmallMatrix<Mat> && SquareMatrix<Mat>;

template <typename Mat>
concept SmallSquareMatrix = SmallSquareMatrixImpl<std::remove_cvref_t<Mat>>;

template <typename MatA, typename MatB>
concept MatrixCompatibleImpl =
    MatA::cols_v == MatB::rows_v &&
    std::same_as<typename MatA::value_type, typename MatB::value_type>;

template <typename MatA, typename MatB>
concept MatrixCompatible =
    MatrixCompatibleImpl<std::remove_cvref_t<MatA>, std::remove_cvref_t<MatB>>;

template <typename Mat>
concept Matrix2x2 = Mat::rows_v == 2 && Mat::cols_v == 2;
template <typename Mat>
concept Matrix3x3 = Mat::rows_v == 3 && Mat::cols_v == 3;

};  // namespace csics::linalg
