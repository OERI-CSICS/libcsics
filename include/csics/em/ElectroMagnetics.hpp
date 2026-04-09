#pragma once
#include <concepts>
#include <random>

#include "csics/Buffer.hpp"
#include "csics/dsp/Integrators.hpp"
#include "csics/dsp/Transfer.hpp"
#include "csics/executor/Concept.hpp"
#include "csics/geo/Concepts.hpp"
#include "csics/geo/Coordinates.hpp"
#include "csics/geo/Ellipsoids.hpp"
#include "csics/geo/Ops.hpp"
#include "csics/linalg/Complex.hpp"
namespace csics::em {

constexpr double kSpeedOfLight = 299792458.0;  // m/s

constexpr auto to_watts(auto dbm) -> double {
    return std::pow(10.0, dbm / 10.0) / 1000.0;
}

constexpr auto to_dbm(auto watts) -> double {
    if (watts <= 0.0) {
        return -1000.0;  // effectively negative infinity for practical purposes
                         // without introducing NaN or -inf which can cause
                         // issues in calculations
    }
    return 10.0 * std::log10(watts * 1000.0);
}

class Link;

struct PropagationResult {
    double power;             // watts
    double center_frequency;  // Hz
    double bandwidth;         // Hz
};

struct Propagation {
   public:
    double distance = 0.0;
    dsp::TransferFunction tf;
    Propagation() : distance(0.0), tf({}) {}
    Propagation(double delay, dsp::TransferFunction tf)
        : distance(delay), tf(std::move(tf)) {}

    Propagation operator*(const Propagation& other) const {
        auto d = distance + other.distance;
        auto combined_tf = tf * other.tf;
        CSICS_RUNTIME_ASSERT(combined_tf.has_value(),
                             "Failed to combine transfer functions");
        auto tf = std::move(combined_tf.value());
        return {d, std::move(tf)};
    }

    Propagation operator*(const dsp::TransferFunction& other) const {
        auto combined_tf = tf * other;
        CSICS_RUNTIME_ASSERT(combined_tf.has_value(),
                             "Failed to combine transfer functions");
        auto tf = std::move(combined_tf.value());
        return {distance, std::move(tf)};
    }

    Propagation& operator*=(const Propagation& other) {
        auto combined_tf = tf * other.tf;
        CSICS_RUNTIME_ASSERT(combined_tf.has_value(),
                             "Failed to combine transfer functions");
        tf = std::move(combined_tf.value());
        distance += other.distance;
        return *this;
    }

    PropagationResult result() const {
        double bw = tf.bandwidth();
        auto psd = tf.psd();
        float received_power = std::accumulate(psd.begin(), psd.end(), 0.0f) /
                               psd.size() * tf.bin_width() / bw;
        double center_frequency = tf.center_frequency();
        return {received_power, center_frequency, bw};
    }
};

template <typename T>
concept EnvironmentalModel = requires(T t, Linspace<double> freq_range) {
    {
        t.attenuate(double{}, freq_range)
    } -> std::convertible_to<dsp::TransferFunction>;
};

template <typename T>
concept NoiseModel = requires(T t, Range<float> range, float bin_width) {
    { t.sample() } -> std::convertible_to<linalg::Complex<float>>;
    {
        t.sample(range, bin_width)
    } -> std::convertible_to<Buffer<linalg::Complex<float>>>;
};

template <typename T>
concept Transmitter = requires(T t) {
    {
        t.transmit_towards(std::declval<geo::Geodetic<double, geo::WGS84>>())
    } -> std::same_as<Propagation>;

    {
        t.transmit_towards(std::declval<geo::Geocentric<double, geo::WGS84>>())
    } -> std::same_as<Propagation>;

    { t.propagate() } -> std::same_as<Propagation>;
    { t.location() } -> geo::GeospatialCoordinate;
};

template <typename T>
concept Receiver = requires(T t) {
    {
        t.receive_from(std::declval<geo::Geodetic<double, geo::WGS84>>())
    } -> std::same_as<Propagation>;

    {
        t.receive_from(std::declval<geo::Geocentric<double, geo::WGS84>>())
    } -> std::same_as<Propagation>;

    { t.receive() } -> std::same_as<Propagation>;
    { t.location() } -> geo::GeospatialCoordinate;
};

class AWGNoiseModel {
   public:
    AWGNoiseModel(double watts_per_hz, std::mt19937& rng)
        : rng_(rng), dist_(0.0f, std::sqrt(watts_per_hz / 2.0f)) {}

    linalg::Complex<float> sample() {
        return linalg::Complex<float>(dist_(rng_), dist_(rng_));
    }

    Buffer<linalg::Complex<float>> sample(Range<float> frequency_range,
                                          float bin_width) {
        std::size_t num_samples =
            static_cast<std::size_t>(frequency_range.size() / bin_width);
        Buffer<linalg::Complex<float>> samples(num_samples);
        for (std::size_t i = 0; i < num_samples; ++i) {
            samples[i] = sample();
        }
        return samples;
    }

   private:
    std::mt19937& rng_;
    std::normal_distribution<float> dist_;
};

// TODO: Implement
class ThermalNoiseModel;

class FreeSpaceEnvironmentalModel {
   public:
    dsp::TransferFunction attenuate(double distance,
                                    Linspace<float> freq_bins) const {
        // Free space path loss: (4 * pi * d * f / c)^2
        // where d is distance, f is frequency, c is speed of light
        // We can represent this as a transfer function with a single band
        // that covers all frequencies and has the appropriate attenuation
        float r = 1.0f / (4.0f * static_cast<float>(M_PI) *
                          static_cast<float>(distance) * kSpeedOfLight);
        return dsp::TransferFunction::constant(linalg::Complex<float>{r, 0.0f},
                                          freq_bins);
    }
};

class IsometricTransmitter {
   public:
    IsometricTransmitter(float power, float center_frequency, float bandwidth,
                         float bin_width,
                         geo::GeospatialCoordinate auto location)
        : power_(power),
          center_frequency_(center_frequency),
          bandwidth_(bandwidth),
          location_(geo::to_geodetic(location)),
          bin_width_(bin_width) {}

    auto transmit_towards(geo::GeospatialCoordinate auto) const {
        return propagate();
    }

    Propagation propagate() const {
        return Propagation(0.0f,
                           dsp::TransferFunction::constant(
                               linalg::Complex<float>{power_, 0.0f},
                               Range{center_frequency_ - bandwidth_ / 2.0,
                                     center_frequency_ + bandwidth_ / 2.0},
                               bin_width_));
    }

    auto location() const { return location_; }

   private:
    float power_;             // watts
    float center_frequency_;  // Hz
    float bandwidth_;         // Hz
    geo::LatLongAlt<double> location_;
    float bin_width_;
};

class IsometricReceiver {
   public:
    IsometricReceiver(geo::GeospatialCoordinate auto loc) : location_(loc) {}

    Propagation receive_from(geo::GeospatialCoordinate auto) const {
        return receive();
    }

    Propagation receive() const {
        Propagation result;
        return result;
    }

    auto location() const { return location_; }

   private:
    geo::LatLongAlt<double> location_;
};

struct LinkBudget {
    double received_power;      // dBm
    double noise_floor;         // dBm
    double interference_power;  // dBm
    double sinr;                // dB
};

struct LinkBudgetBatch {
    using ValueBuffer =
        Buffer<double, kCacheLineSize, CapacityPolicy::CacheAligned()>;
    ValueBuffer received_powers;
    ValueBuffer noise_floors;
    ValueBuffer interference_powers;
    ValueBuffer sinrs;

    LinkBudgetBatch(std::size_t size)
        : received_powers(size),
          noise_floors(size),
          interference_powers(size),
          sinrs(size) {}

    void push_back(const LinkBudget& budget) {
        received_powers.push_back(budget.received_power);
        noise_floors.push_back(budget.noise_floor);
        interference_powers.push_back(budget.interference_power);
        sinrs.push_back(budget.sinr);
    }
};

class Link {
   public:
    static Propagation apply(EnvironmentalModel auto channel,
                             Transmitter auto tx, Receiver auto rx) {
        auto tx_propagation = tx.propagate();
        auto channel_propagation = channel.apply(tx_propagation.distance);
        auto rx_propagation = rx.receive();
        return tx_propagation * channel_propagation * rx_propagation;
    }

    static LinkBudget budget(EnvironmentalModel auto channel,
                             BasicBufferView<PropagationResult> desireds,
                             BasicBufferView<PropagationResult> interferers) {
        CSICS_RUNTIME_ASSERT(desireds.size() > 0,
                             "At least one desired signal is required");

        double total_interference_power = 0.0;
        double total_desired_power = 0.0;
        double avg_center_frequency = 0.0;
        double avg_bandwidth = 0.0;
        for (const auto& interferer : interferers) {
            total_interference_power += interferer.power;
            avg_center_frequency += interferer.center_frequency;
            avg_bandwidth += interferer.bandwidth;
        }

        for (const auto& desired : desireds) {
            total_desired_power += desired.power;
            avg_center_frequency += desired.center_frequency;
            avg_bandwidth += desired.bandwidth;
        }

        avg_center_frequency /= (desireds.size() + interferers.size());
        avg_bandwidth /= (desireds.size() + interferers.size());

        double noise_floor =
            channel.noise_floor(avg_center_frequency - avg_bandwidth / 2.0,
                                avg_center_frequency + avg_bandwidth / 2.0);

        return LinkBudget{
            .received_power = to_dbm(total_desired_power),
            .noise_floor = noise_floor,
            .interference_power = to_dbm(total_interference_power),
            .sinr = to_dbm(total_desired_power /
                           (to_watts(noise_floor) + total_interference_power))};
    }

    template <Transmitter Tx, Receiver Rx>
    static LinkBudgetBatch apply_and_budget(EnvironmentalModel auto channel,
                                            Buffer<Tx>& priority_txs,
                                            Buffer<Tx>& interferer_txs,
                                            Buffer<Rx>& rxs,
                                            executor::Executor auto&) {
        LinkBudgetBatch batch(rxs.size());
        // This is always true for now as the batch is in development
        // TODO: implement a heuristic to determine when to batch and when to do
        // sequentially
        if (rxs.size() * (priority_txs.size() + interferer_txs.size()) < 0) {
            Buffer<PropagationResult> desireds(priority_txs.size());
            Buffer<PropagationResult> interferers(interferer_txs.size());

            for (std::size_t i = 0; i < rxs.size(); ++i) {
                auto& rx = rxs[i];

                for (std::size_t j = 0; j < priority_txs.size(); ++j) {
                    desireds[j] = apply(channel, priority_txs[j], rx);
                }

                for (std::size_t k = 0; k < interferer_txs.size(); ++k) {
                    interferers[k] = apply(channel, interferer_txs[k], rx);
                }

                LinkBudget budget = budget(channel, desireds, interferers);
                batch.push_back(budget);
            }
            return batch;
        }

        // executor batch processing tbd
        return batch;
    }
};
};  // namespace csics::em
