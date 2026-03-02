#include <gtest/gtest.h>

#include <concepts>
#include <csics/csics.hpp>

#include "csics/sim/ecs/World.hpp"

using namespace csics::sim::ecs;
struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};

struct System1 {
    using view_type = csics::sim::ecs::View<const Position, const Velocity>;
    template <typename Ctx>
    void operator()(view_type v, double dt, Ctx&) {
        for (auto [entity, pos, vel] : v) {
            // just a dummy system that does nothing
            (void)entity;
            (void)pos;
            (void)vel;
            (void)dt;
        }
    }
};

struct MemberFunctionSystem {
    void system_func(csics::sim::ecs::View<const Position> v) {
        for (auto [entity, pos] : v) {
            (void)entity;
            (void)pos;
        }
    }

    void system_func_with_dt(csics::sim::ecs::View<const Position> v,
                             double dt) {
        for (auto [entity, pos] : v) {
            (void)entity;
            (void)pos;
            (void)dt;
        }
    }

    template <typename Ctx>
    void system_func_with_ctx(csics::sim::ecs::View<const Position> v,
                              double dt, Ctx& ctx) {
        for (auto [entity, pos] : v) {
            (void)entity;
            (void)pos;
            (void)dt;
            (void)ctx;
        }
    };
};

static_assert(std::same_as<System1::view_type,
                           csics::sim::ecs::system_traits<System1>::view_type>);

static_assert(csics::sim::ecs::SystemWithContext<System1, int>);

void sys2(csics::sim::ecs::View<Position, const Velocity> v, double dt) {
    for (auto [entity, pos, vel] : v) {
        pos.x += vel.dx * dt;
        pos.y += vel.dy * dt;
    }
}

void sys3(csics::sim::ecs::View<const Position> v) {
    for (auto [entity, pos] : v) {
        (void)entity;
        (void)pos;
    }
}

template <typename Ctx>
void sys4(csics::sim::ecs::View<Velocity> v, double dt, Ctx&) {
    for (auto [entity, vel] : v) {
        (void)entity;
        (void)vel;
        (void)dt;
    }
}

TEST(CSICSSimTests, ECSBasicTest) {
    using namespace csics::sim::ecs;
    MemberFunctionSystem member_sys;
    auto world =
        StaticWorldBuilder()
            .add_layer(System1{})
            .add_hook(
                [](auto&) { std::cerr << "Finished running sys1" << std::endl; })
            .add_layer(sys2)
            .add_layer(sys3)
            .add_layer(
                std::pair{&member_sys, &MemberFunctionSystem::system_func})
            .add_layer(std::pair{&member_sys,
                                 &MemberFunctionSystem::system_func_with_dt},
                       sys3)
            .add_component<Position>()
            .add_component<Velocity>()
            .build();

    auto e1 = world.add_entity();
    auto e2 = world.add_entity();

    world.add_component<Position>(e1, {0, 0});
    world.add_component<Velocity>(e1, {1, 1});

    world.add_component<Position>(e2, {10, 10});
    world.add_component<Velocity>(e2, {-1, -1});

    world.run(1.0);

    auto& pos1 = world.get_component<Position>(e1);
    auto& pos2 = world.get_component<Position>(e2);

    EXPECT_FLOAT_EQ(pos1.x, 1);
    EXPECT_FLOAT_EQ(pos1.y, 1);
    EXPECT_FLOAT_EQ(pos2.x, 9);
    EXPECT_FLOAT_EQ(pos2.y, 9);
}
