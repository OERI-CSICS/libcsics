
#pragma once

#include <concepts>
namespace csics::sim::ecs {

    template <typename C>
        concept Component = std::is_trivially_copyable_v<C>;
};
