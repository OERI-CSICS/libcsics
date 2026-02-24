#pragma once

#include <cstdint>
#include <optional>

#include "csics/Buffer.hpp"
#include "csics/geo/Coordinates.hpp"
#include "csics/linalg/Coordinates.hpp"
#include "csics/linalg/Vec.hpp"
#include "csics/lvc/dis/Time.hpp"

namespace csics::lvc::dis {
enum class ProtocolVersion : std::uint8_t {
    // Only really concerned with DIS 6 and 7
    IEEE_1278_1998 = 6,
    IEEE_1278_2012 = 7,
    UNKNOWN = 0xFF
};

enum class PDUType : std::uint8_t {
    Other = 0,
    EntityState = 1,
    ElectromagneticEmission = 23,
    Transmitter = 25,
    Signal = 26,
    Receiver = 27,
};

using Vector = linalg::Vec3<float>;
using WorldCoordinates = geo::Geocentric<double>;
using EntityCoordinates = Vector;
using EulerAngles = linalg::EulerAngles<float>;

struct DISQuat {
    std::uint16_t u0;
    float x;
    float y;
    float z;

    DISQuat(EulerAngles angles) {
        x = std::cos(angles.psi() / 2.0f) * std::cos(angles.theta() / 2.0f) *
                std::sin(angles.phi() / 2.0f) -
            std::sin(angles.psi() / 2.0f) * std::sin(angles.theta() / 2.0f) *
                std::cos(angles.phi() / 2.0f);
        y = std::cos(angles.psi() / 2.0f) * std::sin(angles.theta() / 2.0f) *
                std::cos(angles.phi() / 2.0f) +
            std::sin(angles.psi() / 2.0f) * std::cos(angles.theta() / 2.0f) *
                std::sin(angles.phi() / 2.0f);
        z = std::sin(angles.psi() / 2.0f) * std::cos(angles.theta() / 2.0f) *
                std::cos(angles.phi() / 2.0f) -
            std::cos(angles.psi() / 2.0f) * std::sin(angles.theta() / 2.0f) *
                std::sin(angles.phi() / 2.0f);

        float qu0 =
            std::cos(angles.psi() / 2.0f) * std::cos(angles.theta() / 2.0f) *
                std::cos(angles.phi() / 2.0f) +
            std::sin(angles.psi() / 2.0f) * std::sin(angles.theta() / 2.0f) *
                std::sin(angles.phi() / 2.0f);
        if (qu0 < 0) {
            qu0 = -qu0;
            x = -x;
            y = -y;
            z = -z;
        }
        u0 = static_cast<std::uint16_t>(std::min(1.0f, qu0) * 65535);
    };

    constexpr DISQuat() : u0(0), x(0), y(0), z(0) {}
    constexpr DISQuat(std::uint16_t u0, float x, float y, float z)
        : u0(u0), x(x), y(y), z(z) {}
};

class SimulationAddress {
   public:
    std::uint16_t site_id;
    std::uint16_t application_id;

    constexpr SimulationAddress() : site_id(0), application_id(0) {}
    constexpr SimulationAddress(std::uint16_t site_id,
                                std::uint16_t application_id)
        : site_id(site_id), application_id(application_id) {}

    constexpr std::uint16_t site() const noexcept { return site_id; }
    constexpr std::uint16_t application() const noexcept {
        return application_id;
    }
};

struct EntityType {
    std::uint8_t kind;
    std::uint8_t domain;
    std::uint16_t country;
    std::uint8_t category;
    std::uint8_t subcategory;
    std::uint8_t specific;
    std::uint8_t extra;
};

struct ID {
    SimulationAddress simulation_address;
    std::uint16_t id_;

    constexpr ID() : simulation_address(), id_(0) {}
    constexpr ID(SimulationAddress simulation_address, std::uint16_t entity_id)
        : simulation_address(simulation_address), id_(entity_id) {}
    constexpr ID(std::uint16_t site_id, std::uint16_t application_id,
                 std::uint16_t entity_id)
        : simulation_address(site_id, application_id), id_(entity_id) {}

    constexpr std::uint16_t site() const noexcept {
        return simulation_address.site();
    }
    constexpr std::uint16_t application() const noexcept {
        return simulation_address.application();
    }
    constexpr std::uint16_t id() const noexcept { return id_; }
};

// struct EntityID {
//     SimulationAddress simulation_address;
//     std::uint16_t entity_id;
//
//     constexpr EntityID() : simulation_address(), entity_id(0) {}
//     constexpr EntityID(SimulationAddress simulation_address,
//                        std::uint16_t entity_id)
//         : simulation_address(simulation_address), entity_id(entity_id) {}
//     constexpr EntityID(std::uint16_t site_id, std::uint16_t application_id,
//                        std::uint16_t entity_id)
//         : simulation_address(site_id, application_id), entity_id(entity_id)
//         {}
//
//     constexpr std::uint16_t site() const noexcept {
//         return simulation_address.site();
//     }
//     constexpr std::uint16_t application() const noexcept {
//         return simulation_address.application();
//     }
//     constexpr std::uint16_t entity() const noexcept { return entity_id; }
// };

using EntityID = ID;
using ObjectID = ID;
using EventID = ID;

struct PDUHeader {
    ProtocolVersion protocol_version;
    std::uint8_t exercise_id;
    PDUType pdu_type;
    DISTimestamp timestamp;
    uint8_t protocol_family;
    uint8_t status;
};

struct FixedDRMParameters {
    EulerAngles local_angles;
};

struct RotatingDRMParameters {
    DISQuat quat;
};

struct DeadReckoningParameters {
    std::uint8_t algorithm;
    uint8_t params_type;
    union {
        FixedDRMParameters fixed;
        RotatingDRMParameters rotating;
    };
    Vector linear_acceleration;
    Vector angular_velocity;

    constexpr DeadReckoningParameters()
        : algorithm(0),
          params_type(0),
          fixed(),
          linear_acceleration(),
          angular_velocity() {}
};

struct EntityMarking {
    std::uint8_t character_set;
    std::uint8_t marking[11];
};

struct VariableParameters {
    std::uint8_t type;
    std::uint8_t data[15];

    VariableParameters() = default;
};

struct EntityStatePDU {  // DIS 6 and 7 compatible
    PDUHeader header;
    EntityID entity_id;
    std::uint8_t force_id;  // padding on DIS 7, only available in DIS 6
    EntityType entity_type;
    EntityType alternative_entity_type;
    Vector entity_linear_velocity;
    WorldCoordinates entity_location;
    EulerAngles entity_orientation;
    std::uint32_t entity_appearance;
    DeadReckoningParameters dr_parameters;
    EntityMarking entity_marking;
    std::uint32_t capabilities;  // 32 bit record
    Buffer<VariableParameters> variable_parameters;
};

struct EEFundamentalParameterData {
    float center_frequency;
    float frequency_range;
    float effective_radiated_power;
    float pulse_repetition_frequency;
    float pulse_width;
};

struct BeamData {
    float beam_azimuth_center;
    float beam_elevation_center;
    float beam_azimuth_sweep;
    float beam_elevation_sweep;
    float beam_sweep_sync;
};

struct JammingTechnique {
    std::uint8_t kind;
    std::uint8_t category;
    std::uint8_t subcategory;
    std::uint8_t specific;
};

struct TrackJam {
    EntityID track_jam_target;
    std::uint8_t emitter_number;
    std::uint8_t beam_number;
};

// on DIS6, BeamData and EEFundamentalParameterData are part of
// the same struct, but in DIS7 they're separate, so we just combine them into
// one struct for both versions since serialization and deserialization are the
// same either way
struct Beam {
    std::uint16_t beam_parameter_index;
    std::uint8_t beam_number;
    std::uint8_t number_of_targets;
    std::uint8_t beam_function;
    std::uint8_t high_density_track_jam;
    std::uint8_t beam_status;
    JammingTechnique jamming_technique;
    EEFundamentalParameterData fundamental_parameters;
    BeamData beam_data;
    std::optional<Buffer<TrackJam>> track_jams;
};
struct EmitterSystem {
    std::uint16_t emitter_name;
    std::uint8_t function;
    std::uint8_t emitter_number;
    EntityCoordinates emitter_location;
    Buffer<Beam> beams;
};

struct ElectromagneticEmissionPDU {
    PDUHeader header;
    EntityID emitter_id;
    EventID event_id;
    std::uint8_t state_update_indicator;
    Buffer<EmitterSystem> emitter_systems;
};

struct RadioType7 {
    std::uint8_t entity_kind;
    std::uint8_t domain;
    std::uint16_t country_code;
    std::uint8_t category;
    std::uint8_t subcategory;
    std::uint8_t specific;
    std::uint8_t extra;
};

struct RadioType6 {
    std::uint8_t entity_kind;
    std::uint8_t domain;
    std::uint16_t country_code;
    std::uint8_t nomenclature_version;
    std::uint16_t nomenclature;
};

union RadioType {
    RadioType6 dis6;
    RadioType7 dis7;

    RadioType() : dis7() {}
};

struct BeamAntennaPattern {  // DIS 6 and 7 compatible
    EulerAngles beam_direction;
    float azimuth_beamwidth;
    float elevation_beamwidth;
    std::uint8_t reference_system;
    float e_z;
    float e_x;
    float phase;
};

struct VariableTransmitterParameters {
    std::uint32_t type;
    Buffer<std::uint8_t> data;
};

struct ModulationType {
    std::uint16_t spread_spectrum;
    std::uint16_t major_modulation;
    std::uint16_t detail_modulation;
    std::uint16_t radio_system;
};

struct TransmitterPDU {
    PDUHeader header;
    ID radio_reference_id; // in DIS 6 this is an **entity id**
    std::uint16_t radio_number;
    RadioType radio_type; 
    std::uint8_t transmit_state;
    std::uint8_t input_source;
    WorldCoordinates antenna_location;
    EntityCoordinates relative_antenna_location;
    std::uint16_t antenna_pattern_type;
    std::uint64_t center_frequency;
    float transmit_frequency_bandwidth;
    float power;
    std::uint64_t modulation_type;
    std::uint16_t crypto_system;
    std::uint16_t crypto_key_id;
    Buffer<std::uint8_t> modulation_parameters;
    Buffer<BeamAntennaPattern> antenna_patterns;
    Buffer<VariableTransmitterParameters> variable_parameters;
};

};  // namespace csics::lvc::dis
