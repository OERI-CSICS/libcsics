#pragma once

#include "csics/Buffer.hpp"
#include "csics/serialization/serialization.hpp"
namespace csics::lvc::dis {

template <typename... PDUs>
class DISDispatcher {
   public:
    using dispatch_func = void (*)(BufferView);
    static constexpr dispatch_func no_op = [](BufferView) {};
    DISDispatcher() { dispatch_table_.fill(no_op); }

    template <typename F, typename PDU>
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
                    f(*pdu);  // no error checking for now...
                };
        } else {
            static_assert(std::is_invocable_v<F, BufferView> ||
                              std::is_invocable_v<F, const PDU&>,
                          "Handler function must be invocable with either "
                          "BufferView or const PDU&");
        }
    }

   private:
    std::array<dispatch_func, 255> dispatch_table_;
};

};  // namespace csics::lvc::dis
