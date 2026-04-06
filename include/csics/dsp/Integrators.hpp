#pragma once

#include "csics/Types.hpp"
namespace csics::dsp {

template <typename I, typename F, typename T>
concept Integrator = requires(Range<T> range, F&& func, I integrator) {
    { integrator.operator()(range, func) } -> std::convertible_to<T>;
    { integrator.integrate(range, func) } -> std::convertible_to<T>;
} && std::is_default_constructible_v<I>;

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
        return 1.0f / 3.0f * h * (func(a) + 4.0f * func(a + h) + func(b));
    }

   private:
};

template <typename F, typename T, Integrator<F,T> I = SimpsonsIntegrator>
inline auto integrate(Range<double> range, auto&& func) {
    return I{}(range, func);
}

}  // namespace csics::dsp
