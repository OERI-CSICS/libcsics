#pragma once

#include "csics/Types.hpp"
namespace csics::dsp {

template <typename I, typename F, typename T>
concept Integrator = requires(Range<T> range, F&& func, I integrator) {
    { integrator.operator()(range, func) } -> std::convertible_to<T>;
    { integrator.integrate(range, func) } -> std::convertible_to<T>;
} && std::is_default_constructible_v<I>;

template <typename T>
concept IntegrableObject = requires(T obj, double x) {
    { obj.eval_at(x) } -> std::convertible_to<double>;
};

template <typename T>
concept IntegrableFunction = (requires(T func, double x) {
    { func(x) } -> std::convertible_to<double>;
} && std::invocable<T, double>);

template <typename T>
requires IntegrableObject<std::remove_cvref_t<T>>
inline auto evaluate_at(T&& obj, double x) {
    return obj.eval_at(x);
}

template <typename F>
requires IntegrableFunction<std::remove_cvref_t<F>>
inline auto evaluate_at(F&& func, double x) {
    return func(x);
}

class SimpsonsIntegrator {
   public:
    SimpsonsIntegrator() {};

    template <typename F, typename T>
    float operator()(Range<T> range, F&& func) const {
        return integrate(range, func);
    }

    template <typename F, typename T>
    float integrate(Range<T> range, F&& func) const {
        T b = range.top();
        T a = range.bottom();
        T h = (b - a) / 2.0f;
        return 1.0f / 3.0f * h *
               (evaluate_at(func, a) + 4.0f * evaluate_at(func, a + h) +
                evaluate_at(func, b));
    }

   private:
};

template <typename F, typename T,
          Integrator<std::remove_cvref_t<F>, std::remove_cvref_t<T>> I =
              SimpsonsIntegrator>
inline auto integrate(Range<T> range, F&& func) {
    return I{}(range, func);
}

}  // namespace csics::dsp
