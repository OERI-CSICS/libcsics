
#pragma once
#include <concepts>
#include <utility>
#include <string_view>
#include <csics/serialization/Common.hpp>

namespace csics::serialization {
template <typename T>
concept Field = requires(T t) {
    typename T::name_type;
    typename T::value_type;
    typename T::class_type;
    { t.name() } -> std::convertible_to<std::string_view>;
    { t.ptr() } -> std::same_as<typename T::value_type T::class_type::*>;
};

template <typename T>
concept FieldList = requires { std::tuple_size<T>::value; } &&
                    []<std::size_t... Is>(
                        std::index_sequence<Is...>) {  // ensure all elements in
                                                       // the tuple are Fields
                        return (Field<std::tuple_element_t<Is, T>> && ...);
                    }(std::make_index_sequence<std::tuple_size<T>::value>{});

template <typename S>
concept Serializer = requires(S s, MutableBufferView bv, std::string_view key) {
    typename S::exact_primitives;
    typename S::convertible_primitives;
    std::tuple_size_v<typename S::exact_primitives> > 0;
    { s.begin_obj(bv) } -> std::same_as<SerializationStatus>;
    { s.end_obj(bv) } -> std::same_as<SerializationStatus>;
    { s.begin_array(bv) } -> std::same_as<SerializationStatus>;
    { s.end_array(bv) } -> std::same_as<SerializationStatus>;
    { s.key(bv, key) } -> std::same_as<SerializationStatus>;
    { S::key_overhead() } -> std::convertible_to<std::size_t>;
    { S::obj_overhead() } -> std::convertible_to<std::size_t>;
    { S::array_overhead() } -> std::convertible_to<std::size_t>;
    { S::meta_overhead() } -> std::convertible_to<std::size_t>;
    { S::template value_overhead<int>() } -> std::convertible_to<std::size_t>;
    {
        S::template value_overhead<double>()
    } -> std::convertible_to<std::size_t>;
    { S::template value_overhead<bool>() } -> std::convertible_to<std::size_t>;
    {
        S::template value_overhead<std::string_view>()
    } -> std::convertible_to<std::size_t>;
    { s.value(bv, int{}) } -> std::same_as<SerializationStatus>;
    { s.value(bv, double{}) } -> std::same_as<SerializationStatus>;
    { s.value(bv, bool{}) } -> std::same_as<SerializationStatus>;
    { s.value(bv, std::string_view{}) } -> std::same_as<SerializationStatus>;
} && std::default_initializable<S>;

template <typename Tup, typename T, size_t... Is>
constexpr bool in_tuple(std::index_sequence<Is...>) {
    return ((std::same_as<T, std::tuple_element_t<Is, Tup>>) || ...);
}

template <typename Tup, typename T, size_t N = std::tuple_size_v<Tup>>
constexpr bool in_tuple() {
    return in_tuple<Tup, T>(std::make_index_sequence<N>{});
}

template <typename Tup, typename T, size_t... Is>
constexpr bool convertible_tup(std::index_sequence<Is...>) {
    return ((std::convertible_to<T, std::tuple_element_t<Is, Tup>>) || ...);
}

template <typename Tup, typename T, size_t N = std::tuple_size_v<Tup>>
constexpr bool convertible_tup() {
    return convertible_tup<Tup, T>(std::make_index_sequence<N>{});
}

template <typename T, typename S>
concept PrimitiveSerializable =
    (std::tuple_size_v<typename S::exact_primitives> > 0) &&
    (in_tuple<typename S::exact_primitives, std::remove_cvref_t<T>>() ||
     convertible_tup<typename S::convertible_primitives,
                     std::remove_cvref_t<T>>());

template <typename T>
concept StructSerializableImpl = requires {
    { T::fields() } -> FieldList;
};

template <typename T, typename S>
concept StructSerializable = StructSerializableImpl<std::remove_cvref_t<T>> &&
                             !PrimitiveSerializable<std::remove_cvref_t<T>, S>;

template <typename T>
    requires StructSerializableImpl<std::remove_cvref_t<T>>
constexpr auto get_fields() {
    using CleanT = std::remove_cvref_t<T>;
    return CleanT::fields();
}

template <typename T>
concept ArraySerializableImpl = requires(T t) {
    typename T::value_type;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.data() } -> std::same_as<typename T::value_type*>;
} && !std::is_convertible_v<T, std::string_view>;  // Exclude std::string_view

template <typename T>
concept MapLike = requires(T t) {
    typename T::key_type;
    typename T::mapped_type;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.begin() } -> std::input_iterator;
    { t.end() } -> std::input_iterator;
};

template <typename T, typename S>
concept IterableSerializable =
    std::ranges::range<T> && !MapLike<T> && !StructSerializable<T, S> &&
    !PrimitiveSerializable<T, S> &&
    (StructSerializable<std::remove_cvref_t<std::ranges::range_reference_t<T>>,
                        S> ||
     PrimitiveSerializable<
         std::remove_cvref_t<std::ranges::range_reference_t<T>>, S> ||
     std::ranges::range<std::ranges::range_reference_t<T>>);

template <typename M, typename S>
concept MapSerializable =
    MapLike<M> &&
    std::is_convertible_v<typename M::key_type,
                          std::string_view> &&  // Ensure keys are string-like
    !StructSerializable<std::remove_cvref_t<M>, S> &&
    !PrimitiveSerializable<std::remove_cvref_t<M>, S> &&
    (StructSerializable<typename M::mapped_type, S> ||
     IterableSerializable<typename M::mapped_type, S> ||
     PrimitiveSerializable<typename M::mapped_type, S>);
};  // namespace csics::serialization
