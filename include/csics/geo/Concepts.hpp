#pragma once

#include <concepts>
namespace csics::geo {


template <typename E>
concept Ellipsoid = requires(E e) {
    { E::a } -> std::convertible_to<double>;
    { E::f } -> std::convertible_to<double>;
};


template <typename T, typename E>
concept GeodeticCoordinate = requires(std::remove_cvref_t<T> a) {
    typename T::value_type;
    { a.latitude() } -> std::convertible_to<double>;
    { a.longitude() } -> std::convertible_to<double>;
    { a.altitude() } -> std::convertible_to<double>;
} && Ellipsoid<E>;

template <typename T>
concept GeodeticLike = requires(std::remove_cvref_t<T> a) {
    typename T::value_type;
    { a.latitude() } -> std::convertible_to<double>;
    { a.longitude() } -> std::convertible_to<double>;
    { a.altitude() } -> std::convertible_to<double>;
} && Ellipsoid<typename std::remove_cvref_t<T>::ellipsoid_v>;

template <typename T, typename E>
concept GeocentricCoordinate = requires(std::remove_cvref_t<T> a) {
    typename T::value_type;
    { a.x() } -> std::convertible_to<double>;
    { a.y() } -> std::convertible_to<double>;
    { a.z() } -> std::convertible_to<double>;
} && Ellipsoid<E>;

template<typename T>
concept GeocentricLike = requires(std::remove_cvref_t<T> a) {
    typename T::value_type;
    { a.x() } -> std::convertible_to<double>;
    { a.y() } -> std::convertible_to<double>;
    { a.z() } -> std::convertible_to<double>;
} && Ellipsoid<typename std::remove_cvref_t<T>::ellipsoid_v>;
};  // namespace csics::geo
