
#pragma once

#include <concepts>
#include <tuple>

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
struct callable_traits<F> : callable_traits<decltype(&std::remove_cvref_t<F>::operator())> {};


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
struct callable_traits<Ret (F::*&)(Args...)> {
    using return_type = Ret;
    using args = std::tuple<Args...>;
};

template <typename F, typename Ret, typename... Args>
struct callable_traits<Ret (F::*)(Args...) const> {
    using return_type = Ret;
    using args = std::tuple<Args...>;
};

template <typename F, typename Ret, typename... Args>
struct callable_traits<Ret (F::*&)(Args...) const> {
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

template <typename Tup>
struct tuple_head {
    using type = std::tuple_element_t<0, Tup>;
};

template <typename S>
concept HasView = requires { typename S::view_type; };

template <typename F>
struct system_traits {
    using view_type = tuple_head<typename callable_traits<F>::args>::type;
};

template <typename F>
    requires HasView<std::remove_cvref_t<F>>
struct system_traits<F> {
    using view_type = std::remove_cvref_t<F>::view_type;
};

template <typename F>
concept SystemWithView =
    std::is_invocable_v<F, typename system_traits<F>::view_type>;

template <typename F>
concept SystemWithDt =
    std::is_invocable_v<F, typename system_traits<F>::view_type, double>;

template <typename F, typename WCtx>
concept SystemWithContext =
    std::is_invocable_v<F, typename system_traits<F>::view_type, double, WCtx&>;

};  // namespace csics::sim::ecs
