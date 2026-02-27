
#pragma once

#include <tuple>

#include "csics/sim/ecs/View.hpp"
namespace csics::sim::ecs {

template <typename F>
struct callable_traits;

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

template <typename F>
struct callable_traits : callable_traits<decltype(&F::operator())> {};

template <typename Tup, template <typename> typename Pred, typename Acc=std::tuple<>>
struct filter;

template <template <typename> typename Pred, typename Acc>
struct filter<std::tuple<>, Pred, Acc> {
    using type = Acc;
};

template <typename Head, typename... Tail, template <typename> typename Pred,
          typename... Acc>
struct filter<std::tuple<Head, Tail...>, Pred, std::tuple<Acc...>> {
    using type = std::conditional_t<
        Pred<Head>::value, // Does this pass the filter?
        typename filter<std::tuple<Tail...>, Pred,
                        std::tuple<Head, Acc...>>::type, // Yes, add it to accumulator
        typename filter<std::tuple<Tail...>, Pred, std::tuple<Acc...>>::type>; // No, go on.
};

template <typename V>
struct view_traits;

template <typename... Cs>
struct view_traits<View<Cs...>> {
    using const_components = filter<std::tuple<Cs...>, std::is_const>::type;
    template <typename T>
    using is_mut = std::negation<std::is_const<T>>;
    using mut_components = filter<std::tuple<Cs...>, is_mut>::type;
};


template <typename T, typename Tup>
struct type_in_tuple;

template <typename T>
struct type_in_tuple<T, std::tuple<>> {
    constexpr static bool value = false;
};

template<typename T, typename Head, typename... Ts>
struct type_in_tuple<T,  std::tuple<Head, Ts...>> {
    constexpr static bool value = std::conditional_t<std::same_as<T, Head>,  typename std::true_type, type_in_tuple<T, std::tuple<Ts...>>>::value;
};

template <typename Tup1, typename Tup2, typename Acc=std::tuple<>>
struct tuple_union;


template<typename... Tup2, typename... Acc>
struct tuple_union<std::tuple<>, std::tuple<Tup2...>, std::tuple<Acc...>> {
    using type = std::tuple<Acc...>;
};

template <typename Head, typename... Tup1, typename... Tup2, typename... Acc>
struct tuple_union<std::tuple<Head, Tup1...>, std::tuple<Tup2...>, std::tuple<Acc...>> {
    using type = std::conditional_t<type_in_tuple<Head, std::tuple<Tup2...>>::value, tuple_union<std::tuple<Tup1...>, std::tuple<Tup2...>, std::tuple<Acc..., Head>>,
          tuple_union<std::tuple<Tup1...>, std::tuple<Tup2...>, std::tuple<Acc...>>>::type;
};

template <typename... Vs>
struct incompatible_views {
    using all_mut_views = decltype(std::tuple_cat(view_traits<Vs>::mut_components...));
    using all_const_views = decltype(std::tuple_cat(view_traits<Vs>::const_components...));
    using unioned = tuple_union<all_mut_views, all_const_views>::type;
    constexpr static bool value = std::tuple_size_v<unioned> != 0;
};

};  // namespace csics::sim::ecs
