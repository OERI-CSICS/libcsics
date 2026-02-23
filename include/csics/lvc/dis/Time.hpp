#pragma once

#include "csics/Bit.hpp"
#include <cstdint>
namespace csics::lvc::dis {
class DISTimestamp {
   public:
    DISTimestamp() : timestamp_(0) {}
    constexpr explicit DISTimestamp(std::uint32_t timestamp) : timestamp_(timestamp) {}

    constexpr std::uint32_t raw() const { return timestamp_; }

    constexpr bool absolute() const {
        return (timestamp_ & 0x1) == 0 ;
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
