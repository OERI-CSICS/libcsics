#pragma once

#include "csics/Buffer.hpp"
#include "csics/lvc/dis/PDUs.hpp"
#include "csics/serialization/serialization.hpp"
namespace csics::lvc::dis {

class DISDispatcher {
   public:
    using dispatch_func = std::function<void(BufferView)>;
    static constexpr auto no_op = [](BufferView) {};
    DISDispatcher() { dispatch_table_.fill(no_op); }

    template <typename PDU, typename F>
    void on(F&& func) {
        if constexpr (std::is_invocable_v<F, BufferView>) {
            dispatch_table_[PDU::pdu_type] = func;
        } else if constexpr (std::is_invocable_v<F, const PDU&>) {
            dispatch_table_[PDU::pdu_type] =
                [f = std::forward<F>(func)](BufferView bv) {
                    PDU pdu;
                    serialization::DirectDeserializer deserializer(bv);
                    auto res = serialization::deserialize(deserializer, pdu);
                    // TODO: handle deserialization errors properly
                    f(*res);  // no error checking for now...
                };
        } else {
            static_assert(std::is_invocable_v<F, BufferView> ||
                              std::is_invocable_v<F, const PDU&>,
                          "Handler function must be invocable with either "
                          "BufferView or const PDU&");
        }
    }

    void dispatch(BufferView bv) const {
        if (bv.size() < 2) {
            // Not enough data to read PDU type
            return;
        }
        PDUHeaderView header(bv);
        std::uint8_t pdu_type = static_cast<std::uint8_t>(header.pdu_type());
        if (pdu_type < dispatch_table_.size()) {
            dispatch_table_[pdu_type](bv);
        }
    }

   private:
    std::array<dispatch_func, 255> dispatch_table_;
};

};  // namespace csics::lvc::dis
