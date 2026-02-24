
#pragma once

#include "csics/Bit.hpp"
#ifndef CSICS_BUILD_SERIALIZATION
#error \
    "Serialization support is not enabled. Please define CSICS_BUILD_SERIALIZATION to use serialization."
#endif
#include "csics/Buffer.hpp"
#include "csics/serialization/Common.hpp"
#include "csics/serialization/Concepts.hpp"

namespace csics::serialization {

// TODO: Replace std::string_view with a more efficient key representation
// or at least with something not from the standard library to avoid ABI issues.

struct serializer {
    template <Serializer S, typename T>
    SerializationResult operator()(S& s, MutableBufferView bv, T&& obj) const {
        return apply(s, bv, std::forward<T>(obj));
    }

   private:
    template <StructuralSerializer S, typename T>
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

    template <StructuralSerializer S, typename T>
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

    template <StructuralSerializer S, typename T>
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

    template <StructuralSerializer S, typename T>
        requires PrimitiveSerializable<std::remove_cvref_t<T>, S> &&
                 (!MapSerializable<std::remove_cvref_t<T>, S>)
    constexpr static SerializationResult apply(S& s, MutableBufferView& bv,
                                               T&& value) {
        auto bv_ = bv;
        auto status = s.value(bv_, value);
        return {bv(0, bv.size() - bv_.size()), status};
    }

    template <StructuralSerializer S, typename T, std::endian Endian>
        requires PrimitiveSerializable<std::remove_cvref_t<T>, S> &&
                 (!MapSerializable<std::remove_cvref_t<T>, S>)
    constexpr static SerializationResult apply(S& s, MutableBufferView& bv,
                                               endian<T, Endian>&& value) {
        auto bv_ = bv;
        auto status = s.value(bv_, value.repr_);
        return {bv(0, bv.size() - bv_.size()), status};
    }

    template <WireSerializer S, typename T>
    constexpr static SerializationResult apply(S& s, MutableBufferView& bv,
                                               T&& value) {
        auto bv_ = bv;
        auto status = serialize_wire(s, bv_, value);
        return {bv(0, bv_.size() - bv_.size()), status};
    }
};

template <WireSerializer S, typename T>
constexpr SerializationResult serialize_wire(S&, MutableBufferView& bv,
                                             T&&) {
    static_assert(
        [] { return false; }(),
        "No wire serialization implementation found. serialize_wire must be "
        "specialized within the csics::serialization namespace.");
    return {bv, SerializationStatus::Ok}; // Dummy return to satisfy compiler, should never be used
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
