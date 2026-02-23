#pragma once
#include <csics/Bit.hpp>
#include "csics/lvc/dis/PDUs.hpp"
#include "csics/serialization/Serialization.hpp"
namespace csics::lvc::dis {
    struct PDUHeaderWire {
        uint8_t protocol_version;
        uint8_t exercise_id;
        uint8_t pdu_type;
        be<uint32_t> timestamp;
        be<uint16_t> protocol_family;
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
    };

    struct IDWire {
        be<uint16_t> site_id;
        be<uint16_t> application_id;
        uint8_t id;

        constexpr auto fields() const {
            using namespace csics::serialization;
            return std::tuple{
                make_field("site_id", &IDWire::site_id),
                make_field("application_id", &IDWire::application_id),
                make_field("id", &IDWire::id)};
        };

        constexpr ID to_id() const {
            return ID(site_id.native(), application_id.native(), id);
        }

        constexpr static IDWire from_id(const ID& id) {
            return IDWire{be<uint16_t>(id.site()), be<uint16_t>(id.application()),
                          static_cast<uint8_t>(id.id())};
        }
    };
};
