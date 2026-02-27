
#include "csics/io/net/NetTypes.hpp"
#include "csics/Bit.hpp"
#include <netinet/in.h>

namespace csics::io::net {

    void SockAddr::to_native(void* native, std::size_t& size) const {
        sockaddr_in* addr = static_cast<sockaddr_in*>(native);
        std::memset(addr, 0, sizeof(sockaddr_in));
        addr->sin_family = AF_INET;
        std::memcpy(&addr->sin_addr.s_addr, address_.bytes(), 4);
        addr->sin_port = htons(port_);
        size = sizeof(sockaddr_in);
    }

    std::optional<SockAddr> SockAddr::from_native(const void* native_addr,
                                              std::size_t addr_len) {
        if (addr_len < sizeof(sockaddr_in)) {
            return std::nullopt;  // invalid address length
        }
        const sockaddr_in* addr = static_cast<const sockaddr_in*>(native_addr);
        if (addr->sin_family != AF_INET) {
            return std::nullopt;  // unsupported address family
        }

        IPAddress ip(addr->sin_addr.s_addr);
        uint16_t port = ne<uint16_t>(addr->sin_port).native();
        return SockAddr(ip, port);
    }

};
