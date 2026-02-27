
#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include "csics/executor/Concept.hpp"
#include "csics/executor/Executors.hpp"
#include "csics/sim/ecs/View.hpp"
namespace csics::sim::ecs {

template <typename F>
struct callable_traits;

template <typename F>
concept CallableClass = requires {
    { &F::operator() } -> std::same_as<void>;
};
template <typename F>
    requires CallableClass<F>
struct callable_traits<F> : callable_traits<decltype(&F::operator())> {};

template <typename Ret, typename... Args>
struct callable_traits<Ret (*)(Args...)> {
    using return_type = Ret;
    using args = std::tuple<Args...>;
};

template <typename Ret, typename... Args>
struct callable_traits<Ret (*&)(Args...)> {
    using return_type = Ret;
    using args = std::tuple<Args...>;
};

template <typename Ret, typename... Args>
struct callable_traits<Ret(Args...)> {
    using return_type = Ret;
    using args = std::tuple<Args...>;
};

template <typename F, typename Ret, typename... Args>
struct callable_traits<Ret (F::*)(Args...)> {
    using return_type = Ret;
    using args = std::tuple<Args...>;
};

template <typename F, typename Ret, typename... Args>
struct callable_traits<Ret (F::*)(Args...) const> {
    using return_type = Ret;
    using args = std::tuple<Args...>;
};

template <typename Tup, template <typename> typename Pred,
          typename Acc = std::tuple<>>
struct filter;

template <template <typename> typename Pred, typename Acc>
struct filter<std::tuple<>, Pred, Acc> {
    using type = Acc;
};

template <typename Head, typename... Tail, template <typename> typename Pred,
          typename... Acc>
struct filter<std::tuple<Head, Tail...>, Pred, std::tuple<Acc...>> {
    using type = std::conditional_t<
        Pred<Head>::value,  // Does this pass the filter?
        typename filter<std::tuple<Tail...>, Pred, std::tuple<Head, Acc...>>::
            type,  // Yes, add it to accumulator
        typename filter<std::tuple<Tail...>, Pred,
                        std::tuple<Acc...>>::type>;  // No, go on.
};

template <typename Tup>
struct strip_const_tuple;

template <typename... Ts>
struct strip_const_tuple<std::tuple<Ts...>> {
    using type = std::tuple<std::remove_const_t<Ts>...>;
};

template <typename V>
struct view_traits;

template <typename... Cs>
struct view_traits<View<Cs...>> {
    using const_components = strip_const_tuple<
        typename filter<std::tuple<Cs...>, std::is_const>::type>::type;
    template <typename T>
    using is_mut = std::negation<std::is_const<T>>;
    using mut_components = filter<std::tuple<Cs...>, is_mut>::type;
    using components = std::tuple<Cs...>;
};

template <typename T, typename Tup>
struct type_in_tuple;

template <typename T>
struct type_in_tuple<T, std::tuple<>> {
    constexpr static bool value = false;
};

template <typename T, typename Head, typename... Ts>
struct type_in_tuple<T, std::tuple<Head, Ts...>> {
    constexpr static bool value =
        std::conditional_t<std::same_as<T, Head>, typename std::true_type,
                           type_in_tuple<T, std::tuple<Ts...>>>::value;
};

template <typename Tup1, typename Tup2, typename Acc = std::tuple<>>
struct tuple_intersection;

template <typename... Tup2, typename... Acc>
struct tuple_intersection<std::tuple<>, std::tuple<Tup2...>,
                          std::tuple<Acc...>> {
    using type = std::tuple<Acc...>;
};

template <typename Head, typename... Tup1, typename... Tup2, typename... Acc>
struct tuple_intersection<std::tuple<Head, Tup1...>, std::tuple<Tup2...>,
                          std::tuple<Acc...>> {
    using type = std::conditional_t<
        type_in_tuple<Head, std::tuple<Tup2...>>::value,
        tuple_intersection<std::tuple<Tup1...>, std::tuple<Tup2...>,
                           std::tuple<Acc..., Head>>,
        tuple_intersection<std::tuple<Tup1...>, std::tuple<Tup2...>,
                           std::tuple<Acc...>>>::type;
};

template <typename Tup>
struct contains_duplicate_types;

template <typename T>
struct contains_duplicate_types<std::tuple<T>> {
    constexpr static bool value = false;
};

template <>
struct contains_duplicate_types<std::tuple<>> {
    constexpr static bool value = false;
};

template <typename Head, typename... Ts>
struct contains_duplicate_types<std::tuple<Head, Ts...>> {
    constexpr static bool value =
        type_in_tuple<Head, std::tuple<Ts...>>::value ||
        contains_duplicate_types<std::tuple<Ts...>>::value;
};

template <typename... Vs>
struct incompatible_views {
    using all_mut_views = decltype(std::tuple_cat(
        std::declval<typename view_traits<Vs>::mut_components>()...));
    using all_const_views = decltype(std::tuple_cat(
        std::declval<typename view_traits<Vs>::const_components>()...));
    using unioned = tuple_intersection<all_mut_views, all_const_views>::type;
    using duplicate_mut = contains_duplicate_types<all_mut_views>;
    constexpr static bool value =
        std::tuple_size_v<unioned> != 0 || duplicate_mut::value;
};

static_assert(incompatible_views<View<int>, View<const int>>::value == true);
static_assert(incompatible_views<View<const int>, View<double>>::value ==
              false);

template <typename S>
concept System = requires(S s) {
    typename callable_traits<S>::return_type;
    typename callable_traits<S>::args;
};

template <typename Tup>
struct tuple_head {
    using type = std::tuple_element_t<0, Tup>;
};

template <typename Systems>
struct Layer;

template <typename... Systems>
struct Layer<std::tuple<Systems...>> {
    using type = std::tuple<
        std::conditional_t<std::is_pointer_v<Systems>, Systems, Systems*>...>;

    static_assert(
        (!incompatible_views<typename tuple_head<
             typename callable_traits<Systems>::args>::type...>::value),
        "Incompatible views in the same layer");

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
            entities_.push_back(e);
            generations_.push_back(0);
        }
        return e;
    }

    template <Component C>
    void add_component(const Entity& e, C&& component) {
        static_assert(type_in_tuple<C, std::tuple<Components...>>::value,
                      "Component not registered in the world");
        std::get<SparseSet<C>>(components_)
            .insert(e, std::forward<C>(component));
    }

    template <Component C, typename... Args>
    void add_component(const Entity& e, Args&&... args) {
        static_assert(type_in_tuple<C, std::tuple<Components...>>::value,
                      "Component not registered in the world");
        std::get<SparseSet<C>>(components_)
            .emplace(e, std::forward<Args>(args)...);
    }

    template <Component C>
    auto& get_component(const Entity& e) {
        static_assert(type_in_tuple<C, std::tuple<Components...>>::value,
                      "Component not registered in the world");
        auto* comp = std::get<SparseSet<C>>(components_).at(e);
        CSICS_RUNTIME_ASSERT(comp != nullptr,
                             "Entity does not have the requested component");
        return *comp;
    }

    void run(double dt) {
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            (std::apply(
                 [&](auto&&... systems) {
                     (
                         [&](auto&& system) {
                             using view_type =
                                 typename tuple_head<typename callable_traits<
                                     decltype(system)>::args>::type;
                             using components =
                                 view_traits<view_type>::components;

                             auto v = [&]<typename... Cs>(std::tuple<Cs...>) {
                                 return view_type(
                                     std::get<
                                         SparseSet<std::remove_const_t<Cs>>>(
                                         components_)...);
                             }(components{});
                             executor_.submit(system, v, dt);
                         }(systems),
                         ...);
                     executor_.join();
                     std::get<Is>(hooks_)();
                 },
                 std::get<Is>(layers_).systems),
             ...);
        }(std::make_index_sequence<sizeof...(Layers)>{});
    }

    StaticWorld(std::tuple<Layers...> layers, std::tuple<Hooks...> hooks,
                Executor executor)
        : layers_(layers),
          hooks_(hooks),
          components_({}),
          executor_(executor),
          entities_() {}

   private:
    std::tuple<Layers...> layers_;
    std::tuple<Hooks...> hooks_;
    std::tuple<SparseSet<Components>...> components_;
    Buffer<Entity> entities_;
    Buffer<std::uint32_t> dead_entities_;
    Buffer<std::uint32_t> generations_;
    Executor executor_;

    friend class StaticWorldBuilder<
        std::tuple<Layers...>, std::tuple<Components...>, std::tuple<Hooks...>>;
};

template <typename... Layers, Component... Components, typename... Hooks>
class StaticWorldBuilder<std::tuple<Layers...>, std::tuple<Components...>,
                         std::tuple<Hooks...>> {
   public:
    StaticWorldBuilder() : layers_(), hooks_() {}

    StaticWorldBuilder(std::tuple<Layers...> layers, std::tuple<Hooks...> hooks)
        : layers_(layers), hooks_(hooks) {}

    template <System... Ss>
    auto add_layer(Ss... systems) {
        return StaticWorldBuilder<
            std::tuple<Layers..., Layer<std::tuple<Ss...>>>,
            std::tuple<Components...>, std::tuple<Hooks...>>(
            std::tuple_cat(layers_, std::make_tuple(Layer<std::tuple<Ss...>>{
                                        std::tuple(systems...)})),
            hooks_);
    }

    template <Component C>
    auto add_component() {
        return StaticWorldBuilder<std::tuple<Layers...>,
                                  std::tuple<Components..., C>,
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
        return StaticWorldBuilder<std::tuple<Layers...>,
                                  std::tuple<Components...>,
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
            return std::make_tuple(([]() { (void)Is; })...);
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
