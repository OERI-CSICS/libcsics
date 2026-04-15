#pragma once

#include <cassert>

#include "csics/Types.hpp"
#include "csics/linalg/Complex.hpp"
#include "csics/linalg/Ops.hpp"

namespace csics::dsp {
struct SpectralGrid {
    uint32_t fft_size;
    double center_frequency;
    enum class Alignment { Centered, Unshifted } alignment;
    uint64_t sample_rate_hz;

    bool operator==(const SpectralGrid& other) const {
        return fft_size == other.fft_size && alignment == other.alignment &&
               sample_rate_hz == other.sample_rate_hz;
    }

    constexpr double df() const {
        return static_cast<double>(sample_rate_hz) / fft_size;
    }

    constexpr Range<double> frequency_range() const {
        double f_start, f_end;
        f_start =
            center_frequency - (0.5 * sample_rate_hz);
        f_end =
            center_frequency + (0.5 * sample_rate_hz);
        return {f_start, f_end};
    }

    static SpectralGrid centered(uint32_t fft_size, double center_frequency,
                                 uint64_t sample_rate_hz) {
        return SpectralGrid{fft_size, center_frequency, Alignment::Centered,
                            sample_rate_hz};
    }

    static SpectralGrid unshifted(uint32_t fft_size, double center_frequency,
                                  uint64_t sample_rate_hz) {
        return SpectralGrid{fft_size, center_frequency, Alignment::Unshifted,
                            sample_rate_hz};
    }
};

class TransferFunction {
   public:
    TransferFunction() = default;
    TransferFunction(const TransferFunction& other) = default;
    TransferFunction(TransferFunction&& other) = default;
    TransferFunction& operator=(const TransferFunction& other) = default;
    TransferFunction& operator=(TransferFunction&& other) = default;
    TransferFunction(SpectralGrid grid, Buffer<linalg::Complex<float>> samples)
        : samples_(std::move(samples)), grid_(grid) {
        assert(samples_.size() == grid.fft_size);
    }

    std::optional<TransferFunction> operator*(
        const TransferFunction& other) const {
        if (grid_ != other.grid_) {
            return std::nullopt;
        }
        Buffer<linalg::Complex<float>> result_samples(grid_.fft_size);
        for (std::size_t i = 0; i < grid_.fft_size; ++i) {
            result_samples[i] = samples_[i] * other.samples_[i];
        }
        return TransferFunction(grid_, std::move(result_samples));
    }

    TransferFunction operator*(const linalg::Complex<float>& scalar) const {
        Buffer<linalg::Complex<float>> result_samples(grid_.fft_size);
        for (std::size_t i = 0; i < grid_.fft_size; ++i) {
            result_samples[i] = samples_[i] * scalar;
        }
        return TransferFunction(grid_, std::move(result_samples));
    }

    static TransferFunction constant(linalg::Complex<float> value,
                                     SpectralGrid grid) {
        Buffer<linalg::Complex<float>> samples(grid.fft_size);
        std::fill(samples.begin(), samples.end(), value);
        return TransferFunction(grid, std::move(samples));
    }

    static TransferFunction ideal_bandpass(double center_frequency,
                                           double bandwidth,
                                           SpectralGrid grid) {
        Buffer<linalg::Complex<float>> samples(grid.fft_size);
        double f_start = center_frequency - (bandwidth / 2.0);
        double f_end = center_frequency + (bandwidth / 2.0);
        if (f_start >= grid.frequency_range().top() ||
            f_end <= grid.frequency_range().bottom()) {
            std::cerr << "Warning: Bandpass frequencies [" << f_start << ", " << f_end
                      << "] are completely out of grid range ["
                      << grid.frequency_range().bottom() << ", "
                      << grid.frequency_range().top() << "]. Returning all zeros."
                      << std::endl;
            // band is completely out of range, return all zeros
            std::fill(samples.begin(), samples.end(),
                      linalg::Complex<float>{0.0f, 0.0f});
            return TransferFunction(grid, std::move(samples));
        }
        auto range = grid.frequency_range();
        int64_t ind_start = static_cast<int64_t>(
            std::floor((f_start - range.bottom()) / grid.df()));
        int64_t ind_end = static_cast<int64_t>(
            std::ceil((f_end - range.bottom()) / grid.df()));
        if (grid.alignment == SpectralGrid::Alignment::Unshifted) {
            // circularly shift indices for unshifted grid
            ind_start = (ind_start + grid.fft_size) % grid.fft_size;
            ind_end = (ind_end + grid.fft_size) % grid.fft_size;
            // swap if end came before start due to wrap around
            if (ind_end < ind_start) {
                int64_t temp = ind_start;
                ind_start = ind_end;
                ind_end = temp;
            }
        }

        for (std::size_t i = 0; i < grid.fft_size; ++i) {
            if (i >= static_cast<std::size_t>(ind_start) &&
                i < static_cast<std::size_t>(ind_end)) {
                samples[i] = linalg::Complex<float>{1.0f, 0.0f};
            } else {
                samples[i] = linalg::Complex<float>{0.0f, 0.0f};
            }
        }

        return TransferFunction(grid, std::move(samples));
    }

    void to_psd(BasicBufferView<float> psd_out) const {
        for (std::size_t i = 0; i < psd_out.size(); ++i) {
            psd_out[i] = linalg::norm(samples_[i]);
        }
    }

    double bin_width() const {
        return static_cast<double>(grid_.sample_rate_hz) / grid_.fft_size;
    }

    Buffer<linalg::Complex<float>> const& samples() const { return samples_; }
    const SpectralGrid& grid() const { return grid_; }

    double center_frequency() const { return grid_.center_frequency; }
    double bandwidth() const {
        return static_cast<double>(grid_.fft_size) * grid_.sample_rate_hz /
               grid_.fft_size;
    }

   private:
    Buffer<linalg::Complex<float>> samples_;
    SpectralGrid grid_;
};

};  // namespace csics::dsp
