#pragma once

#include "csics/Buffer.hpp"
#include "csics/io/net/NetTypes.hpp"

namespace csics::io::net {
class TCPEndpoint {
   public:
    using ConnectionParams = SockAddr;

    TCPEndpoint();
    ~TCPEndpoint();
    TCPEndpoint(const TCPEndpoint&) = delete;
    TCPEndpoint& operator=(const TCPEndpoint&) = delete;
    TCPEndpoint(TCPEndpoint&& other) noexcept;
    TCPEndpoint& operator=(TCPEndpoint&& other) noexcept;

    NetResult send(BufferView data);
    NetResult recv(BufferView buffer);
    template <typename T>
        requires std::is_convertible_v<T, SockAddr>
    NetStatus connect(T&& addr) {
        return connect_(SockAddr(std::forward<T>(addr)));
    }

    PollStatus poll(int timeoutMs);

   private:
    struct Internal;
    Internal* internal_;

    NetStatus connect_(SockAddr addr);
};
};  // namespace csics::io::net
