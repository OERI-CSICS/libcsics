#pragma once

#include <cassert>
#include <ranges>

#include "csics/Types.hpp"
#include "csics/linalg/Complex.hpp"
#include "csics/linalg/Ops.hpp"

namespace csics::dsp {

class TransferFunction {
   public:
    using ResponseFunc =
        std::function<linalg::Complex<float>(double frequency)>;
    struct Band {
        Buffer<linalg::Complex<float>> response;
        Range<float> frequency_range;  // [start, end] in Hz
        float bin_width;

        BasicBufferView<const linalg::Complex<float>> response_view() const {
            return response(0, response.size());
        }

        BasicBufferView<linalg::Complex<float>> response_view() {
            return response(0, response.size());
        }

        BasicBufferView<const linalg::Complex<float>> response_view(
            Range<float> range) const {
            std::size_t offset =
                (range.bottom() - frequency_range.bottom()) / bin_width;
            std::size_t length = range.size() / bin_width;
            return response(offset, length);
        }

        BasicBufferView<linalg::Complex<float>> response_view(
            Range<float> range) {
            std::size_t offset =
                (range.bottom() - frequency_range.bottom()) / bin_width;
            std::size_t length = range.size() / bin_width;
            return response(offset, length);
        }
    };

    TransferFunction(const Buffer<Band>& bands) : bands_(bands) {}
    TransferFunction(Buffer<Band>&& bands) : bands_(std::move(bands)) {}
    TransferFunction(const Band& band) : bands_({band}) {}
    TransferFunction(Band&& band) : bands_({std::move(band)}) {}

    TransferFunction(std::initializer_list<Band> bands) : bands_(bands) {}

    TransferFunction(ResponseFunc response_func,
                     BasicBufferView<const Range<float>> band_ranges,
                     std::size_t points_per_band = 100) {
        for (const auto& range : band_ranges) {
            Buffer<linalg::Complex<float>> response(
                points_per_band);  // 100 points per band
            double step = (range.top() - range.bottom()) / (response.size());
            for (std::size_t i = 0; i < response.size(); ++i) {
                double freq = range.bottom() + i * step;
                response[i] = response_func(freq);
            }
            bands_.push_back({std::move(response), range, (float)step});
        }
    }

    static TransferFunction constant(linalg::Complex<float> value,
                                     Range<float> frequency_range,
                                     float bin_width) {
        std::size_t points = (frequency_range.size() / bin_width);
        Buffer<Band> bands;
        Buffer<linalg::Complex<float>> response(points, value);
        bands.push_back({std::move(response), frequency_range, bin_width});
        return TransferFunction{std::move(bands)};
    }

    static TransferFunction constant(linalg::Complex<float> value,
                                     Linspace<float> frequency_bins) {
        return TransferFunction::constant(
            value,
            Range<float>{frequency_bins.bottom(), frequency_bins.top()},
            frequency_bins.step());
    }

    Buffer<Band> bands() const { return bands_; }

    TransferFunction operator*(linalg::Complex<float> scalar) const {
        Buffer<Band> new_bands;
        for (const auto& band : bands_) {
            Buffer<linalg::Complex<float>> new_response(band.response.size());
            for (std::size_t i = 0; i < band.response.size(); ++i) {
                new_response[i] = band.response[i] * scalar;
            }
            new_bands.push_back({std::move(new_response), band.frequency_range,
                                 band.bin_width});
        }
        return TransferFunction{std::move(new_bands)};
    }

    std::optional<TransferFunction> operator*(
        const TransferFunction& other) const {
        Buffer<Band> new_bands;
        Buffer<Range<float>> leftover_ranges;
        for (const auto& band1 : bands_) {
            leftover_ranges.push_back(band1.frequency_range);
            for (const auto& band2 : other.bands_) {
                // Find overlap
                if (band1.bin_width != band2.bin_width) {
                    // Require same bin width for now to simplify interpolation.
                    // Resampling needs to be separate and explicit.
                    // in order to avoid hidden changes
                    return std::nullopt;
                }

                float start = std::max(band1.frequency_range.bottom(),
                                       band2.frequency_range.bottom());
                float end = std::min(band1.frequency_range.top(),
                                     band2.frequency_range.top());

                if (start >= end) {
                    continue;
                }

                // Create new band for the overlap
                Buffer<linalg::Complex<float>> new_response((end - start) /
                                                            band1.bin_width);
                std::size_t offset1 =
                    (start - band1.frequency_range.bottom()) / band1.bin_width;
                std::size_t offset2 =
                    (start - band2.frequency_range.bottom()) / band2.bin_width;
                for (std::size_t i = 0; i < new_response.size(); ++i) {
                    new_response[i] = band1.response[offset1 + i] *
                                      band2.response[offset2 + i];
                }
                new_bands.push_back({std::move(new_response),
                                     Range<float>{start, end},
                                     band1.bin_width});

                if (start <= leftover_ranges.front().bottom() &&
                    end >= leftover_ranges.back().top()) {
                    // Do nothing, this band is fully covered.
                } else if (start <= leftover_ranges.front().bottom()) {
                    leftover_ranges.front() =
                        Range<float>{end, leftover_ranges.front().top()};
                } else if (end >= leftover_ranges.back().top()) {
                    leftover_ranges.back() =
                        Range<float>{leftover_ranges.back().bottom(), start};
                } else {
                    // Can't support holes in the middle of the range for now.
                    // Assuming this never gets hit, leftover_ranges should
                    // only ever have one range in it
                    assert(false && "Unsupported leftover range configuration");
                }
            }

            for (const auto& leftover_range : leftover_ranges) {
                if (leftover_range.size() > 0) {
                    auto left = (leftover_range.bottom() -
                                 band1.frequency_range.bottom()) /
                                band1.bin_width;
                    auto right = (leftover_range.top() -
                                  band1.frequency_range.bottom()) /
                                 band1.bin_width;
                    auto view = band1.response((std::size_t)left,
                                               (std::size_t)(right - left));
                    new_bands.push_back({Buffer<linalg::Complex<float>>(view),
                                         leftover_range, band1.bin_width});
                }
            }
            leftover_ranges.clear();
        }
        return TransferFunction{std::move(new_bands)};
    }

    float bandwidth() const {
        float bw = 0.0f;
        for (const auto& band : bands_) {
            bw += band.frequency_range.size();
        }
        return bw;
    }

    float center_frequency() const {
        float total_weighted_freq = 0.0f;
        float total_bandwidth = 0.0f;
        for (const auto& band : bands_) {
            float band_center =
                (band.frequency_range.bottom() + band.frequency_range.top()) /
                2.0f;
            total_weighted_freq += band_center * band.frequency_range.size();
            total_bandwidth += band.frequency_range.size();
        }
        return total_weighted_freq / total_bandwidth;
    }

    float eval_at(float frequency) const {
        for (const auto& band : bands_) {
            if (frequency >= band.frequency_range.bottom() &&
                frequency <= band.frequency_range.top()) {
                std::size_t index =
                    (frequency - band.frequency_range.bottom()) /
                    band.bin_width;
                return linalg::abs(band.response[index]);
            }
        }
        return 0.0f;  // No response outside of defined bands
    }

    Range<float> frequency_range() const {
        if (bands_.empty()) {
            return Range<float>(0.0f, 0.0f);
        }
        float min_freq = bands_.front().frequency_range.bottom();
        float max_freq = bands_.back().frequency_range.top();
        return Range<float>(min_freq, max_freq);
    }

    float at_freq(float frequency) const { return eval_at(frequency); }

    float at_bin(std::size_t bin_index) const {
        std::size_t cumulative_bins = 0;
        for (const auto& band : bands_) {
            std::size_t band_bins = band.response.size();
            if (bin_index < cumulative_bins + band_bins) {
                return linalg::abs(band.response[bin_index - cumulative_bins]);
            }
            cumulative_bins += band_bins;
        }
        throw std::out_of_range("Bin index out of range");
    }

    float bin_width() const {
        if (bands_.empty()) {
            return 0.0f;
        }
        return bands_.front()
            .bin_width;  // Assuming all bands have the same bin width
    }

    Buffer<float> psd() const {
        Buffer<float> result;
        for (const auto& band : bands_) {
            for (const auto& value : band.response) {
                result.push_back(linalg::norm(value));
            }
        }
        return result;
    }

   private:
    Buffer<Band> bands_;

   public:
    class Iterator {
       public:
        using value_type = linalg::Complex<float>;
        using difference_type = std::ptrdiff_t;
        using pointer = const linalg::Complex<float>*;
        using reference = value_type&;
        using iterator_category = std::input_iterator_tag;

        Iterator(TransferFunction* tf, std::size_t band_index,
                 std::size_t bin_index)
            : band_index_(band_index), bin_index_(bin_index), tf_(tf) {}

        reference operator*() {
            return tf_->bands_[band_index_].response[bin_index_];
        }

        Iterator& operator++() {
            if (band_index_ >= tf_->bands_.size()) {
                return *this;  // Already at end
            }
            bin_index_++;
            if (bin_index_ >= tf_->bands_[band_index_].response.size()) {
                band_index_++;
                bin_index_ = 0;
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const Iterator& other) const {
            return tf_ == other.tf_ && band_index_ == other.band_index_ &&
                   bin_index_ == other.bin_index_;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

       private:
        std::size_t band_index_;
        std::size_t bin_index_;
        TransferFunction* tf_;
    };

    class ConstIterator {
       public:
        using value_type = const linalg::Complex<float>;
        using difference_type = std::ptrdiff_t;
        using pointer = const linalg::Complex<float>*;
        using reference = const value_type&;
        using iterator_category = std::input_iterator_tag;

        ConstIterator(const TransferFunction* tf, std::size_t band_index,
                      std::size_t bin_index)
            : band_index_(band_index), bin_index_(bin_index), tf_(tf) {}

        reference operator*() {
            return tf_->bands_[band_index_].response[bin_index_];
        }

        ConstIterator& operator++() {
            if (band_index_ >= tf_->bands_.size()) {
                return *this;  // Already at end
            }
            bin_index_++;
            if (bin_index_ >= tf_->bands_[band_index_].response.size()) {
                band_index_++;
                bin_index_ = 0;
            }
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const ConstIterator& other) const {
            return tf_ == other.tf_ && band_index_ == other.band_index_ &&
                   bin_index_ == other.bin_index_;
        }

        bool operator!=(const ConstIterator& other) const {
            return !(*this == other);
        }

       private:
        std::size_t band_index_;
        std::size_t bin_index_;
        const TransferFunction* tf_;
    };

    Iterator begin() { return Iterator(this, 0, 0); }
    Iterator end() { return Iterator(this, bands_.size(), 0); }

    ConstIterator begin() const { return ConstIterator(this, 0, 0); }
    ConstIterator end() const { return ConstIterator(this, bands_.size(), 0); }
};

};  // namespace csics::dsp
