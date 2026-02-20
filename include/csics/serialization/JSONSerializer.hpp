
#pragma once

#include <csics/serialization/Serialization.hpp>
#include <memory>
#include <optional>

namespace csics::serialization {

template <typename T>
concept JSONIsNull =
    std::same_as<T, std::nullptr_t> || std::same_as<T, std::nullopt_t>;

class JSONSerializer {
   public:
    JSONSerializer();
    ~JSONSerializer();

    SerializationStatus begin_obj(BufferView& bv);
    SerializationStatus end_obj(BufferView& bv);
    SerializationStatus begin_array(BufferView& bv);
    SerializationStatus end_array(BufferView& bv);
    SerializationStatus key(BufferView& bv, std::string_view key);
    template <typename T>
    SerializationStatus value(BufferView& bv, T&& value) {
        using D = std::decay_t<T>;

        if constexpr (std::is_same_v<D, bool>) {
            return write_bool(bv, value);
        } else if constexpr (std::is_integral_v<D>) {
            return write_int(bv, static_cast<std::int64_t>(value));
        } else if constexpr (std::is_floating_point_v<D>) {
            return write_number(bv, static_cast<double>(value));
        } else if constexpr (std::convertible_to<D, std::string_view>) {
            return write_string(bv, std::string_view(value));
        } else if constexpr (JSONIsNull<D>) {
            return write_null(bv);
        } else {
            static_assert([] { return false; }(), "Unsupported type for value");
            return SerializationStatus::Ok;  // Unreachable, but satisfies return type
        }
    }

    static constexpr std::size_t key_overhead() {
        return 4;  // For quotes around the key, colon, and comma
    }
    static constexpr std::size_t obj_overhead() {
        return 2;  // For '{' and '}'
    }
    static constexpr std::size_t array_overhead() {
        return 2;  // For '[' and ']'
    }
    static constexpr std::size_t meta_overhead() {
        return 2;  // for '{' and '}' for entire object
    }

    template <typename T>
    static constexpr std::size_t value_overhead() {
        if constexpr (std::is_same_v<std::decay_t<T>, int>) {
            return 11;  // Max length of int in JSON (including sign)
        } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
            return 24;  // Approximate max length of double in JSON
        } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
            return 5;  // "true" or "false"
        } else if constexpr (std::is_same_v<std::decay_t<T>,
                                            std::string_view>) {
            return 2;  // Quotes around the string
        } else {
            static_assert(sizeof(T) == 0,
                          "Unsupported type for value_overhead");
            return 0;  // Unreachable, but satisfies return type
        }
    }

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    SerializationStatus write_string(BufferView& bv, std::string_view str);
    SerializationStatus write_number(BufferView& bv, double num);
    SerializationStatus write_bool(BufferView& bv, bool value);
    SerializationStatus write_null(BufferView& bv);
    SerializationStatus write_int(BufferView& bv, std::int64_t num);
};

};  // namespace csics::serialization
