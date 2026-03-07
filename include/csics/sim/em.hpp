#pragma once



#include <concepts>
namespace csics::sim::em {

    class Link;

    template <typename T>
    concept EnvironmentalModel = requires(T t, Link l) {
        { t.attenuate(l) } -> std::convertible_to<double>;
    };

    template <typename T>
    concept NoiseModel = requires(T t, Link l, double band_start, double band_end) {
        { t.floor() } -> std::convertible_to<double>;
        { t.floor(band_start, band_end) } -> std::convertible_to<double>;
    };

    class AWGNoiseModel {
    };

    class Channel {
    };

    class Transmitter {
    };

    class Receiver {
    };

    class Link {
        public:
            Link(Channel channel);

    };
};
