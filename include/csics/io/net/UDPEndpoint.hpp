#pragma once

#include "csics/Buffer.hpp"
#include "csics/io/net/NetTypes.hpp"
namespace csics::io::net {
class UDPEndpoint {
   public:
    using ConnectionParams = SockAddr;

    UDPEndpoint();
    ~UDPEndpoint();
    UDPEndpoint(const UDPEndpoint&) = delete;
    UDPEndpoint& operator=(const UDPEndpoint&) = delete;
    UDPEndpoint(UDPEndpoint&& other) noexcept;
    UDPEndpoint& operator=(UDPEndpoint&& other) noexcept;

    NetResult send(BufferView data, const SockAddr& dest);
    NetStatus bind(const Port port);
    NetResult recv(MutableBufferView buffer, SockAddr& src);
    template <typename T>
        requires std::is_convertible_v<T, SockAddr>
    NetStatus connect(T&& addr) {
        return connect_(SockAddr(std::forward<T>(addr)));
    }

    PollStatus poll(int timeout_ms);

   private:
    struct Internal;
    Internal* internal_;

    NetStatus connect_(SockAddr addr);
};
};  // namespace csics::io::net
