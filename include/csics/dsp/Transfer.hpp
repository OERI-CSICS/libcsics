#pragma once

#include <ranges>

#include "csics/linalg/Complex.hpp"

namespace csics::dsp {

class TransferFunction {
   public:
    using transfer_fn = std::function<linalg::Complex<float>(double frequency)>;
    TransferFunction(transfer_fn fn) : fn_(fn) {}

    TransferFunction operator*(TransferFunction other) const {
        return TransferFunction([f = fn_, g = other.fn_](double frequency) {
            return f(frequency) * g(frequency);
        });
    }

    static TransferFunction from_range(
        std::ranges::range auto r,
        std::function<std::size_t(double)> index_map) {
        return TransferFunction([r, index_map](double frequency) {
            std::size_t index = index_map(frequency);
            if (index < r.size()) {
                return r[index];
            }
            return linalg::Complex<float>{0.0f, 0.0f};
        });
    }

    static TransferFunction impulse() {
        return TransferFunction(
            [](double) { return linalg::Complex<float>{1.0f, 0.0f}; });
    }

    auto operator()(double frequency) const { return fn_(frequency); }
    auto operator[](double frequency) const { return fn_(frequency); }

   private:
    transfer_fn fn_;
};

inline auto ideal_bp_filter(double low_cutoff, double high_cutoff) {
    return TransferFunction([low_cutoff, high_cutoff](double frequency) {
        return (frequency >= low_cutoff && frequency <= high_cutoff)
                   ? linalg::Complex<float>{1.0f, 0.0f}
                   : linalg::Complex<float>{0.0f, 0.0f};
    });
}

inline auto ideal_low_pass(double cutoff_frequency) {
    return TransferFunction([cutoff_frequency](double frequency) {
        return (frequency <= cutoff_frequency)
                   ? linalg::Complex<float>{1.0f, 0.0f}
                   : linalg::Complex<float>{0.0f, 0.0f};
    });
}

};  // namespace csics::dsp
