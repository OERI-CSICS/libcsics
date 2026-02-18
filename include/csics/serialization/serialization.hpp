#pragma once

// #include <csics/io/Buffer.hpp>
#include <concepts>
#include <string_view>
#include <tuple>

#include "../Buffer.hpp"

namespace csics::serialization {

// TODO: Replace std::string_view with a more efficient key representation
// or at least with something not from the standard library to avoid ABI issues.

enum class SerializationStatus {
    Ok,
    BufferFull,
};

struct SerializationResult {
    io::BufferView written_view;
    SerializationStatus status;
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
concept Serializer = requires(S s, io::BufferView bv, std::string_view key) {
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

template <typename T>
concept StructSerializableImpl = requires {
    { T::fields() } -> FieldList;
};

template <typename T>
concept StructSerializable = StructSerializableImpl<std::remove_cvref_t<T>>;

template <StructSerializable T>
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
                                                   // which has data() and
                                                   // size() but is not an array

template <typename T>
concept ArraySerializable = ArraySerializableImpl<std::remove_cvref_t<T>>;

template <typename T, typename S>
concept PrimitiveSerializable = requires(T t, S s, io::BufferView bv) {
    { s.value(bv, t) } -> std::same_as<SerializationStatus>;
} && !StructSerializable<T> && !ArraySerializable<T>;

template <typename T, typename S>
concept Serializable = requires(T t, S s, io::BufferView bv) {
    { t.serialize(s, bv) } -> std::same_as<SerializationStatus>;
};

struct serializer {
    template <Serializer S, typename T>
        requires StructSerializable<std::remove_cvref_t<T>>
    SerializationResult operator()(S& s, io::BufferView& bv, T&& obj) const {
        auto fields = get_fields<T>();
        auto bv_ = bv;
        s.begin_obj(bv_);
        std::apply(
            [&](auto&&... field) {
                (...,
                 (s.key(bv_, field.name()),
                  bv_ += this->operator()(
                      s, bv_, obj.*(field.ptr())).written_view.size()));  // Serialize each field
            },
            fields);
        s.end_obj(bv_);
        return {bv(0, bv.size() - bv_.size()), SerializationStatus::Ok};
    }

    template <Serializer S, typename T>
        requires ArraySerializable<std::remove_cvref_t<T>>
    SerializationResult operator()(S& s, io::BufferView& bv, T&& arr) const {
        auto bv_ = bv;
        s.begin_array(bv_);
        for (std::size_t i = 0; i < arr.size(); ++i) {
            auto res = this->operator()(s, bv_, arr.data()[i]);
            if (res.status != SerializationStatus::Ok) {
                return {bv(0, bv.size() - bv_.size()), res.status};
            }
            bv_ += res.written_view.size();
        }
        s.end_array(bv_);
        return {bv(0, bv.size() - bv_.size()), SerializationStatus::Ok};
    }

    template <Serializer S, typename T>
        requires PrimitiveSerializable<std::remove_cvref_t<T>, S>
    SerializationResult operator()(S& s, io::BufferView& bv, T&& value) const {
        auto bv_ = bv;
        auto status = s.value(bv_, value);
        return {bv(0, bv.size() - bv_.size()), status};
    }

    template <Serializer S, typename T>
    SerializationStatus operator()(S&, io::BufferView&, T&&) const {
        static_assert([] { return false; }(), "Type is not serializable");
        return SerializationStatus::Ok;  // Unreachable, but satisfies return
                                         // type
    }

    template <Serializer S, typename T>
    SerializationStatus operator()(io::BufferView&, T&&) const {
        static_assert([] { return false; }(), "Type is not serializable");
        return SerializationStatus::Ok;  // Unreachable, but satisfies return
                                         // type
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
