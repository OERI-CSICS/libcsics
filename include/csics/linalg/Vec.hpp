#pragma once

#include <concepts>
#include <csics/linalg/Concepts.hpp>
#include <csics/linalg/Ops.hpp>
#include <csics/linalg/Matrix.hpp>
#include <cstddef>
#include <tuple>
namespace csics::linalg {

    template <typename T, std::size_t N>
    using ColumnVec = Matrix<T, N, 1>;

    template <typename T, std::size_t N>
    using RowVec = Matrix<T, 1, N>;

    template<typename T, std::size_t N>
    using Vec = ColumnVec<T, N>;

    template <typename T>
    using Vec2 = ColumnVec<T, 2>;

    template <typename T>
    using Vec3 = ColumnVec<T, 3>;
    
    template <typename T>
    using Vec4 = ColumnVec<T, 4>;
};
