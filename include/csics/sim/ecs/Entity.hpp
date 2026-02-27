#pragma once

#include <cstdint>
#include <ostream>
#include "csics/lvc/dis/PDUs.hpp"

namespace csics::sim::ecs {
struct Entity {
    uint32_t id;
    uint32_t generation;

    Entity() {
        id = std::numeric_limits<uint32_t>::max();
        generation = std::numeric_limits<uint32_t>::max();
    }
    Entity(const Entity& other) = default;
    Entity& operator=(const Entity& other) = default;
    explicit Entity(std::uint64_t id);
    Entity(std::uint8_t generation, std::uint32_t id);
    Entity(const uint8_t generation, const csics::lvc::dis::EntityID& disEntityID);

    bool valid() const;

    bool operator==(const Entity& other) const;

    bool operator!=(const Entity& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Entity& entity);

};
};

