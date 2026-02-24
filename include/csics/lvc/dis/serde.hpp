#pragma once
#include "csics/lvc/dis/PDUs.hpp"
#include "csics/serialization/Serializer.hpp"

namespace csics::serialization {

template <typename PDU>
constexpr size_t pdu_size_calc(const PDU&);

template <>
constexpr size_t pdu_size_calc(const lvc::dis::PDUHeader&) {
    return 96 / 8;  // header is 96 bits, which is 12 bytes
}

template <>
constexpr size_t pdu_size_calc(const lvc::dis::EntityStatePDU& pdu) {
    return (1152 + 128 * pdu.variable_parameters.size()) /
           8;  // EntityStatePDU is 1152 bits + 128 bits per variable parameter
}

template <>
constexpr size_t pdu_size_calc(
    const lvc::dis::ElectromagneticEmissionPDU& pdu) {
    std::size_t total_size_bits = 224;

    for (const auto& emitter_system : pdu.emitter_systems) {
        total_size_bits += 160;  // EmitterSystem is 160 bits
        for (const auto& beam : emitter_system.beams) {
            total_size_bits += 416;  // Beam is 416 bits
            if (beam.track_jams.has_value()) {
                total_size_bits +=
                    64 * beam.track_jams->size();  // Each TrackJam is 64 bits
            }
        }
    }

    return total_size_bits / 8;  // Convert bits to bytes
}

template <>
constexpr size_t pdu_size_calc(const lvc::dis::TransmitterPDU& pdu) {
    size_t m = pdu.modulation_parameters.size() + 8 -
               (pdu.modulation_parameters.size() %
                8);  // modulation parameters are padded to the next byte
    size_t size_bits = 832 + m * 8 + pdu.antenna_patterns.size() * 320;

    for (const auto& var_param : pdu.variable_parameters) {
        // data: K_i + P_i, where K_i is the size of the data in bits and P_i is
        // the padding
        size_bits +=
            var_param.data.size() * 8 + (8 - (var_param.data.size() % 8));
    }

    return size_bits / 8;  // convert bits to bytes
}

template <WireSerializer S>
constexpr void serialize_pdu_header(S& s, MutableBufferView& bv,
                                    const lvc::dis::PDUHeader& pdu,
                                    std::size_t pdu_size) {
    auto bv_ = bv;
    s.write(bv_, pdu.protocol_version);
    s.write(bv_, pdu.exercise_id);
    s.write(bv_, pdu.pdu_type);
    s.write(bv_, pdu.protocol_family);
    s.write(bv_, be<std::uint32_t>(pdu.timestamp.raw()));
    s.write(bv_, be<std::uint16_t>(pdu_size));
    s.write(bv_, pdu.status);
    s.pad(bv_, 1);
};

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const lvc::dis::EntityID& entity_id) {
    auto bv_ = bv;
    s.write(bv_, be<std::uint16_t>(entity_id.site()));
    s.write(bv_, be<std::uint16_t>(entity_id.application()));
    s.write(bv_, be<std::uint16_t>(entity_id.id()));
    return {bv_(0, bv.size() - bv_.size()), SerializationStatus::Ok};
}

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const lvc::dis::EntityType& entity_type) {
    auto bv_ = bv;
    s.write(bv_, entity_type.kind);
    s.write(bv_, entity_type.domain);
    s.write(bv_, be<std::uint16_t>(entity_type.country));
    s.write(bv_, entity_type.category);
    s.write(bv_, entity_type.subcategory);
    s.write(bv_, entity_type.specific);
    s.write(bv_, entity_type.extra);
    return {bv_(0, bv.size() - bv_.size()), SerializationStatus::Ok};
}

template <WireSerializer S>
constexpr SerializationResult serialize_wire(S& s, MutableBufferView& bv,
                                             const lvc::dis::Vector& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<float>(pdu.get<0>()));
    s.write(bv_, be<float>(pdu.get<1>()));
    s.write(bv_, be<float>(pdu.get<2>()));
    return {bv_(0, bv.size() - bv_.size()), SerializationStatus::Ok};
};

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const lvc::dis::WorldCoordinates& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<double>(pdu.x()));
    s.write(bv_, be<double>(pdu.y()));
    s.write(bv_, be<double>(pdu.z()));
    return {bv_(0, bv.size() - bv_.size()), SerializationStatus::Ok};
};

template <WireSerializer S>
constexpr SerializationResult serialize_wire(S& s, MutableBufferView& bv,
                                             const lvc::dis::EulerAngles& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<float>(pdu.psi()));
    s.write(bv_, be<float>(pdu.theta()));
    s.write(bv_, be<float>(pdu.phi()));
    return {bv_(0, bv.size() - bv_.size()), SerializationStatus::Ok};
};

template <WireSerializer S>
constexpr SerializationResult serialize_wire(S& s, MutableBufferView& bv,
                                             const lvc::dis::DISQuat& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<std::uint16_t>(pdu.u0));
    s.write(bv_, be<float>(pdu.x));
    s.write(bv_, be<float>(pdu.y));
    s.write(bv_, be<float>(pdu.z));
    return {bv_(0, bv.size() - bv_.size()), SerializationStatus::Ok};
};

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const lvc::dis::DeadReckoningParameters& pdu) {
    auto bv_ = bv;
    s.write(bv_, pdu.algorithm);
    s.write(bv_, pdu.params_type);
    if (pdu.params_type == 1) {
        s.pad(bv_, 2);  // pad to align the fixed parameters
        serialize_wire(s, bv_, pdu.fixed.local_angles);
    } else if (pdu.params_type == 2) {
        serialize_wire(s, bv_, pdu.rotating.quat);
    } else {
        // this should never happen, but if it does, we just write zeros for the
        // parameters
        s.pad(bv_, 8);  // pad to align the linear acceleration
    }
    serialize_wire(s, bv_, pdu.linear_acceleration);
    serialize_wire(s, bv_, pdu.angular_velocity);
    return {bv_(0, bv.size() - bv_.size()), SerializationStatus::Ok};
};

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const lvc::dis::EntityStatePDU& pdu) {
    auto bv_ = bv;
    serialize_pdu_header(s, bv_, pdu.header, pdu_size_calc(pdu));
    serialize_wire(s, bv_, pdu.entity_id);
    s.write(bv_, pdu.force_id);
    s.write(bv_, std::uint8_t(pdu.variable_parameters.size()));
    serialize_wire(s, bv_, pdu.entity_type);
    serialize_wire(s, bv_, pdu.alternative_entity_type);
    serialize_wire(s, bv_, pdu.entity_linear_velocity);
    serialize_wire(s, bv_, pdu.entity_location);
    serialize_wire(s, bv_, pdu.entity_orientation);
    s.write(bv_, pdu.entity_appearance);  // for now, we just write the raw 32
                                          // bit value of the appearance
    serialize_wire(s, bv_, pdu.dr_parameters);
    s.write(bv_,
            pdu.entity_marking.character_set);  // just 12 bytes in a row, no
                                                // endianness to worry about
    s.write(bv_, pdu.entity_marking.marking, 11);
    s.write(bv_, pdu.capabilities);  // for now, we just write the raw 32 bit
                                     // value of the capabilities
    for (const auto& var_param : pdu.variable_parameters) {
        s.write(bv_, var_param.type);
        s.write(bv_, var_param.data, 15);
    }
    return {bv_(0, bv.size() - bv_.size()), SerializationStatus::Ok};
};

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv,
    const lvc::dis::EEFundamentalParameterData& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<float>(pdu.center_frequency));
    s.write(bv_, be<float>(pdu.frequency_range));
    s.write(bv_, be<float>(pdu.effective_radiated_power));
    s.write(bv_, be<float>(pdu.pulse_repetition_frequency));
    s.write(bv_, be<float>(pdu.pulse_width));
    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
}

template <WireSerializer S>
constexpr SerializationResult serialize_wire(S& s, MutableBufferView& bv,
                                             const lvc::dis::BeamData& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<float>(pdu.beam_azimuth_center));
    s.write(bv_, be<float>(pdu.beam_elevation_center));
    s.write(bv_, be<float>(pdu.beam_azimuth_sweep));
    s.write(bv_, be<float>(pdu.beam_elevation_sweep));
    s.write(bv_, be<float>(pdu.beam_sweep_sync));
    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
};

template <WireSerializer S>
constexpr SerializationResult serialize_radio_type(
    S& s, MutableBufferView& bv, const lvc::dis::RadioType& pdu,
    lvc::dis::ProtocolVersion protocol_version) {
    auto bv_ = bv;
    if (protocol_version == lvc::dis::ProtocolVersion::IEEE_1278_1998) {
        s.write(bv_, pdu.dis6.entity_kind);
        s.write(bv_, pdu.dis6.domain);
        s.write(bv_, be<std::uint16_t>(pdu.dis6.country_code));
        s.write(bv_, pdu.dis6.nomenclature_version);
        s.write(bv_, be<std::uint16_t>(pdu.dis6.nomenclature));
    } else if (protocol_version == lvc::dis::ProtocolVersion::IEEE_1278_2012) {
        s.write(bv_, pdu.dis7.entity_kind);
        s.write(bv_, pdu.dis7.domain);
        s.write(bv_, be<std::uint16_t>(pdu.dis7.country_code));
        s.write(bv_, pdu.dis7.category);
        s.write(bv_, pdu.dis7.subcategory);
        s.write(bv_, pdu.dis7.specific);
        s.write(bv_, pdu.dis7.extra);
    } else {
        // this should never happen, but if it does, we just write zeros for the
        // radio type. Doesn't really affect the rest of serialization but needs
        // to be reported.
        s.pad(bv_, 8);  // pad to align the next field
        return {bv_(0, bv_.size() - bv_.size()),
                SerializationStatus::NonFatalError};
    }
    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
};

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const lvc::dis::JammingTechnique& pdu) {
    auto bv_ = bv;
    s.write(bv_, pdu.kind);
    s.write(bv_, pdu.category);
    s.write(bv_, pdu.subcategory);
    s.write(bv_, pdu.specific);
    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
}

template <WireSerializer S>
constexpr SerializationResult serialize_wire(S& s, MutableBufferView& bv,
                                             const lvc::dis::TrackJam& pdu) {
    auto bv_ = bv;
    serialize_wire(s, bv_, pdu.track_jam_target);
    s.write(bv_, pdu.emitter_number);
    s.write(bv_, pdu.beam_number);
    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
}

template <WireSerializer S>
constexpr SerializationResult serialize_wire(S& s, MutableBufferView& bv,
                                             const lvc::dis::Beam& pdu) {
    auto bv_ = bv;
    std::size_t track_jams_size =
        pdu.track_jams.has_value() ? pdu.track_jams->size() : 0;
    s.write(bv_, std::uint8_t(416 + 64 * track_jams_size) /
                     32);  // beam record length in 32 bit words
    s.write(bv_, pdu.beam_number);
    s.write(bv_, be<std::uint16_t>(pdu.beam_parameter_index));
    serialize_wire(s, bv_, pdu.fundamental_parameters);
    serialize_wire(s, bv_, pdu.beam_data);
    serialize_wire(s, bv_, pdu.beam_function);
    serialize_wire(s, bv_, pdu.number_of_targets);
    serialize_wire(s, bv_, pdu.high_density_track_jam);
    serialize_wire(s, bv_, pdu.beam_status);
    serialize_wire(s, bv_, pdu.jamming_technique);

    if (pdu.track_jams.has_value()) {
        for (const auto& track_jam : *pdu.track_jams) {
            serialize_wire(s, bv_, track_jam);
        }
    }

    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
}

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const lvc::dis::EmitterSystem& pdu) {
    auto bv_ = bv;

    size_t beams_size = 0;

    for (const auto& beam : pdu.beams) {
        beams_size +=
            416 +
            64 * (beam.track_jams.has_value() ? beam.track_jams->size() : 0);
    }
    s.write(bv_, (160 + beams_size) /
                     32);  // emitter system record length in 32 bit words
    s.write(bv_, std::uint8_t(pdu.beams.size()));
    s.pad(bv_, 2);
    s.write(bv_, be<std::uint16_t>(pdu.emitter_name));
    s.write(bv_, pdu.function);
    s.write(bv_, pdu.emitter_number);
    serialize_wire(s, bv_, pdu.emitter_location);

    for (const auto& beam : pdu.beams) {
        serialize_wire(s, bv_, beam);
    }

    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
}

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv,
    const lvc::dis::ElectromagneticEmissionPDU& pdu) {
    auto bv_ = bv;
    serialize_pdu_header(s, bv_, pdu.header, pdu_size_calc(pdu));
    serialize_wire(s, bv_, pdu.emitter_id);
    serialize_wire(s, bv_, pdu.event_id);
    s.write(bv_, pdu.state_update_indicator);
    s.write(bv_, std::uint8_t(pdu.emitter_systems.size()));
    s.pad(bv_, 2);

    for (const auto& emitter_system : pdu.emitter_systems) {
        serialize_wire(s, bv_, emitter_system);
    }

    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
}

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const lvc::dis::ModulationType& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<std::uint16_t>(pdu.spread_spectrum));
    s.write(bv_, be<std::uint16_t>(pdu.major_modulation));
    s.write(bv_, be<std::uint16_t>(pdu.detail_modulation));
    s.write(bv_, be<std::uint16_t>(pdu.radio_system));
    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
}

template <WireSerializer S>
constexpr SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const lvc::dis::TransmitterPDU& pdu) {
    auto bv_ = bv;
    serialize_pdu_header(s, bv_, pdu.header, pdu_size_calc(pdu));
    serialize_wire(s, bv_, pdu.radio_reference_id);
    s.write(bv_, be<std::uint16_t>(pdu.radio_number));
    serialize_wire(s, bv_, pdu.radio_type);
    s.write(bv_, std::uint8_t(pdu.transmit_state));
    s.write(bv_, std::uint8_t(pdu.input_source));
    if (pdu.header.protocol_version ==
        lvc::dis::ProtocolVersion::IEEE_1278_1998) {
        s.pad(bv_, 2);  // pad to align the next field
    } else {
        // DIS 7 has a variable parameter count field here that DIS 6 doesn't
        // have, so we need to write it out for DIS 7 and pad for DIS 6
        s.write(bv_, be<std::uint16_t>(pdu.variable_parameters.size()));
    }
    serialize_wire(s, bv_, pdu.antenna_location);
    serialize_wire(s, bv_, pdu.relative_antenna_location);
    s.write(bv_, be<std::uint16_t>(pdu.antenna_pattern_type));
    s.write(bv_, be<std::uint16_t>(pdu.antenna_patterns.size()));
    s.write(bv_, be<std::uint64_t>(pdu.center_frequency));
    s.write(bv_, be<float>(pdu.transmit_frequency_bandwidth));
    s.write(bv_, be<float>(pdu.power));
    serialize_wire(s, bv_, pdu.modulation_type);
    s.write(bv_, be<std::uint16_t>(pdu.crypto_system));
    s.write(bv_, be<std::uint16_t>(pdu.crypto_key_id));
    s.write(bv_, std::uint8_t(pdu.modulation_parameters.size()));
    s.pad(bv, 3);
    for (const auto& mod_param : pdu.modulation_parameters) {
        s.write(bv_,
                std::uint8_t(mod_param));  // for now they're represented as raw
                                           // bytes rather than structs.
    }

    for (const auto& antenna_pattern : pdu.antenna_patterns) {
        serialize_wire(s, bv_, antenna_pattern);
    }

    for (const auto& var_param : pdu.variable_parameters) {
        size_t required_padding =
            (8 - (var_param.data.size() % 8)) %
            8;  // calculate padding needed to align to next 8 byte boundary
        s.write(bv_, be<std::uint32_t>(var_param.type));
        s.write(bv_,
                be<std::uint16_t>(
                    6 + var_param.data.size() +
                    required_padding));  // length of the variable parameter
                                         // record in bytes (6 bytes for the
                                         // type and length fields, plus the
                                         // data and padding)
        for (const auto& byte : var_param.data) {
            s.write(bv_, byte);
        }
        s.pad(bv, required_padding);  // pad to align the data to the next 8
                                      // byte boundary
    }
    return {bv_(0, bv_.size() - bv_.size()), SerializationStatus::Ok};
}

};  // namespace csics::serialization
