#pragma once

#include <csics/io/net/NetTypes.hpp>
#include <csics/Buffer.hpp>

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
    NetStatus connect(T&& addr) {
        static_assert(std::is_convertible_v<T, SockAddr>,
                      "Address type must be convertible to SockAddr for "
                      "TCPEndpoint connection");
        return connect_(static_cast<SockAddr>(addr));
    }

    static PollStatus poll(const TCPEndpoint* endpoint, int timeoutMs);

    static std::vector<PollStatus> poll(const std::vector<TCPEndpoint*>& endpoints, int timeoutMs);

   private:
    struct Internal;
    Internal* internal_;

    NetStatus connect_(SockAddr addr);
};
};  // namespace csics::io::net
