#include <gtest/gtest.h>

#include <csics/csics.hpp>

struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};

void sys1(csics::sim::ecs::View<const Position, const Velocity> v, double dt) {
    for (auto [entity, pos, vel] : v) {
        // just a dummy system that does nothing
        (void)entity;
        (void)pos;
        (void)vel;
        (void)dt;
    }
}

void sys2(csics::sim::ecs::View<Position, const Velocity> v, double dt) {
    for (auto [entity, pos, vel] : v) {
        pos.x += vel.dx * dt;
        pos.y += vel.dy * dt;
    }
}

TEST(CSICSSimTests, ECSBasicTest) {
    using namespace csics::sim::ecs;
    auto world = StaticWorldBuilder()
                     .add_layer(sys1)
                     .add_hook([]() {
                         std::cerr << "Finished running sys1" << std::endl;
                     })
                     .add_layer(sys2)
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
