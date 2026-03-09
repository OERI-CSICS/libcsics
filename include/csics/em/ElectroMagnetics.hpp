#pragma once
#include <concepts>

#include "csics/Buffer.hpp"
#include "csics/executor/Concept.hpp"
#include "csics/geo/Concepts.hpp"
#include "csics/geo/Coordinates.hpp"
#include "csics/geo/Ellipsoids.hpp"
#include "csics/geo/Ops.hpp"
namespace csics::em {

constexpr auto to_watts(auto dbm) -> double {
    return std::pow(10.0, dbm / 10.0) / 1000.0;
}

constexpr auto to_dbm(auto watts) -> double {
    if (watts <= 0.0) {
        return -1000.0; // effectively negative infinity for practical purposes
                        // without introducing NaN or -inf which can cause issues in calculations
    }
    return 10.0 * std::log10(watts * 1000.0);
}

class Link;

struct PropagationResult {
    double path_loss;         // dB
    double received_power;    // watts
    double center_frequency;  // Hz
    double bandwidth;         // Hz
};

class Propagation {
   public:
    double attenuation = 0.0;       // dB
    double phase_shift = 0.0;       // radians
    double center_frequency = 0.0;  // Hz
    double bandwidth = 0.0;         // Hz
    double transmit_power = 0.0;    // watts

   public:
    static constexpr auto isotropic = [](double, double) { return 0.0; };
    using propagation_fn = double (*)(double, double);
    Propagation operator*(Propagation other) const {
        Propagation result;
        result.attenuation = attenuation + other.attenuation;
        result.phase_shift =
            std::fmod(phase_shift + other.phase_shift, 2.0 * M_PI);
        result.center_frequency =
            center_frequency == 0 ? other.center_frequency : center_frequency;
        result.bandwidth = bandwidth == 0 ? other.bandwidth : bandwidth;
        result.transmit_power =
            transmit_power == 0 ? other.transmit_power : transmit_power;
        return result;
    }

    Propagation operator()(double theta, double phi) const;  // tbd
    PropagationResult to_result() const {
        PropagationResult result;
        result.path_loss = attenuation;
        result.received_power = to_watts(to_dbm(transmit_power) - attenuation);
        result.center_frequency = center_frequency;
        result.bandwidth = bandwidth;
        return result;
    }
    operator PropagationResult() const { return to_result(); }

   private:
    propagation_fn model = isotropic;
};

template <typename T>
concept EnvironmentalModel = requires(T t) {
    { t.attenuate(double{}, double{}) } -> std::convertible_to<Propagation>;
};

template <typename T>
concept NoiseModel = requires(T t, double band_start, double band_end) {
    { t.floor() } -> std::convertible_to<double>;                      // dBm
    { t.floor(band_start, band_end) } -> std::convertible_to<double>;  // dBm
};

template <typename T>
concept ChannelModel =
    requires(T t) {
        typename T::EnvironmentalModelType;
        typename T::NoiseModelType;
        { t(double{}, double{}) } -> std::same_as<Propagation>;
        { t.noise_floor() } -> std::convertible_to<double>;          // dBm
        { t.noise_floor(0.0, 0.0) } -> std::convertible_to<double>;  // dBm
    } && EnvironmentalModel<typename T::EnvironmentalModelType> &&
    NoiseModel<typename T::NoiseModelType>;

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
    AWGNoiseModel(double dbm_per_hz) : n0_(dbm_per_hz) {}

    double floor() const { return n0_; }
    double floor(double band_start, double band_end) const {
        return floor() + 10.0 * std::log10(band_end - band_start);
    }

   private:
    double n0_;  // dBm/Hz
};

// TODO: Implement
class ThermalNoiseModel;

class FreeSpaceEnvironmentalModel {
   public:
    Propagation attenuate(double distance, double center_frequency) const {
        Propagation result;
        result.attenuation = 20.0 * std::log10(distance) +
                             20.0 * std::log10(center_frequency) - 147.55;
        return result;
    }
};

template <EnvironmentalModel EM, NoiseModel NM>
class StaticChannel {
   public:
    using EnvironmentalModelType = EM;
    using NoiseModelType = NM;
    StaticChannel(EM em, NM nm) : env_model_(em), noise_model_(nm) {}

    Propagation operator()(double distance, double center_frequency) const {
        return env_model_.attenuate(distance, center_frequency);
    }

    double noise_floor() const { return noise_model_.floor(); }
    double noise_floor(double band_start, double band_end) const {
        return noise_model_.floor(band_start, band_end);
    }

   private:
    EM env_model_;
    NM noise_model_;
};

class IsometricTransmitter {
   public:
    IsometricTransmitter(double power, double center_frequency,
                         double bandwidth,
                         geo::GeospatialCoordinate auto location)
        : power_(power),
          center_frequency_(center_frequency),
          bandwidth_(bandwidth), location_(geo::to_geodetic(location)) {}

    auto transmit_towards(geo::GeospatialCoordinate auto) const {
        return propagate();
    }

    Propagation propagate() const {
        Propagation result;
        result.transmit_power = power_;
        result.center_frequency = center_frequency_;
        result.bandwidth = bandwidth_;
        return result;
    }

    auto location() const { return location_; }

   private:
    double power_;             // watts
    double center_frequency_;  // Hz
    double bandwidth_;         // Hz
    geo::LatLongAlt<double> location_;
};

class IsometricReceiver {
   public:
    IsometricReceiver(geo::GeospatialCoordinate auto loc): location_(loc) {}

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
    static Propagation apply(ChannelModel auto channel, Transmitter auto tx,
                      Receiver auto rx) {
        auto tx_propagation = tx.propagate();
        auto channel_propagation =
            channel(geo::Euclidean::distance(tx.location(), rx.location()),
                    tx_propagation.center_frequency);
        auto rx_propagation = rx.receive();
        return tx_propagation * channel_propagation * rx_propagation;
    }

    static LinkBudget budget(ChannelModel auto channel,
                      BasicBufferView<PropagationResult> desireds,
                      BasicBufferView<PropagationResult> interferers) {
        CSICS_RUNTIME_ASSERT(desireds.size() > 0, "At least one desired signal is required");

        double total_interference_power = 0.0;
        double total_desired_power = 0.0;
        double avg_center_frequency = 0.0;
        double avg_bandwidth = 0.0;
        for (const auto& interferer : interferers) {
            total_interference_power += interferer.received_power;
            avg_center_frequency += interferer.center_frequency;
            avg_bandwidth += interferer.bandwidth;
        }

        for (const auto& desired : desireds) {
            total_desired_power += desired.received_power;
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
    static LinkBudgetBatch apply_and_budget(ChannelModel auto channel,
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

                LinkBudget budget =
                    budget(channel, desireds, interferers);
                batch.push_back(budget);
            }
            return batch;
        }

        // executor batch processing tbd
        return batch;
    }
};
};  // namespace csics::sim::em
