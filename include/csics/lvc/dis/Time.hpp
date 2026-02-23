#pragma once

#include <csics/Bit.hpp>
#include <cstdint>
namespace csics::lvc::dis {
class DISTimestamp {
   public:
    DISTimestamp() : timestamp_(0) {}
    explicit DISTimestamp(std::uint32_t timestamp) : timestamp_(timestamp) {}

    std::uint32_t raw() const { return timestamp_; }

    constexpr bool absolute() const {
        // the least significant bit of the big endian representation of the
        // timestamp indicates whether it's absolute or relative
        if constexpr (std::endian::native == std::endian::little) {
            return (timestamp_ & 0x01000000) != 0;
        } else {
            return (timestamp_ & 0x00000001) != 0;
        }
    }

    constexpr bool relative() const { return !absolute(); }

    bool operator==(const DISTimestamp& other) const {
        return timestamp_ == other.timestamp_;
    }

    bool operator!=(const DISTimestamp& other) const {
        return !(*this == other);
    }

   private:
    std::uint32_t timestamp_;
};
};  // namespace csics::lvc::dis
