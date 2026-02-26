#pragma once

#include "csics/Bit.hpp"
#include "csics/Types.hpp"
#include "csics/serialization/Common.hpp"
#include "csics/serialization/Concepts.hpp"
#include "csics/serialization/Deserializer.hpp"
namespace csics::serialization {
class DirectSerializer {
   public:
    DirectSerializer() = default;

    template <typename T>
        requires std::is_trivially_copyable_v<std::remove_cvref_t<T>>
    SerializationStatus write(MutableBufferView& bv, T&& t) {
        // direct byte-wise copying of the object into the buffer.
        if (sizeof(T) > bv.size()) {
            std::cerr << "Buffer too small to write type of size " << sizeof(T)
                      << " bytes, only " << bv.size() << " bytes left\n";
            return SerializationStatus::BufferFull;
        }
        std::memcpy(bv.data(), &t, sizeof(T));
        bv += sizeof(T);
        return SerializationStatus::Ok;
    };

    template <typename T, std::endian E>
    SerializationStatus write(MutableBufferView& bv, endian<T, E>&& t) {
        return write(bv, t.repr_);
    };

    SerializationStatus pad(MutableBufferView& bv, std::size_t padding_bytes) {
        if (padding_bytes > bv.size()) {
            return SerializationStatus::BufferFull;
        }
        std::memset(bv.data(), 0, padding_bytes);
        bv += padding_bytes;
        return SerializationStatus::Ok;
    };
};

class DirectDeserializer {
   public:
    using error_type = DeserializationStatus;
    DirectDeserializer(BufferView bv) : bv_(bv) {}

    template <typename T>
        requires is_endian_wrapper<T>::value ||
                 std::is_trivially_copyable_v<std::remove_cvref_t<T>>
    expected<T, error_type> read() {
        if constexpr (is_endian_wrapper<T>::value) {
            using UnderlyingT = typename T::value_type;
            if (sizeof(UnderlyingT) > bv_.size()) {
                return unexpected(DeserializationStatus::BufferEmpty);
            }
            UnderlyingT repr;
            std::memcpy(&repr, bv_.data(), sizeof(UnderlyingT));
            bv_ += sizeof(UnderlyingT);
            T value;
            value.repr_ = repr;
            return expected<T, DeserializationStatus>(value);
        } else if constexpr (std::is_trivially_copyable_v<
                                 std::remove_cvref_t<T>>) {
            if (sizeof(T) > bv_.size()) {
                return unexpected(DeserializationStatus::BufferEmpty);
            }
            T value;
            std::memcpy(&value, bv_.data(), sizeof(T));
            bv_ += sizeof(T);
            return expected<T, DeserializationStatus>(value);
        }
    }

   private:
    BufferView bv_;
};
};  // namespace csics::serialization
