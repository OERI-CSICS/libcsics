#pragma once
#include "csics/Bit.hpp"
#include "csics/lvc/dis/PDUs.hpp"
#include "csics/serialization/serialization.hpp"

namespace csics::lvc::dis {

struct IDWire {
    be<uint16_t> site_id;
    be<uint16_t> application_id;
    uint8_t id;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{make_field("site_id", &IDWire::site_id),
                          make_field("application_id", &IDWire::application_id),
                          make_field("id", &IDWire::id)};
    };

    constexpr operator ID() const {
        return ID(site_id.native(), application_id.native(), id);
    }

    constexpr IDWire(const ID& id)
        : site_id(id.site()), application_id(id.application()), id(id.id()) {}
};

struct EntityIDWire {
    SimulationAddressWire simulation_address;
    be<uint16_t> entity_id;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{
            make_field("simulation_address", &EntityIDWire::simulation_address),
            make_field("entity_id", &EntityIDWire::entity_id)};
    };

    constexpr operator EntityID() const {
        return EntityID(site_id.native(), application_id.native(),
                        entity_id.native());
    }

    constexpr EntityIDWire(const EntityID& id)
        : site_id(id.site()),
          application_id(id.application()),
          entity_id(id.entity_id) {}
};

struct DISQuatWire {
    be<std::uint16_t> u0;
    be<float> x;
    be<float> y;
    be<float> z;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{make_field("u0", &DISQuatWire::u0),
                          make_field("x", &DISQuatWire::x),
                          make_field("y", &DISQuatWire::y),
                          make_field("z", &DISQuatWire::z)};
    }

    constexpr operator DISQuat() const {
        return DISQuat(u0.native(), x.native(), y.native(), z.native());
    }

    constexpr DISQuatWire(const DISQuat& quat)
        : u0(quat.u0), x(quat.x), y(quat.y), z(quat.z) {}
};

struct SimulationAddressWire {
    be<uint16_t> site_id;
    be<uint16_t> application_id;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{
            make_field("site_id", &SimulationAddressWire::site_id),
            make_field("application_id",
                       &SimulationAddressWire::application_id)};
    };

    constexpr operator SimulationAddress() const {
        return SimulationAddress(site_id.native(), application_id.native());
    }

    constexpr SimulationAddressWire(const SimulationAddress& addr)
        : site_id(addr.site()), application_id(addr.application()) {}
};

struct EntityTypeWire {
    uint8_t kind;
    uint8_t domain;
    be<uint16_t> country;
    uint8_t category;
    uint8_t subcategory;
    uint8_t specific;
    uint8_t extra;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{
            make_field("kind", &EntityTypeWire::kind),
            make_field("domain", &EntityTypeWire::domain),
            make_field("country", &EntityTypeWire::country),
            make_field("category", &EntityTypeWire::category),
            make_field("subcategory", &EntityTypeWire::subcategory),
            make_field("specific", &EntityTypeWire::specific),
            make_field("extra", &EntityTypeWire::extra)};
    };

    constexpr operator EntityType() const {
        return EntityType{kind,     domain,      country.native(),
                          category, subcategory, specific,
                          extra};
    }

    constexpr EntityTypeWire(const EntityType& type)
        : kind(type.kind),
          domain(type.domain),
          country(type.country),
          category(type.category),
          subcategory(type.subcategory),
          specific(type.specific),
          extra(type.extra) {}
};

struct PDUHeaderWire {
    uint8_t protocol_version;
    uint8_t exercise_id;
    uint8_t pdu_type;
    be<uint32_t> timestamp;
    be<uint8_t> protocol_family;
    uint8_t pdu_status;
    uint8_t padding;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{
            make_field("protocol_version", &PDUHeaderWire::protocol_version),
            make_field("exercise_id", &PDUHeaderWire::exercise_id),
            make_field("pdu_type", &PDUHeaderWire::pdu_type),
            make_field("timestamp", &PDUHeaderWire::timestamp),
            make_field("protocol_family", &PDUHeaderWire::protocol_family),
            make_field("pdu_status", &PDUHeaderWire::pdu_status),
            make_field("padding", &PDUHeaderWire::padding)};
    };

    constexpr operator PDUHeader() const {
        return PDUHeader{static_cast<ProtocolVersion>(protocol_version),
                         exercise_id,
                         static_cast<PDUType>(pdu_type),
                         DISTimestamp(timestamp.native()),
                         protocol_family.native(),
                         pdu_status};
    }

    constexpr PDUHeaderWire(const PDUHeader& header)
        : protocol_version(static_cast<uint8_t>(header.protocol_version)),
          exercise_id(header.exercise_id),
          pdu_type(static_cast<uint8_t>(header.pdu_type)),
          timestamp(header.timestamp.raw()),
          protocol_family(header.protocol_family),
          pdu_status(header.status),
          padding(0) {}
};

struct VectorWire {
    be<float> x;
    be<float> y;
    be<float> z;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{make_field("x", &VectorWire::x),
                          make_field("y", &VectorWire::y),
                          make_field("z", &VectorWire::z)};
    };

    constexpr operator Vector() const {
        return Vector(x.native(), y.native(), z.native());
    }

    constexpr VectorWire(const Vector& vec)
        : x(vec.get<0>()), y(vec.get<1>()), z(vec.get<2>()) {}
};

struct WorldCoordinatesWire {
    be<double> x;
    be<double> y;
    be<double> z;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{make_field("x", &WorldCoordinates::x),
                          make_field("y", &WorldCoordinates::y),
                          make_field("z", &WorldCoordinates::z)};
    };

    constexpr operator WorldCoordinates() const {
        return WorldCoordinates(x.native(), y.native(), z.native());
    }

    constexpr WorldCoordinatesWire(const WorldCoordinates& coords)
        : x(coords.x()), y(coords.y()), z(coords.z()) {}
};

struct EulerAnglesWire {
    be<float> psi;
    be<float> theta;
    be<float> phi;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{make_field("psi", &EulerAnglesWire::psi),
                          make_field("theta", &EulerAnglesWire::theta),
                          make_field("phi", &EulerAnglesWire::phi)};
    };

    constexpr operator EulerAngles() const {
        return EulerAngles(psi.native(), theta.native(), phi.native());
    }

    constexpr EulerAnglesWire(const EulerAngles& angles)
        : psi(angles.psi()), theta(angles.theta()), phi(angles.phi()) {}
};

struct DRParametersWire {
    uint8_t algorithm;
    uint8_t params_type;
    be<uint16_t> padding_or_data;
    be<float> psi_or_quat_x;
    be<float> theta_or_quat_y;
    be<float> phi_or_quat_z;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{
            make_field("algorithm", &DRParametersWire::algorithm),
            make_field("params_type", &DRParametersWire::params_type),
            make_field("padding_or_data", &DRParametersWire::padding_or_data),
            make_field("psi_or_quat_x", &DRParametersWire::psi_or_quat_x),
            make_field("theta_or_quat_y", &DRParametersWire::theta_or_quat_y),
            make_field("phi_or_quat_z", &DRParametersWire::phi_or_quat_z)};
    }

    constexpr DRParametersWire(const DeadReckoningParameters& drm) {
        algorithm = drm.algorithm;
        params_type = drm.params_type;
        if (drm.params_type == 1) {
            padding_or_data = 0;
            psi_or_quat_x = drm.local_angles.psi();
            theta_or_quat_y = drm.local_angles.theta();
            phi_or_quat_z = drm.local_angles.phi();
        } else if (drm.params_type == 2) {
            const DISQuat& quat = drm.rotating.quat;
            padding_or_data = quat.u0;
            psi_or_quat_x = quat.x;
            theta_or_quat_y = quat.y;
            phi_or_quat_z = quat.z;
        } else {
            padding_or_data = 0;
            psi_or_quat_x = 0.0f;
            theta_or_quat_y = 0.0f;
            phi_or_quat_z = 0.0f;
        }
    }

    constexpr operator DeadReckoningParameters() const {
        DeadReckoningParameters drm;
        drm.algorithm = algorithm;
        drm.params_type = params_type;
        if (params_type == 1) {
            drm.fixed.local_angles =
                EulerAngles(psi_or_quat_x.native(), theta_or_quat_y.native(),
                            phi_or_quat_z.native());
        } else if (params_type == 2) {
            drm.rotating.quat =
                DISQuat(padding_or_data.native(), psi_or_quat_x.native(),
                        theta_or_quat_y.native(), phi_or_quat_z.native());
        }
        return drm;
    }
};
};

struct EntityStateWire {
    PDUHeaderWire header;
    EntityIDWire entity_id;
    std::uint8_t force_id;
    std::uint8_t variable_params;
    EntityTypeWire entity_type;
    EntityTypeWire alternative_entity_type;
    VectorWire entity_linear_velocity;
    WorldCoordinatesWire entity_location;
    EulerAnglesWire entity_orientation;
    be<uint32_t> entity_appearance;
    DRParametersWire dr_parameters;
    std::uint8_t entity_marking[12];
    be<uint32_t> capabilities;
    Buffer<VariableParameters> variable_parameters;

    constexpr auto fields() const {
        using namespace csics::serialization;
        return std::tuple{
            make_field("header", &EntityStateWire::header),
            make_field("entity_id", &EntityStateWire::entity_id),
            make_field("force_id", &EntityStateWire::force_id),
            make_field("variable_params", &EntityStateWire::variable_params),
            make_field("entity_type", &EntityStateWire::entity_type),
            make_field("alternative_entity_type",
                       &EntityStateWire::alternative_entity_type),
            make_field("entity_linear_velocity",
                       &EntityStateWire::entity_linear_velocity),
            make_field("entity_location", &EntityStateWire::entity_location),
            make_field("entity_orientation",
                       &EntityStateWire::entity_orientation),
            make_field("entity_appearance",
                       &EntityStateWire::entity_appearance),
            make_field("dr_parameters", &EntityStateWire::dr_parameters),
            make_field("entity_marking", &EntityStateWire::entity_marking),
            make_field("capabilities", &EntityStateWire::capabilities),
            make_field("variable_parameters",
                       &EntityStateWire::variable_parameters)};
    }

    constexpr operator EntityStatePDU() const {
        EntityStatePDU pdu;
        pdu.header = header;
        pdu.entity_id = entity_id;
        pdu.force_id = force_id;
        pdu.entity_type = entity_type;
        pdu.alternative_entity_type = alternative_entity_type;
        pdu.entity_linear_velocity = entity_linear_velocity;
        pdu.entity_location = entity_location;
        pdu.entity_orientation = entity_orientation;
        pdu.entity_appearance = entity_appearance.native();
        pdu.dr_parameters = dr_parameters;
        pdu.entity_marking.character_set = entity_marking[0];
        std::copy(std::begin(entity_marking) + 1, std::end(entity_marking),
                  std::begin(pdu.entity_marking.marking));
        pdu.capabilities = capabilities.native();
        pdu.variable_parameters = variable_parameters;
        return pdu;
    }

    constexpr EntityStateWire(const EntityStatePDU& pdu)
        : header(pdu.header),
          entity_id(pdu.entity_id),
          force_id(pdu.force_id),
          variable_params(pdu.variable_parameters.size()),
          entity_type(pdu.entity_type),
          alternative_entity_type(pdu.alternative_entity_type),
          entity_linear_velocity(pdu.entity_linear_velocity),
          entity_location(pdu.entity_location),
          entity_orientation(pdu.entity_orientation),
          entity_appearance(pdu.entity_appearance),
          dr_parameters(pdu.dr_parameters),
          capabilities(pdu.capabilities),
          variable_parameters(pdu.variable_parameters) {
        std::copy(std::begin(pdu.entity_marking.marking),
                  std::end(pdu.entity_marking.marking),
                  std::begin(entity_marking));
    }
};
};  // namespace csics::lvc::dis
