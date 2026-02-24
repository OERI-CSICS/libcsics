#pragma once
#include "PDUs.hpp"
#include "csics/lvc/dis/PDUs.hpp"
#include "csics/serialization/serialization.hpp"

namespace csics::lvc::dis {

template <typename PDU>
constexpr size_t pdu_size_calc(const PDU&);

template <>
constexpr inline size_t pdu_size_calc(const PDUHeader&) {
    return 96 / 8;  // header is 96 bits, which is 12 bytes
}

template <>
constexpr inline size_t pdu_size_calc(const EntityStatePDU& pdu) {
    return (1152 + 128 * pdu.variable_parameters.size()) /
           8;  // EntityStatePDU is 1152 bits + 128 bits per variable parameter
}

template <>
inline size_t pdu_size_calc(const ElectromagneticEmissionPDU& pdu) {
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
inline size_t pdu_size_calc(const TransmitterPDU& pdu) {
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

template <serialization::WireSerializer S>
constexpr void serialize_pdu_header(S& s, MutableBufferView& bv,
                                    const PDUHeader& pdu,
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

template <serialization::Deserializer D>
inline expected<PDUHeader, typename D::error_type> deserialize_direct(D& d) {
    auto protocol_version = d.template read<std::uint8_t>();
    auto exercise_id = d.template read<std::uint8_t>();
    auto pdu_type = d.template read<std::uint8_t>();
    auto protocol_family = d.template read<std::uint8_t>();
    auto timestamp = d.template read<be<std::uint32_t>>();
    auto pdu_length = d.template read<be<std::uint16_t>>();
    auto status = d.template read<std::uint8_t>();
    d.pad(1);

    if (!protocol_version || !exercise_id || !pdu_type || !protocol_family ||
        !timestamp || !pdu_length || !status) {
        return unexpect;
    }

    return PDUHeader{*protocol_version, *exercise_id,     *pdu_type,
                     *protocol_family,  Time(*timestamp), *status};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const EntityID& entity_id) {
    auto bv_ = bv;
    s.write(bv_, be<std::uint16_t>(entity_id.site()));
    s.write(bv_, be<std::uint16_t>(entity_id.application()));
    s.write(bv_, be<std::uint16_t>(entity_id.id()));
    return {bv_(0, bv.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<EntityID, typename D::error_type> deserialize_direct(D& d) {
    auto site_id = d.template read<be<std::uint16_t>>();
    auto application_id = d.template read<be<std::uint16_t>>();
    auto entity_id = d.template read<be<std::uint16_t>>();

    if (!site_id || !application_id || !entity_id) {
        return unexpect;
    }

    return EntityID(*site_id, *application_id, *entity_id);
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const EntityType& entity_type) {
    auto bv_ = bv;
    s.write(bv_, entity_type.kind);
    s.write(bv_, entity_type.domain);
    s.write(bv_, be<std::uint16_t>(entity_type.country));
    s.write(bv_, entity_type.category);
    s.write(bv_, entity_type.subcategory);
    s.write(bv_, entity_type.specific);
    s.write(bv_, entity_type.extra);
    return {bv_(0, bv.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<EntityType, typename D::error_type> deserialize_direct(D& d) {
    auto kind = d.template read<std::uint8_t>();
    auto domain = d.template read<std::uint8_t>();
    auto country = d.template read<be<std::uint16_t>>();
    auto category = d.template read<std::uint8_t>();
    auto subcategory = d.template read<std::uint8_t>();
    auto specific = d.template read<std::uint8_t>();
    auto extra = d.template read<std::uint8_t>();

    if (!kind || !domain || !country || !category || !subcategory ||
        !specific || !extra) {
        return unexpect;
    }

    return EntityType{*kind,        *domain,   *country, *category,
                      *subcategory, *specific, *extra};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const Vector& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<float>(pdu.get<0>()));
    s.write(bv_, be<float>(pdu.get<1>()));
    s.write(bv_, be<float>(pdu.get<2>()));
    return {bv_(0, bv.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
};

template <serialization::Deserializer D>
inline expected<Vector, typename D::error_type> deserialize_direct(D& d) {
    auto x = d.template read<be<float>>();
    auto y = d.template read<be<float>>();
    auto z = d.template read<be<float>>();

    if (!x || !y || !z) {
        return unexpect;
    }

    return Vector(*x, *y, *z);
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const WorldCoordinates& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<double>(pdu.x()));
    s.write(bv_, be<double>(pdu.y()));
    s.write(bv_, be<double>(pdu.z()));
    return {bv_(0, bv.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
};

template <serialization::Deserializer D>
inline expected<WorldCoordinates, typename D::error_type> deserialize_direct(
    D& d) {
    auto x = d.template read<be<double>>();
    auto y = d.template read<be<double>>();
    auto z = d.template read<be<double>>();

    if (!x || !y || !z) {
        return unexpect;
    }

    return WorldCoordinates(*x, *y, *z);
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const EulerAngles& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<float>(pdu.psi()));
    s.write(bv_, be<float>(pdu.theta()));
    s.write(bv_, be<float>(pdu.phi()));
    return {bv_(0, bv.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
};

template <serialization::Deserializer D>
inline expected<EulerAngles, typename D::error_type> deserialize_direct(D& d) {
    auto psi = d.template read<be<float>>();
    auto theta = d.template read<be<float>>();
    auto phi = d.template read<be<float>>();

    if (!psi || !theta || !phi) {
        return unexpect;
    }

    return EulerAngles(*psi, *theta, *phi);
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const DISQuat& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<std::uint16_t>(pdu.u0));
    s.write(bv_, be<float>(pdu.x));
    s.write(bv_, be<float>(pdu.y));
    s.write(bv_, be<float>(pdu.z));
    return {bv_(0, bv.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
};

template <serialization::Deserializer D>
inline expected<DISQuat, typename D::error_type> deserialize_direct(D& d) {
    auto u0 = d.template read<be<std::uint16_t>>();
    auto x = d.template read<be<float>>();
    auto y = d.template read<be<float>>();
    auto z = d.template read<be<float>>();

    if (!u0 || !x || !y || !z) {
        return unexpect;
    }

    return DISQuat{*u0, *x, *y, *z};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const DeadReckoningParameters& pdu) {
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
    return {bv_(0, bv.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
};

template <serialization::Deserializer D>
inline expected<DeadReckoningParameters, typename D::error_type>
deserialize_direct(D& d) {
    auto algorithm = d.template read<std::uint8_t>();
    auto params_type = d.template read<std::uint8_t>();

    if (!algorithm || !params_type) {
        return unexpect;
    }

    DeadReckoningParameters params;
    params.algorithm = *algorithm;
    params.params_type = *params_type;

    if (params.params_type == 1) {
        d.pad(2);  // pad to align the fixed parameters
        auto local_angles = deserialize_direct<EulerAngles>(d);
        if (!local_angles) {
            return unexpect;
        }
        params.fixed.local_angles = *local_angles;
    } else if (params.params_type == 2) {
        auto quat = deserialize_direct<DISQuat>(d);
        if (!quat) {
            return unexpect;
        }
        params.rotating.quat = *quat;
    } else {
        // this should never happen, but if it does, we just leave the
        // parameters as zeros
        d.pad(8);  // pad to align the linear acceleration
    }

    auto linear_acceleration = deserialize_direct<Vector>(d);
    auto angular_velocity = deserialize_direct<Vector>(d);

    if (!linear_acceleration || !angular_velocity) {
        return unexpect;
    }

    params.linear_acceleration = *linear_acceleration;
    params.angular_velocity = *angular_velocity;

    return params;
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const EntityStatePDU& pdu) {
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
    return {bv_(0, bv.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
};

template <serialization::Deserializer D>
inline expected<EntityStatePDU, typename D::error_type> deserialize_direct(
    D& d) {
    auto header = deserialize_direct<PDUHeader>(d);
    auto entity_id = deserialize_direct<EntityID>(d);
    auto force_id = d.template read<std::uint8_t>();
    auto num_variable_parameters = d.template read<std::uint8_t>();
    auto entity_type = deserialize_direct<EntityType>(d);
    auto alternative_entity_type = deserialize_direct<EntityType>(d);
    auto entity_linear_velocity = deserialize_direct<Vector>(d);
    auto entity_location = deserialize_direct<WorldCoordinates>(d);
    auto entity_orientation = deserialize_direct<EulerAngles>(d);
    auto entity_appearance = d.template read<std::uint32_t>();
    auto dr_parameters = deserialize_direct<DeadReckoningParameters>(d);
    auto marking_character_set = d.template read<std::uint8_t>();

    std::array<std::uint8_t, 11> marking;
    for (std::size_t i = 0; i < 11; ++i) {
        auto byte = d.template read<std::uint8_t>();
        if (!byte) {
            return unexpect;
        }
        marking[i] = *byte;
    }

    auto capabilities = d.template read<std::uint32_t>();

    Buffer<VariableParameters> variable_parameters;
    for (std::size_t i = 0; i < *num_variable_parameters; ++i) {
        auto type = d.template read<std::uint8_t>();
        std::array<std::uint8_t, 15> data;
        for (std::size_t j = 0; j < 15; ++j) {
            auto byte = d.template read<std::uint8_t>();
            if (!byte) {
                return unexpect;
            }
            data[j] = *byte;
        }
        variable_parameters.push_back(VariableParameters{*type, data});
    }

    if (!header || !entity_id || !force_id || !entity_type ||
        !alternative_entity_type || !entity_linear_velocity ||
        !entity_location || !entity_orientation || !entity_appearance ||
        !dr_parameters || !marking_character_set || !capabilities) {
        return unexpect;
    }

    EntityStatePDU pdu{*header,
                       *entity_id,
                       *force_id,
                       *entity_type,
                       *alternative_entity_type,
                       *entity_linear_velocity,
                       *entity_location,
                       *entity_orientation,
                       *entity_appearance,
                       *dr_parameters,
                       EntityMarking{*marking_character_set, marking},
                       *capabilities,
                       std::move(variable_parameters)};
};

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const EEFundamentalParameterData& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<float>(pdu.center_frequency));
    s.write(bv_, be<float>(pdu.frequency_range));
    s.write(bv_, be<float>(pdu.effective_radiated_power));
    s.write(bv_, be<float>(pdu.pulse_repetition_frequency));
    s.write(bv_, be<float>(pdu.pulse_width));
    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<EEFundamentalParameterData, typename D::error_type>
deserialize_direct(D& d) {
    auto center_frequency = d.template read<be<float>>();
    auto frequency_range = d.template read<be<float>>();
    auto effective_radiated_power = d.template read<be<float>>();
    auto pulse_repetition_frequency = d.template read<be<float>>();
    auto pulse_width = d.template read<be<float>>();

    if (!center_frequency || !frequency_range || !effective_radiated_power ||
        !pulse_repetition_frequency || !pulse_width) {
        return unexpect;
    }

    return EEFundamentalParameterData{
        *center_frequency, *frequency_range, *effective_radiated_power,
        *pulse_repetition_frequency, *pulse_width};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const BeamData& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<float>(pdu.beam_azimuth_center));
    s.write(bv_, be<float>(pdu.beam_elevation_center));
    s.write(bv_, be<float>(pdu.beam_azimuth_sweep));
    s.write(bv_, be<float>(pdu.beam_elevation_sweep));
    s.write(bv_, be<float>(pdu.beam_sweep_sync));
    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
};

template <serialization::Deserializer D>
inline expected<BeamData, typename D::error_type> deserialize_direct(D& d) {
    auto beam_azimuth_center = d.template read<be<float>>();
    auto beam_elevation_center = d.template read<be<float>>();
    auto beam_azimuth_sweep = d.template read<be<float>>();
    auto beam_elevation_sweep = d.template read<be<float>>();
    auto beam_sweep_sync = d.template read<be<float>>();

    if (!beam_azimuth_center || !beam_elevation_center || !beam_azimuth_sweep ||
        !beam_elevation_sweep || !beam_sweep_sync) {
        return unexpect;
    }

    return BeamData{*beam_azimuth_center, *beam_elevation_center,
                    *beam_azimuth_sweep, *beam_elevation_sweep,
                    *beam_sweep_sync};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_radio_type(
    S& s, MutableBufferView& bv, const RadioType& pdu,
    ProtocolVersion protocol_version) {
    auto bv_ = bv;
    if (protocol_version == ProtocolVersion::IEEE_1278_1998) {
        s.write(bv_, pdu.dis6.entity_kind);
        s.write(bv_, pdu.dis6.domain);
        s.write(bv_, be<std::uint16_t>(pdu.dis6.country_code));
        s.write(bv_, pdu.dis6.nomenclature_version);
        s.write(bv_, be<std::uint16_t>(pdu.dis6.nomenclature));
    } else if (protocol_version == ProtocolVersion::IEEE_1278_2012) {
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
                serialization::SerializationStatus::NonFatalError};
    }
    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
};

template <serialization::Deserializer D>
inline expected<RadioType, typename D::error_type> deserialize_radio_type(
    D& d, ProtocolVersion protocol_version) {
    RadioType radio_type;
    if (protocol_version == ProtocolVersion::IEEE_1278_1998) {
        auto entity_kind = d.template read<std::uint8_t>();
        auto domain = d.template read<std::uint8_t>();
        auto country_code = d.template read<be<std::uint16_t>>();
        auto nomenclature_version = d.template read<std::uint8_t>();
        auto nomenclature = d.template read<be<std::uint16_t>>();

        if (!entity_kind || !domain || !country_code || !nomenclature_version ||
            !nomenclature) {
            return unexpect;
        }

        radio_type.dis6.entity_kind = *entity_kind;
        radio_type.dis6.domain = *domain;
        radio_type.dis6.country_code = *country_code;
        radio_type.dis6.nomenclature_version = *nomenclature_version;
        radio_type.dis6.nomenclature = *nomenclature;
    } else if (protocol_version == ProtocolVersion::IEEE_1278_2012) {
        auto entity_kind = d.template read<std::uint8_t>();
        auto domain = d.template read<std::uint8_t>();
        auto country_code = d.template read<be<std::uint16_t>>();
        auto category = d.template read<std::uint8_t>();
        auto subcategory = d.template read<std::uint8_t>();
        auto specific = d.template read<std::uint8_t>();
        auto extra = d.template read<std::uint8_t>();

        if (!entity_kind || !domain || !country_code || !category ||
            !subcategory || !specific || !extra) {
            return unexpect;
        }

        radio_type.dis7.entity_kind = *entity_kind;
        radio_type.dis7.domain = *domain;
        radio_type.dis7.country_code = *country_code;
        radio_type.dis7.category = *category;
        radio_type.dis7.subcategory = *subcategory;
        radio_type.dis7.specific = *specific;
        radio_type.dis7.extra = *extra;
    } else {
        return unexpect;
    };
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const JammingTechnique& pdu) {
    auto bv_ = bv;
    s.write(bv_, pdu.kind);
    s.write(bv_, pdu.category);
    s.write(bv_, pdu.subcategory);
    s.write(bv_, pdu.specific);
    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<JammingTechnique, typename D::error_type> deserialize_direct(
    D& d) {
    auto kind = d.template read<std::uint8_t>();
    auto category = d.template read<std::uint8_t>();
    auto subcategory = d.template read<std::uint8_t>();
    auto specific = d.template read<std::uint8_t>();

    if (!kind || !category || !subcategory || !specific) {
        return unexpect;
    }

    return JammingTechnique{*kind, *category, *subcategory, *specific};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const TrackJam& pdu) {
    auto bv_ = bv;
    serialize_wire(s, bv_, pdu.track_jam_target);
    s.write(bv_, pdu.emitter_number);
    s.write(bv_, pdu.beam_number);
    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<TrackJam, typename D::error_type> deserialize_direct(D& d) {
    auto track_jam_target = deserialize_direct<EntityID>(d);
    auto emitter_number = d.template read<std::uint8_t>();
    auto beam_number = d.template read<std::uint8_t>();

    if (!track_jam_target || !emitter_number || !beam_number) {
        return unexpect;
    }

    return TrackJam{*track_jam_target, *emitter_number, *beam_number};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const Beam& pdu) {
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

    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<Beam, typename D::error_type> deserialize_direct(D& d) {
    auto record_length = d.template read<std::uint8_t>();
    auto beam_number = d.template read<std::uint8_t>();
    auto beam_parameter_index = d.template read<be<std::uint16_t>>();
    auto fundamental_parameters =
        deserialize_direct<EEFundamentalParameterData>(d);
    auto beam_data = deserialize_direct<BeamData>(d);
    auto beam_function = d.template read<std::uint8_t>();
    auto number_of_targets = d.template read<std::uint8_t>();
    auto high_density_track_jam = d.template read<std::uint8_t>();
    auto beam_status = d.template read<std::uint8_t>();
    auto jamming_technique = deserialize_direct<JammingTechnique>(d);

    if (!record_length || !beam_number || !beam_parameter_index ||
        !fundamental_parameters || !beam_data || !beam_function ||
        !number_of_targets || !high_density_track_jam || !beam_status ||
        !jamming_technique) {
        return unexpect;
    }

    // calculate how many track jams there are based on the record length
    size_t track_jams_size =
        (*record_length * 32 - 416) / 64;  // each track jam is 64 bits

    std::optional<Buffer<TrackJam>> track_jams;
    if (track_jams_size > 0) {
        Buffer<TrackJam> jams;
        for (size_t i = 0; i < track_jams_size; ++i) {
            auto track_jam = deserialize_direct<TrackJam>(d);
            if (!track_jam) {
                return unexpect;
            }
            jams.push_back(*track_jam);
        }
        track_jams = std::move(jams);
    }

    return Beam{*beam_parameter_index,
                *beam_number,
                *number_of_targets,
                *beam_function,
                *high_density_track_jam,
                *beam_status,
                *jamming_technique,
                *fundamental_parameters,
                *beam_data,
                std::move(track_jams)};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const EmitterSystem& pdu) {
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

    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<EmitterSystem, typename D::error_type> deserialize_direct(
    D& d) {
    auto record_length = d.template read<std::uint8_t>();
    auto num_beams = d.template read<std::uint8_t>();
    d.pad(2);
    auto emitter_name = d.template read<be<std::uint16_t>>();
    auto function = d.template read<std::uint8_t>();
    auto emitter_number = d.template read<std::uint8_t>();
    auto emitter_location = deserialize_direct<WorldCoordinates>(d);

    if (!record_length || !num_beams || !emitter_name || !function ||
        !emitter_number || !emitter_location) {
        return unexpect;
    }

    Buffer<Beam> beams;
    for (size_t i = 0; i < *num_beams; ++i) {
        auto beam = deserialize_direct<Beam>(d);
        if (!beam) {
            return unexpect;
        }
        beams.push_back(*beam);
    }

    return EmitterSystem{*emitter_name, *function, *emitter_number,
                         *emitter_location, std::move(beams)};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const ElectromagneticEmissionPDU& pdu) {
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

    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<ElectromagneticEmissionPDU, typename D::error_type>
deserialize_direct(D& d) {
    auto header = deserialize_direct<PDUHeader>(d);
    auto emitter_id = deserialize_direct<EntityID>(d);
    auto event_id = deserialize_direct<EventID>(d);
    auto state_update_indicator = d.template read<std::uint8_t>();
    auto num_emitter_systems = d.template read<std::uint8_t>();
    d.pad(2);

    if (!header || !emitter_id || !event_id || !state_update_indicator ||
        !num_emitter_systems) {
        return unexpect;
    }

    Buffer<EmitterSystem> emitter_systems;
    for (size_t i = 0; i < *num_emitter_systems; ++i) {
        auto emitter_system = deserialize_direct<EmitterSystem>(d);
        if (!emitter_system) {
            return unexpect;
        }
        emitter_systems.push_back(*emitter_system);
    }

    return ElectromagneticEmissionPDU{*header, *emitter_id, *event_id,
                                      *state_update_indicator,
                                      std::move(emitter_systems)};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const ModulationType& pdu) {
    auto bv_ = bv;
    s.write(bv_, be<std::uint16_t>(pdu.spread_spectrum));
    s.write(bv_, be<std::uint16_t>(pdu.major_modulation));
    s.write(bv_, be<std::uint16_t>(pdu.detail_modulation));
    s.write(bv_, be<std::uint16_t>(pdu.radio_system));
    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<ModulationType, typename D::error_type> deserialize_direct(
    D& d) {
    auto spread_spectrum = d.template read<be<std::uint16_t>>();
    auto major_modulation = d.template read<be<std::uint16_t>>();
    auto detail_modulation = d.template read<be<std::uint16_t>>();
    auto radio_system = d.template read<be<std::uint16_t>>();

    if (!spread_spectrum || !major_modulation || !detail_modulation ||
        !radio_system) {
        return unexpect;
    }

    return ModulationType{*spread_spectrum, *major_modulation,
                          *detail_modulation, *radio_system};
}

template <serialization::WireSerializer S>
constexpr serialization::SerializationResult serialize_wire(
    S& s, MutableBufferView& bv, const TransmitterPDU& pdu) {
    auto bv_ = bv;
    serialize_pdu_header(s, bv_, pdu.header, pdu_size_calc(pdu));
    serialize_wire(s, bv_, pdu.radio_reference_id);
    s.write(bv_, be<std::uint16_t>(pdu.radio_number));
    serialize_wire(s, bv_, pdu.radio_type);
    s.write(bv_, std::uint8_t(pdu.transmit_state));
    s.write(bv_, std::uint8_t(pdu.input_source));
    if (pdu.header.protocol_version == ProtocolVersion::IEEE_1278_1998) {
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
    return {bv_(0, bv_.size() - bv_.size()),
            serialization::SerializationStatus::Ok};
}

template <serialization::Deserializer D>
inline expected<TransmitterPDU, typename D::error_type> deserialize_direct(
    D& d) {
    auto header = deserialize_direct<PDUHeader>(d);
    auto radio_reference_id = deserialize_direct<EntityID>(d);
    auto radio_number = d.template read<be<std::uint16_t>>();
    auto radio_type = deserialize_radio_type(d, header->protocol_version);
    auto transmit_state = d.template read<std::uint8_t>();
    auto input_source = d.template read<std::uint8_t>();

    // variable parameter count is only present in DIS 7,
    // but reading it for DIS 6 doesn't cause any issues since it's just padding
    // in that case
    std::optional<std::uint16_t> variable_parameter_count;
    variable_parameter_count = d.template read<be<std::uint16_t>>();

    auto antenna_location = deserialize_direct<WorldCoordinates>(d);
    auto relative_antenna_location = deserialize_direct<Vector>(d);
    auto antenna_pattern_type = d.template read<be<std::uint16_t>>();
    auto num_antenna_patterns = d.template read<be<std::uint16_t>>();
    auto center_frequency = d.template read<be<std::uint64_t>>();
    auto transmit_frequency_bandwidth = d.template read<be<float>>();
    auto power = d.template read<be<float>>();
    auto modulation_type = deserialize_direct<ModulationType>(d);
    auto crypto_system = d.template read<be<std::uint16_t>>();
    auto crypto_key_id = d.template read<be<std::uint16_t>>();
    auto num_modulation_parameters = d.template read<std::uint8_t>();
    std::ignore = d.template read<std::uint16_t>();
    std::ignore = d.template read<std::uint8_t>();  // 3 byte padding

    if (!header || !radio_reference_id || !radio_number || !radio_type ||
        !transmit_state || !input_source || !antenna_location ||
        !relative_antenna_location || !antenna_pattern_type ||
        !num_antenna_patterns || !center_frequency ||
        !transmit_frequency_bandwidth || !power || !modulation_type ||
        !crypto_system || !crypto_key_id || !num_modulation_parameters) {
        return unexpect;
    }

    Buffer<uint8_t> modulation_parameters;
    for (size_t i = 0; i < *num_modulation_parameters; ++i) {
        auto mod_param = d.template read<std::uint8_t>();
        if (!mod_param) {
            return unexpect;
        }
        modulation_parameters.push_back(*mod_param);
    }

    Buffer<BeamAntennaPattern> antenna_patterns;
    for (size_t i = 0; i < *num_antenna_patterns; ++i) {
        auto antenna_pattern = deserialize_direct<BeamAntennaPattern>(d);
        if (!antenna_pattern) {
            return unexpect;
        }
        antenna_patterns.push_back(*antenna_pattern);
    }

    Buffer<VariableParameters> variable_parameters;
    if (variable_parameter_count.has_value()) {
        for (size_t i = 0; i < *variable_parameter_count; ++i) {
            auto type = d.template read<be<std::uint32_t>>();
            auto length = d.template read<be<std::uint16_t>>();
            std::ignore =
                d.template read<std::uint16_t>();  // padding to align to 8 byte
                                                   // boundary
            Buffer<uint8_t> data;
            for (size_t j = 0; j < length - 6; ++j) {
                auto byte = d.template read<std::uint8_t>();
                if (!byte) {
                    return unexpect;
                }
                data.push_back(*byte);
            }
            variable_parameters.push_back(VariableParameters{*type, data});
            // pad to align to next 8 byte boundary
            size_t required_padding = (8 - (length % 8)) % 8;
            d.pad(required_padding);
        }
    }

    return TransmitterPDU{*header,
                          *radio_reference_id,
                          *radio_number,
                          *radio_type,
                          *transmit_state,
                          *input_source,
                          *variable_parameter_count,
                          *antenna_location,
                          *relative_antenna_location,
                          *antenna_pattern_type,
                          *num_antenna_patterns,
                          *center_frequency,
                          *transmit_frequency_bandwidth,
                          *power,
                          *modulation_type,
                          *crypto_system,
                          *crypto_key_id,
                          std::move(modulation_parameters),
                          std::move(antenna_patterns),
                          std::move(variable_parameters)};
}

};  // namespace csics::lvc::dis
