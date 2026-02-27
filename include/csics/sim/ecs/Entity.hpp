#pragma once

#include <cstdint>
#include <ostream>
#include "csics/lvc/dis/PDUs.hpp"

namespace csics::sim::ecs {
struct Entity {
    uint32_t m_ID;
    uint32_t m_Generation;

    Entity() {
        m_ID = std::numeric_limits<uint32_t>::max();
        m_Generation = std::numeric_limits<uint32_t>::max();
    }
    Entity(const Entity& other) = default;
    Entity& operator=(const Entity& other) = default;
    explicit Entity(std::uint64_t id);
    Entity(std::uint8_t generation, std::uint32_t id);
    Entity(const uint8_t generation, const csics::lvc::dis::EntityID& disEntityID);

    uint32_t id() const;
    uint32_t generation() const;
    bool valid() const;

    bool operator==(const Entity& other) const;

    bool operator!=(const Entity& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Entity& entity);

};
};

