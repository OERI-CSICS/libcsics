
#pragma once

#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "csics/executor/Concept.hpp"
#include "csics/executor/Executors.hpp"
#include "csics/sim/ecs/Traits.hpp"
#include "csics/sim/ecs/View.hpp"
namespace csics::sim::ecs {

template <typename Systems>
struct Layer;

template <typename... Systems>
struct Layer<std::tuple<Systems...>> {
    using type = std::tuple<Systems...>;

    type systems;
};

template <typename Layers, typename Components, typename Hooks,
          typename Executor = executor::SingleThreadedExecutor>
class StaticWorld;

template <typename Layers, typename Components, typename Hooks>
class StaticWorldBuilder;

template <typename... Layers, Component... Components, typename... Hooks,
          typename Executor>
class StaticWorld<std::tuple<Layers...>, std::tuple<Components...>,
                  std::tuple<Hooks...>, Executor> {
   public:
    Entity add_entity() {
        Entity e;
        if (!dead_entities_.empty()) {
            e.id = dead_entities_.pop_back();
            e.generation = generations_[e.id]++;
        } else {
            e.id = entities_.size();
            e.generation = 0;
            generations_.push_back(0);
        }
        entities_.push_back(e);
        return e;
    }

    template <Component Cc>
    void add_component(const Entity& e, Cc&& component) {
        using C = std::remove_cvref_t<Cc>;
        static_assert(type_in_tuple<std::remove_cvref_t<C>,
                                    std::tuple<Components...>>::value,
                      "Component not registered in the world");
        std::get<SparseSet<C>>(components_)
            .insert(e, std::forward<Cc>(component));
    }

    template <Component Cc, typename... Args>
    void add_component(const Entity& e, Args&&... args) {
        using C = std::remove_cvref_t<Cc>;
        static_assert(type_in_tuple<C, std::tuple<Components...>>::value,
                      "Component not registered in the world");
        std::get<SparseSet<C>>(components_)
            .emplace(e, std::forward<Args>(args)...);
    }

    template <Component Cc>
    auto& get_component(const Entity& e) {
        using C = std::remove_cvref_t<Cc>;
        static_assert(type_in_tuple<C, std::tuple<Components...>>::value,
                      "Component not registered in the world");
        auto* comp = std::get<SparseSet<C>>(components_).at(e);
        CSICS_RUNTIME_ASSERT(comp != nullptr,
                             "Entity does not have the requested component");
        return *comp;
    }

    template <Component Cc>
    const auto& get_component(const Entity& e) const {
        using C = std::remove_cvref_t<Cc>;
        static_assert(type_in_tuple<C, std::tuple<Components...>>::value,
                      "Component not registered in the world");
        auto* comp = std::get<SparseSet<C>>(components_).at(e);
        CSICS_RUNTIME_ASSERT(comp != nullptr,
                             "Entity does not have the requested component");
        return *comp;
    }

    template <Component Cc>
    void remove_component(const Entity& e) {
        using C = std::remove_cvref_t<Cc>;
        static_assert(type_in_tuple<C, std::tuple<Components...>>::value,
                      "Component not registered in the world");
        std::get<SparseSet<C>>(components_).remove(e);
    }

    void remove_entity(const Entity& e) {
        auto it = std::find_if(
            entities_.begin(), entities_.end(),
            [&](const Entity& entity) { return entity.id == e.id; });
        CSICS_RUNTIME_ASSERT(it != entities_.end(),
                             "Entity not found in world entities.");
        dead_entities_.push_back(e.id);
        entities_.erase(it);
        std::apply(
            [&](auto&&... sets) { (sets.remove(e), ...); },
            std::make_tuple(std::get<SparseSet<Components>>(components_)...));
    }

    void run(double dt) {
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            (std::apply(
                 [&](auto&&... systems) {
                     std::array<WorldContext, sizeof...(systems)> context_arr =
                         [&]<std::size_t... WCs>(std::index_sequence<WCs...>)
                             ->std::array<WorldContext, sizeof...(systems)> {
                         return {{(static_cast<void>(WCs),
                                   WorldContext(*this))...}};
                     }
                     (std::make_index_sequence<sizeof...(systems)>{});

                     [&]<size_t... Js>(std::index_sequence<Js...>) {
                         (
                             [&](auto&& system) {
                                 using view_type =
                                     system_traits<std::remove_cvref_t<
                                         decltype(system)>>::view_type;
                                 using components =
                                     view_traits<view_type>::components;

                                 auto v =
                                     [&]<typename... Cs>(std::tuple<Cs...>) {
                                         return view_type(
                                             std::get<SparseSet<
                                                 std::remove_const_t<Cs>>>(
                                                 components_)...);
                                     }(components{});

                                 dispatch_system(system, v, dt,
                                                 context_arr[Js]);
                             }(systems),
                             ...);
                         executor_.join();
                         std::get<Is>(hooks_)(*this);
                         for (const auto& ctx : context_arr) {
                             for (const auto& df_entity :
                                  ctx.deferred_entities) {
                                 for (const auto& comp : df_entity.components) {
                                     std::visit(
                                         [&](const auto& c) {
                                             using C = std::remove_cvref_t<
                                                 std::decay_t<decltype(c)>>;
                                             add_component<C>(df_entity.entity, c);
                                         },
                                         comp);
                                 }
                             }
                             for (const auto& action : ctx.deferred_actions) {
                                 std::visit(
                                     [&](auto& action) {
                                         using ActionType = std::remove_cvref_t<
                                             std::decay_t<decltype(action)>>;
                                         if constexpr (ActionType::type == 0) {
                                             using C = typename ActionType::
                                                 component_type;
                                             add_component<C>(action.e,
                                                              action.component);
                                         } else if constexpr (ActionType::
                                                                  type == 1) {
                                             using C = typename ActionType::
                                                 component_type;
                                             remove_component<C>(action.e);
                                         } else if constexpr (ActionType::
                                                                  type == 2) {
                                             remove_entity(action.e);
                                         }
                                     },
                                     action);
                             }
                         }
                     }(std::make_index_sequence<sizeof...(systems)>{});
                 },
                 std::get<Is>(layers_).systems),
             ...);
        }(std::make_index_sequence<sizeof...(Layers)>{});
    }

    template <typename S, typename V, typename Ctx>
        requires SystemWithView<S> || SystemWithDt<S> ||
                 SystemWithContext<S, Ctx>
    void dispatch_system(S&& system, V v, double dt, Ctx& context) {
        if constexpr (SystemWithContext<std::remove_cvref_t<decltype(system)>, WorldContext>) {
            executor_.submit(system, v, dt, context);
        } else if constexpr (SystemWithDt<std::remove_cvref_t<decltype(system)>>) {
            executor_.submit(system, v, dt);
        } else if constexpr (SystemWithView<std::remove_cvref_t<decltype(system)>>) {
            executor_.submit(system, v);
        }
    }

    StaticWorld(std::tuple<Layers...> layers, std::tuple<Hooks...> hooks,
                Executor executor)
        : layers_(layers),
          hooks_(hooks),
          components_({}),
          entities_(),
          executor_(executor) {}

    class WorldContext {
        StaticWorld& world;

        friend class StaticWorld;

       protected:
        WorldContext(StaticWorld& world) : world(world) {}

        struct DeferredEntity {
            Entity entity;
            Buffer<std::variant<Components...>> components;
        };

        template <typename C>
        struct AddComponent {
            static constexpr int type = 0;
            using component_type = C;
            Entity e;
            C component;
        };
        template <typename C>
        struct RemoveComponent {
            static constexpr int type = 1;
            using component_type = C;
            Entity e;
            std::optional<C> component = std::nullopt;
        };
        struct DestroyEntity {
            static constexpr int type = 2;
            Entity e;
        };

        using DeferredAction =
            std::variant<AddComponent<Components>...,
                         RemoveComponent<Components>..., DestroyEntity>;

        Buffer<DeferredAction> deferred_actions;
        Buffer<DeferredEntity> deferred_entities;

       public:
        Entity add_entity() {
            Entity deferred = world.add_entity();
            deferred_entities.push_back(
                DeferredEntity{.entity = deferred, .components = {}});
            return deferred;
        }

        template <Component C, typename... Args>
        void add_component(const Entity& e, Args&&... args) {
            static_assert(type_in_tuple<std::remove_cvref_t<C>,
                                        std::tuple<Components...>>::value,
                          "Component not registered in the world");
            auto it = std::find_if(
                deferred_entities.begin(), deferred_entities.end(),
                [&](const DeferredEntity& de) { return de.entity.id == e.id; });

            if (it == deferred_entities.end()) {
                auto it2 = std::find_if(
                    world.entities_.begin(), world.entities_.end(),
                    [&](const Entity& entity) { return entity.id == e.id; });
                CSICS_RUNTIME_ASSERT(it2 != world.entities_.end(),
                                     "Entity not found in deferred "
                                     "entities or world entities");
                (void)it2;  // disable warning in release builds
                deferred_actions.push_back(AddComponent<C>{
                    .e = e, .component = C{std::forward<Args>(args)...}});
                return;
            }

            it->components.emplace_back(std::forward<Args>(args)...);
        }

        template <Component C>
        void remove_component(const Entity& e) {
            static_assert(type_in_tuple<std::remove_cvref_t<C>,
                                        std::tuple<Components...>>::value,
                          "Component not registered in the world");
            auto it = std::find_if(
                world.entities_.begin(), world.entities_.end(),
                [&](const Entity& entity) { return entity.id == e.id; });
            CSICS_RUNTIME_ASSERT(it != world.entities_.end(),
                                 "Entity not found in world entities.");
            deferred_actions.push_back(RemoveComponent<C>{.e = e});
        }

        void destroy_entity(const Entity& e) {
            auto it = std::find_if(
                world.entities_.begin(), world.entities_.end(),
                [&](const Entity& entity) { return entity.id == e.id; });
            CSICS_RUNTIME_ASSERT(it != world.entities_.end(),
                                 "Entity not found in world entities.");
            deferred_actions.push_back(DestroyEntity{.e = e});
        }
    };

   protected:
    std::tuple<Layers...> layers_;
    std::tuple<Hooks...> hooks_;
    std::tuple<SparseSet<Components>...> components_;
    Buffer<Entity> entities_;
    Buffer<std::uint32_t> dead_entities_;
    Buffer<std::uint32_t> generations_;
    Executor executor_;

    friend class WorldContext;

    friend class StaticWorldBuilder<
        std::tuple<Layers...>, std::tuple<Components...>, std::tuple<Hooks...>>;
};

template <typename... Layers, Component... Components, typename... Hooks>
class StaticWorldBuilder<std::tuple<Layers...>, std::tuple<Components...>,
                         std::tuple<Hooks...>> {
   public:
    template <typename I, typename Ret, typename... Args>
    struct MemberPointerWrapper {
        using view_type = tuple_head<std::tuple<Args...>>::type;
        MemberPointerWrapper(std::pair<I*, Ret (I::*)(Args...)> member_func)
            : func([member_func = std::move(member_func)](Args&&... args) {
                  return (member_func.first->*member_func.second)(
                      std::forward<Args>(args)...);
              }) {}

        void operator()(Args... args) const {
            func(std::forward<Args>(args)...);
        }

        std::function<void(Args...)> func;
    };
    StaticWorldBuilder() : layers_(), hooks_() {}

    StaticWorldBuilder(std::tuple<Layers...> layers, std::tuple<Hooks...> hooks)
        : layers_(layers), hooks_(hooks) {}

    template <typename... Ss>
    auto add_layer(Ss... systems) {
        return StaticWorldBuilder<
            std::tuple<Layers..., Layer<std::tuple<decltype(make_layer_system(
                                      systems))...>>>,
            std::tuple<Components...>, std::tuple<Hooks...>>(
            std::tuple_cat(
                layers_,
                std::make_tuple(
                    Layer<std::tuple<decltype(make_layer_system(systems))...>>{
                        .systems = {make_layer_system(systems)...}})),
            hooks_);
    }

    template <typename F>
        requires(!requires { typename F::first_type; })  // not a pair
    auto make_layer_system(F&& system) {
        return std::forward<F>(system);
    }
    template <typename I, typename Ret, typename... Args>
    auto make_layer_system(std::pair<I*, Ret (I::*)(Args...)> member_func) {
        return MemberPointerWrapper<I, Ret, Args...>{member_func};
    }
    template <Component C>
    auto add_component() {
        return StaticWorldBuilder<std::tuple<Layers...>,
                                  std::tuple<Components..., C>,
                                  std::tuple<Hooks...>>(layers_, hooks_);
    }

    template <typename... Cs>
    auto add_components() {
        return StaticWorldBuilder<std::tuple<Layers...>,
                                  std::tuple<Components..., Cs...>,
                                  std::tuple<Hooks...>>(layers_, hooks_);
    }

    auto build() { return build(executor::SingleThreadedExecutor{}); }

    template <executor::Executor E = executor::SingleThreadedExecutor>
    auto build(E&& executor) {
        return StaticWorld<std::tuple<Layers...>, std::tuple<Components...>,
                           decltype(make_padded_hook()), E>(
            layers_, make_padded_hook(), std::forward<E>(executor));
    }

    template <typename F>
    auto add_hook(F&& hook) {
        static_assert(sizeof...(Hooks) <= sizeof...(Layers),
                      "Cannot add more hooks than layers");
        static_assert(sizeof...(Layers) > 0,
                      "Must add at least one layer before adding hooks");
        return StaticWorldBuilder<
            std::tuple<Layers...>, std::tuple<Components...>,
            decltype(make_padded_hook(std::declval<F>()))>(
            layers_, make_padded_hook(std::forward<F>(hook)));
    }

   private:
    std::tuple<Layers...> layers_;
    std::tuple<Hooks...> hooks_;

    using components = std::tuple<Components...>;

    constexpr static size_t required_padding =
        sizeof...(Layers) - sizeof...(Hooks) - 1;
    static_assert(sizeof...(Hooks) <= sizeof...(Layers),
                  "Number of hooks cannot exceed number of layers - 1");

    template <std::size_t N>
    static auto make_empty_hooks_tuple() {
        return []<size_t... Is>(std::index_sequence<Is...>) {
            return std::make_tuple(([](auto&) { (void)Is; })...);
        }(std::make_index_sequence<N>{});
    }

    auto make_padded_hook() {
        return std::tuple_cat(hooks_,
                              make_empty_hooks_tuple<required_padding + 1>());
    }

    template <typename F>
    auto make_padded_hook(F&& hook) {
        return std::tuple_cat(hooks_,
                              make_empty_hooks_tuple<required_padding>(),
                              std::make_tuple(std::forward<F>(hook)));
    }
};

StaticWorldBuilder()
    -> StaticWorldBuilder<std::tuple<>, std::tuple<>, std::tuple<>>;

// holy moly

};  // namespace csics::sim::ecs
