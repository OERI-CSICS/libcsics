#pragma once

#include <cstdint>
namespace csics::lvc::dis {
    constexpr std::uint16_t ALL_APPLIC = 0xFFFF;
    constexpr std::uint16_t ALL_SITES = 0xFFFF;
    constexpr std::uint16_t ALL_ENTITIES = 0xFFFF;
};

#include "csics/lvc/dis/PDUs.hpp"
#include "csics/lvc/dis/Time.hpp"
#include "csics/lvc/dis/serde.hpp"
