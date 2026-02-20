#pragma once

#include <csics/io/net/NetTypes.hpp>
#include <csics/Buffer.hpp>
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
        NetResult recv(BufferView buffer, SockAddr& src);
        template <typename T>
        NetResult connect(T&& addr) {
            static_assert(std::is_convertible_v<T, SockAddr>,
                          "Address type must be convertible to SockAddr for "
                          "UDPEndpoint connection");
            return connect_(static_cast<SockAddr>(addr));
        }
    private:
        struct Internal;
        Internal* internal_;

        NetResult connect_(SockAddr addr);
    };
};
