#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <type_traits>
namespace csics::io::net {

enum class EndpointType : uint8_t { TCP = 0, UDP = 1, MQTT = 2 };
using TypeErasedParams = void;
struct TypeErasedEndpoint {
    void* impl;
    uint8_t type;

    using ConnectionParams = TypeErasedParams;
};


using Port = uint16_t;

class IPAddress {
   public:
    constexpr IPAddress() : bytes_{0, 0, 0, 0, 0, 0} {};
    constexpr ~IPAddress() {}

    IPAddress(const char* address);
    IPAddress(const std::string& address);
    constexpr IPAddress(const uint32_t address) noexcept {
        bytes_[0] = static_cast<uint8_t>((address >> 24) & 0xFF);
        bytes_[1] = static_cast<uint8_t>((address >> 16) & 0xFF);
        bytes_[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
        bytes_[3] = static_cast<uint8_t>(address & 0xFF);
        bytes_[4] = 0;
        bytes_[5] = 0;
    };  // must be in network byte order
    constexpr IPAddress(std::array<uint8_t, 4> bytes) noexcept {
        bytes_[0] = bytes[0];
        bytes_[1] = bytes[1];
        bytes_[2] = bytes[2];
        bytes_[3] = bytes[3];
        bytes_[4] = 0;
        bytes_[5] = 0;
    };  // must be in network byte order
    constexpr IPAddress(std::array<uint8_t, 6> bytes) noexcept {
        bytes_[0] = bytes[0];
        bytes_[1] = bytes[1];
        bytes_[2] = bytes[2];
        bytes_[3] = bytes[3];
        bytes_[4] = bytes[4];
        bytes_[5] = bytes[5];
    };  // must be in network byte order
    constexpr IPAddress(std::array<uint16_t, 2> words) noexcept {
        bytes_[0] = static_cast<uint8_t>((words[0] >> 8) & 0xFF);
        bytes_[1] = static_cast<uint8_t>(words[0] & 0xFF);
        bytes_[2] = static_cast<uint8_t>((words[1] >> 8) & 0xFF);
        bytes_[3] = static_cast<uint8_t>(words[1] & 0xFF);
        bytes_[4] = 0;
        bytes_[5] = 0;
    };  // must be in network byte order
    constexpr IPAddress(std::array<uint16_t, 3> words) noexcept {
        bytes_[0] = static_cast<uint8_t>((words[0] >> 8) & 0xFF);
        bytes_[1] = static_cast<uint8_t>(words[0] & 0xFF);
        bytes_[2] = static_cast<uint8_t>((words[1] >> 8) & 0xFF);
        bytes_[3] = static_cast<uint8_t>(words[1] & 0xFF);
        bytes_[4] = static_cast<uint8_t>((words[2] >> 8) & 0xFF);
        bytes_[5] = static_cast<uint8_t>(words[2] & 0xFF);
    };  // must be in network byte order
    constexpr static IPAddress localhost() noexcept {
        return IPAddress(uint32_t{0x7F000001});
    }

   private:
    uint8_t bytes_[6];  // enough to hold IPv4 and IPv6
};

class SockAddr {
   public:
    constexpr SockAddr() : address_(), port_(0) {};
    constexpr ~SockAddr() {}
    SockAddr(const SockAddr&) noexcept;
    SockAddr& operator=(const SockAddr&) noexcept;
    SockAddr(SockAddr&& other) noexcept;
    SockAddr& operator=(SockAddr&& other) noexcept;

    constexpr SockAddr(const IPAddress& address, uint16_t port)
        : address_(address), port_(port) {}

    constexpr static SockAddr localhost(Port port) noexcept {
        return SockAddr(IPAddress::localhost(), port);
    }

   private:
    IPAddress address_;
    Port port_;
};

class URI {
   public:
    URI(const std::string& uri);
    ~URI() = default;
    URI(const URI&) = default;
    URI& operator=(const URI&) = default;
    URI(URI&&) noexcept = default;
    URI& operator=(URI&&) noexcept = default;

    const std::string& scheme() const { return scheme_; }
    const std::string& host() const { return host_; }
    Port port() const { return port_; }
    const std::string& path() const { return path_; }

   private:
    std::string scheme_;
    std::string host_;
    std::string path_;
    Port port_;
};

enum class PollStatus { Ready, Timeout, Disconnected, Error };

inline PollStatus poll_endpoint(const TypeErasedEndpoint* endpoint, int timeoutMs);
inline size_t poll_endpoints(const TypeErasedEndpoint* endpoints, size_t size,
                             PollStatus* status_array, const size_t poll_size,
                             int timeoutMs);

template <typename T>
constexpr T byte_swap(T val) {
    static_assert(std::is_integral_v<T>, "byte_swap requires an integral type");

    if constexpr (sizeof(T) == 1) {
        return val;
    } else if constexpr (sizeof(T) == 2) {
        return (val << 8) | (val >> 8);
    } else if constexpr (sizeof(T) == 4) {
        return ((val & 0x000000FFu) << 24) | ((val & 0x0000FF00u) << 8) |
               ((val & 0x00FF0000u) >> 8) | ((val & 0xFF000000u) >> 24);
    } else if constexpr (sizeof(T) == 8) {
        return ((val & 0x00000000000000FFull) << 56) |
               ((val & 0x000000000000FF00ull) << 40) |
               ((val & 0x0000000000FF0000ull) << 24) |
               ((val & 0x00000000FF000000ull) << 8) |
               ((val & 0x000000FF00000000ull) >> 8) |
               ((val & 0x0000FF0000000000ull) >> 24) |
               ((val & 0x00FF000000000000ull) >> 40) |
               ((val & 0xFF00000000000000ull) >> 56);
    } else {
        static_assert(sizeof(T) <= 8, "Unsupported integer size for byte_swap");
    }
}

template <typename T>
constexpr T hton(T val) {
    if constexpr (std::endian::native == std::endian::little) {
        return byte_swap(val);
    } else {
        return val;
    }
}

constexpr uint16_t csics_htons(uint16_t val) { return hton(val); }
constexpr uint32_t csics_htonl(uint32_t val) { return hton(val); }
constexpr uint64_t csics_htonll(uint64_t val) { return hton(val); }

constexpr uint16_t csics_ntohs(uint16_t val) { return hton(val); }
constexpr uint32_t csics_ntohl(uint32_t val) { return hton(val); }
constexpr uint64_t csics_ntohll(uint64_t val) { return hton(val); }

};  // namespace csics::io::net
