
#pragma once

#include "csics/Bit.hpp"
#ifndef CSICS_BUILD_SERIALIZATION
#error \
    "Serialization support is not enabled. Please define CSICS_BUILD_SERIALIZATION to use serialization."
#endif
#include <concepts>
#include <csics/Buffer.hpp>
#include <string_view>
#include <tuple>

namespace csics::serialization {

// TODO: Replace std::string_view with a more efficient key representation
// or at least with something not from the standard library to avoid ABI issues.

enum class SerializationStatus {
    Ok,
    BufferFull,
};

struct SerializationResult {
    MutableBufferView written_view;
    SerializationStatus status;

    constexpr SerializationResult(MutableBufferView written_view,
                                  SerializationStatus status)
        : written_view(written_view), status(status) {}
};

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
     convertible_tup<typename S::convertible_primitives, std::remove_cvref_t<T>>());

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

struct serializer {
    template <Serializer S, typename T>
    SerializationResult operator()(S& s, MutableBufferView bv, T&& obj) const {
        return apply(s, bv, std::forward<T>(obj));
    }

   private:
    template <Serializer S, typename T>
        requires StructSerializable<std::remove_cvref_t<T>, S>
    static constexpr SerializationResult apply(S& s, MutableBufferView& bv,
                                               T&& obj) {
        auto fields = get_fields<T>();
        auto bv_ = bv;
        s.begin_obj(bv_);
        std::apply(
            [&](auto&&... field) {
                (...,
                 (s.key(bv_, field.name()),
                  bv_ += apply(s, bv_, obj.*(field.ptr()))
                             .written_view.size()));  // Serialize each field
            },
            fields);
        s.end_obj(bv_);
        return {bv(0, bv.size() - bv_.size()), SerializationStatus::Ok};
    }

    template <Serializer S, typename T>
        requires IterableSerializable<std::remove_cvref_t<T>, S>
    static constexpr SerializationResult apply(S& s, MutableBufferView& bv,
                                               T&& iterable) {
        auto bv_ = bv;
        s.begin_array(bv_);
        for (const auto& item : iterable) {
            auto res = apply(s, bv_, item);
            if (res.status != SerializationStatus::Ok) {
                return {bv(0, bv.size() - bv_.size()), res.status};
            }
            bv_ += res.written_view.size();
        }
        s.end_array(bv_);
        return {bv(0, bv.size() - bv_.size()), SerializationStatus::Ok};
    };

    template <Serializer S, typename T>
        requires MapSerializable<std::remove_cvref_t<T>, S>
    static constexpr SerializationResult apply(S& s, MutableBufferView& bv,
                                               T&& map) {
        auto bv_ = bv;
        s.begin_obj(bv_);
        for (const auto& [key, value] : map) {
            s.key(bv_, key);
            auto res = apply(s, bv_, value);
            if (res.status != SerializationStatus::Ok) {
                return {bv(0, bv.size() - bv_.size()), res.status};
            }
            bv_ += res.written_view.size();
        }
        s.end_obj(bv_);
        return {bv(0, bv.size() - bv_.size()), SerializationStatus::Ok};
    };

    template <Serializer S, typename T>
        requires PrimitiveSerializable<std::remove_cvref_t<T>, S> &&
                 (!MapSerializable<std::remove_cvref_t<T>, S>)
    constexpr static SerializationResult apply(S& s, MutableBufferView& bv,
                                               T&& value) {
        auto bv_ = bv;
        auto status = s.value(bv_, value);
        return {bv(0, bv.size() - bv_.size()), status};
    }

    template <Serializer S, typename T, std::endian Endian>
        requires PrimitiveSerializable<std::remove_cvref_t<T>, S> &&
                 (!MapSerializable<std::remove_cvref_t<T>, S>)
    constexpr static SerializationResult apply(S& s, MutableBufferView& bv,
                                               endian<T, Endian>&& value) {
        auto bv_ = bv;
        auto status = s.value(bv_, value.repr_);
        return {bv(0, bv.size() - bv_.size()), status};
    }
};

inline constexpr serializer serialize{};

template <typename T, typename Member>
struct SerializableField {
    using name_type = std::string_view;
    using value_type = Member;
    using class_type = T;

    std::string_view name_;
    value_type class_type::* ptr_;

    constexpr auto name() const noexcept { return name_; }
    constexpr value_type class_type::* ptr() const noexcept { return ptr_; }
};

template <typename T, typename Member>
consteval Field auto make_field(std::string_view name, Member T::* ptr) {
    return SerializableField<T, Member>{name, ptr};
};

template <typename... Fields>
consteval FieldList auto make_fields(Fields... field) {
    return std::tuple{field...};
};

}  // namespace csics::serialization
