#pragma once

#ifndef CSICS_BUILD_IO
#error "IO support is not enabled. Please define CSICS_BUILD_IO to use net."
#endif

#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <type_traits>

#include "csics/Buffer.hpp"
namespace csics::io::net {

enum class NetStatus { Success, Empty, Timeout, Disconnected, Error };
struct NetResult {
    NetStatus status;
    std::size_t bytes_transferred;
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
    URI();
    ~URI() = default;
    URI(const URI&) = default;
    URI& operator=(const URI&) = default;
    URI(URI&&) noexcept = default;
    URI& operator=(URI&&) noexcept = default;
    URI(const StringView uri) {
        // This is a very basic URI parser that only supports the format:
        // scheme://host:port/path
        auto scheme_end = uri.begin();
        auto host_start = uri.begin();
        auto host_end = uri.begin();
        auto path_start = uri.end();
        auto cursor = uri.begin();

        while (cursor != uri.end()) {
            if (*cursor == ':') {
                if (scheme_end == uri.begin()) {
                    scheme_end = cursor;
                    host_start = cursor + 3;  // skip "://"
                } else if (host_end == uri.begin()) {
                    host_end = cursor;
                }
            } else if (*cursor == '/') {
                if (host_end == uri.begin()) {
                    host_end = cursor;
                }
                if (path_start == uri.end()) {
                    path_start = cursor;
                }
            }
            cursor++;
        }

        if (scheme_end != uri.begin()) {
            scheme_ = String(StringView(uri.data(), scheme_end - uri.begin()));
        }

        if (host_start != uri.end() && host_end != uri.begin()) {
            host_ = String(StringView(host_start, host_end - host_start));
        }

        if (path_start != uri.end()) {
            path_ = String(StringView(path_start, uri.end() - path_start));
        }

        if (host_end != path_start) {
            auto port_str = StringView(host_end + 1, path_start - host_end - 1);
            port_ = static_cast<Port>(
                std::stoi(std::string(port_str.data(), port_str.size())));
        }
    }

    const String scheme() const { return scheme_; }
    const String host() const { return host_; }
    Port port() const { return port_; }
    const String path() const { return path_; }

    const String str() const;
    const char* c_str() const { return str().c_str(); }

   private:
    String scheme_;
    String host_;
    String path_;
    Port port_;
};

enum class PollStatus { Ready, Empty, Timeout, Disconnected, Error };

};  // namespace csics::io::net
