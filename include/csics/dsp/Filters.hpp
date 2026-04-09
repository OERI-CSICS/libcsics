#pragma once

#include "csics/Types.hpp"
#include "csics/dsp/Transfer.hpp"
#include "csics/linalg/Concepts.hpp"
namespace csics::dsp {

template <typename T>
concept FrequencyDomainFilter = requires(T t, TransferFunction tf) {
    {
        t.apply(tf)
    } -> std::convertible_to<TransferFunction>;
};

template <typename T, typename Iq>
concept TimeDomainFilter = requires(T t, Iq iq) {
    {
        t.apply(iq)
    } -> std::convertible_to<Iq>;
} && linalg::StaticMatrixLike<Iq>;


class IdealBandPassFilter {
   public:
    IdealBandPassFilter(float center_freq, float bandwidth)
        : center_freq_(center_freq), bandwidth_(bandwidth) {}

    TransferFunction apply(const TransferFunction& tf) const {
        TransferFunction::Band ret_band;
        Range<float> filter_range(center_freq_ - bandwidth_ / 2.0f,
                                  center_freq_ + bandwidth_ / 2.0f);
        for (const auto& band : tf.bands()) {
            auto intersection = band.frequency_range.intersection_with(filter_range);
            if (intersection.size() > 0) {
                auto response_view = band.response_view(intersection);
                ret_band.response.append(response_view.data(), response_view.size());
            }
        }

        ret_band.frequency_range = filter_range;
        ret_band.bin_width = tf.bands().front().bin_width;
        return TransferFunction(std::move(ret_band));
    }

   private:
    float center_freq_;
    float bandwidth_;
};

}  // namespace csics::dsp
