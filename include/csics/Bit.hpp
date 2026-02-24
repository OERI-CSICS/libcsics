
#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>
namespace csics {
namespace detail {
template <typename T>
constexpr T bswap_fallback(T v) noexcept {
    static_assert(std::is_integral_v<T>, "byteswap requires integral type");
    static_assert(!std::is_same_v<T, bool>, "byteswap not valid for bool");

    using U = std::make_unsigned_t<T>;
    U x = static_cast<U>(v);

    if constexpr (sizeof(T) == 1) {
        return v;
    } else if constexpr (sizeof(T) == 2) {
        x = (x >> 8) | (x << 8);
    } else if constexpr (sizeof(T) == 4) {
        x = (x >> 24) | ((x >> 8) & 0x0000FF00u) | ((x << 8) & 0x00FF0000u) |
            (x << 24);
    } else if constexpr (sizeof(T) == 8) {
        x = (x >> 56) | ((x >> 40) & 0x000000000000FF00ull) |
            ((x >> 24) & 0x0000000000FF0000ull) |
            ((x >> 8) & 0x00000000FF000000ull) |
            ((x << 8) & 0x000000FF00000000ull) |
            ((x << 24) & 0x0000FF0000000000ull) |
            ((x << 40) & 0x00FF000000000000ull) | (x << 56);
    } else {
        static_assert(sizeof(T) <= 8, "Unsupported integer size");
    }

    return static_cast<T>(x);
}
}  // namespace detail

template <typename T>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
constexpr T byteswap(T v) noexcept {
    static_assert(!std::is_same_v<T, bool>, "byteswap not valid for bool");
    // used for bitcasts into the correct size unsigned integer type for byte swapping
    using U = std::conditional_t<
        sizeof(T) == 1, std::uint8_t,
        std::conditional_t<
            sizeof(T) == 2, std::uint16_t,
            std::conditional_t<
                sizeof(T) == 4, std::uint32_t,
                std::conditional_t<sizeof(T) == 8, std::uint64_t, void>>>>;

    static_assert(!std::is_same_v<U, void>,
                  "Unsupported type size for byteswap");

    U x = std::bit_cast<U>(v);
#if defined(__cpp_lib_byteswap)
    return std::byteswap(x);
#elif defined(__clang__) || defined(__GNUC__)
    if constexpr (sizeof(T) == 2)
        return __builtin_bswap16(x);
    else if constexpr (sizeof(T) == 4)
        return __builtin_bswap32(x);
    else if constexpr (sizeof(T) == 8)
        return __builtin_bswap64(x);
    else
        return x;  // 1 byte
#elif defined(_MSC_VER)
    if constexpr (sizeof(T) == 2)
        return _byteswap_ushort(x);
    else if constexpr (sizeof(T) == 4)
        return _byteswap_ulong(x);
    else if constexpr (sizeof(T) == 8)
        return _byteswap_uint64(x);
    else
        return x;
#else
    return detail::bswap_fallback(x);
#endif
}

template <typename T>
    requires std::is_integral_v<T>
constexpr T hton(T val) {
    if constexpr (std::endian::native == std::endian::little) {
        return byteswap(val);
    } else {
        return val;
    }
}

template <typename T, std::endian Endian>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
struct endian {
    using value_type = T;
    static constexpr std::endian endian_v = Endian;
    T repr_;

    template <std::endian E>
    constexpr T to() const {
        if constexpr (E == Endian) {
            return repr_;
        } else {
            return byteswap(repr_);
        }
    }

    constexpr T native() const { return to<std::endian::native>(); }
    constexpr T big() const { return to<std::endian::big>(); }
    constexpr T little() const { return to<std::endian::little>(); }
    constexpr T network() const { return to<std::endian::big>(); }

    template <std::endian E>
    constexpr void from(T value) {
        if constexpr (E == Endian) {
            repr_ = value;
        } else {
            repr_ = byteswap(value);
        }
    }

    constexpr void from_native(T value) { from<std::endian::native>(value); }
    constexpr void from_big(T value) { from<std::endian::big>(value); }
    constexpr void from_little(T value) { from<std::endian::little>(value); }
    constexpr void from_network(T value) { from<std::endian::big>(value); }

    endian() = default;
    // Assuming the input value is in native endianness.
    constexpr explicit endian(T value) { from_native(value); }
};

template <typename T>
using be = endian<T, std::endian::big>;

template <typename T>
using le = endian<T, std::endian::little>;

template <typename T>
using ne = endian<T, std::endian::native>;

template <typename T>
struct is_endian_wrapper : std::false_type {};

template <typename T, std::endian E>
struct is_endian_wrapper<endian<T, E>> : std::true_type {};

};  // namespace csics
