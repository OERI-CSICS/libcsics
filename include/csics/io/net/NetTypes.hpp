#pragma once

#include <optional>
#ifndef CSICS_BUILD_IO
#error "IO support is not enabled. Please define CSICS_BUILD_IO to use net."
#endif

#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <type_traits>

#include "csics/String.hpp"
namespace csics::io::net {

enum class NetStatus { Success, Empty, Timeout, Disconnected, Error };
struct NetResult {
    NetStatus status;
    std::size_t bytes_transferred;
};

using Port = uint16_t;

class IPAddress {
    // TODO: this sucks refactor it using endian<T,E>
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

    constexpr const uint8_t* bytes() const noexcept { return bytes_; }

   private:
    uint8_t bytes_[6];  // enough to hold IPv4 and IPv6
};

class SockAddr {
   public:
    constexpr SockAddr() : address_(), port_(0) {};
    constexpr ~SockAddr() {}
    SockAddr(const SockAddr&) noexcept = default;
    SockAddr& operator=(const SockAddr&) noexcept = default;
    SockAddr(SockAddr&& other) noexcept = default;
    SockAddr& operator=(SockAddr&& other) noexcept = default;

    constexpr SockAddr(const IPAddress& address, uint16_t port)
        : address_(address), port_(port) {}

    constexpr static SockAddr localhost(Port port) noexcept {
        return SockAddr(IPAddress::localhost(), port);
    }

    void to_native(void* native_addr, std::size_t& addr_len) const;
    static std::optional<SockAddr> from_native(const void* native_addr,
                                              std::size_t addr_len);

   private:
    IPAddress address_;
    Port port_;
};

class URI {
   public:
    URI() = default;
    ~URI() = default;
    URI(const URI&) = default;
    URI& operator=(const URI&) = default;
    URI(URI&&) noexcept = default;
    URI& operator=(URI&&) noexcept = default;
    static std::optional<URI> from(const StringView uri) {
        // scheme://host:port/path
        // optional port, path
        auto scheme_end = uri.end();
        auto host_end = uri.end();
        auto port_end = uri.end();
        auto cursor = uri.begin();

        std::size_t state = 0;  // 0 = scheme, 1 = host, 2 = port, 3 = path

        while (cursor != uri.end()) {
            switch (state) {
                case 0: {
                    if (*cursor == ':') {
                        scheme_end = cursor;
                        if (StringView(cursor, 3) != "://") {
                            return std::nullopt;  // invalid URI format
                        }
                        cursor += 3;  // skip "://"
                        state = 1;
                    }
                    break;
                }
                case 1: {
                    if (*cursor == ':') {
                        host_end = cursor;
                        state = 2;
                    } else if (*cursor == '/') {
                        host_end = cursor;
                        state = 3;
                    }
                    break;
                }
                case 2: {
                    if (*cursor == '/') {
                        port_end = cursor;
                        state = 3;
                    }
                    break;
                }
                case 3:
                    // just consume the rest of the path
                    break;
            }
            cursor++;
        }

        URI result;
        result.uri_ = String(uri);
        result.scheme_ = StringView(uri.begin(), scheme_end - uri.begin());
        result.host_ = StringView(scheme_end + 3, host_end - (scheme_end + 3));
        if (port_end != uri.end() || state == 2) {
            result.port_ = static_cast<Port>(
                std::stoi(std::string(host_end + 1, port_end)));
        } else {
            result.port_ = 0;
        }

        if (state == 3) {
            if (result.port_ != 0) {
                result.path_ = StringView(port_end + 1, uri.end() - port_end);
            } else {
                result.path_ = StringView(host_end + 1, uri.end() - host_end);
            }
        } else {
            result.path_ = StringView();
        }
        return result;
    }

    StringView scheme() const { return scheme_; }
    StringView host() const { return host_; }
    Port port() const { return port_; }
    StringView path() const { return path_; }

    const String str() const { return uri_; }
    const char* c_str() const { return uri_.c_str(); }

   private:
    String uri_;
    StringView scheme_;
    StringView host_;
    StringView path_;
    Port port_;
};

enum class PollStatus { Ready, Empty, Timeout, Disconnected, Error };

};  // namespace csics::io::net
