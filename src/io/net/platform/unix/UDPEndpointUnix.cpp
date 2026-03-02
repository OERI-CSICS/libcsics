#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include <csics/io/net/UDPEndpoint.hpp>

namespace csics::io::net {
struct UDPEndpoint::Internal {
    int sockfd;
    Internal() : sockfd(-1) {}
    ~Internal() {
        if (sockfd != -1) {
            close(sockfd);
        }
    }
};

UDPEndpoint::UDPEndpoint() : internal_(new Internal()) {}

UDPEndpoint::~UDPEndpoint() { delete internal_; }

UDPEndpoint::UDPEndpoint(UDPEndpoint&& other) noexcept
    : internal_(other.internal_) {
    other.internal_ = nullptr;
}

UDPEndpoint& UDPEndpoint::operator=(UDPEndpoint&& other) noexcept {
    if (this != &other) {
        delete internal_;
        internal_ = other.internal_;
        other.internal_ = nullptr;
    }
    return *this;
};

NetResult UDPEndpoint::send(BufferView data, const SockAddr& dest) {
    if (internal_ == nullptr || internal_->sockfd == -1) {
        return NetResult{NetStatus::Error, 0};
    }
    struct sockaddr_in dest_addr{};
    std::size_t dest_addr_size;
    dest.to_native(&dest_addr, dest_addr_size);
    ssize_t bytesSent = ::sendto(
        internal_->sockfd, data.data(), data.size(), 0,
        reinterpret_cast<const struct sockaddr*>(&dest_addr), dest_addr_size);
    if (bytesSent < 0) {
        return NetResult{NetStatus::Error, 0};
    }
    return NetResult{NetStatus::Success, static_cast<std::size_t>(bytesSent)};
};

NetStatus UDPEndpoint::bind(const Port port) {
    if (internal_ == nullptr) {
        return NetStatus::Error;
    }
    // Create socket
    internal_->sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (internal_->sockfd < 0) {
        return NetStatus::Error;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    // Bind the socket to the specified port
    int result = ::bind(internal_->sockfd, reinterpret_cast<struct sockaddr*>(&addr),
                        sizeof(addr));
    if (result < 0) {
        close(internal_->sockfd);
        internal_->sockfd = -1;
        return NetStatus::Error;
    }

    return NetStatus::Success;
}

NetStatus UDPEndpoint::connect_(SockAddr addr) {
    if (internal_ == nullptr) {
        return NetStatus::Error;
    }
    // Create socket
    internal_->sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (internal_->sockfd < 0) {
        return NetStatus::Error;
    }

    struct sockaddr native_addr = {};
    std::size_t native_addr_size;

    addr.to_native(&native_addr, native_addr_size);

    // Connect to the server
    int result =
        ::connect(internal_->sockfd, &native_addr, sizeof(native_addr));
    if (result < 0) {
        close(internal_->sockfd);
        internal_->sockfd = -1;
        return NetStatus::Error;
    }

    return NetStatus::Success;
}

NetResult UDPEndpoint::recv(MutableBufferView buffer, SockAddr& src) {
    if (internal_ == nullptr || internal_->sockfd == -1) {
        return NetResult{NetStatus::Error, 0};
    }
    struct sockaddr_in src_addr{};
    socklen_t src_addr_len = sizeof(src_addr);
    ssize_t bytesReceived = ::recvfrom(internal_->sockfd, buffer.data(),
                                       buffer.size(), 0,
                                       reinterpret_cast<struct sockaddr*>(&src_addr),
                                       &src_addr_len);
    if (bytesReceived < 0) {
        return NetResult{NetStatus::Error, 0};
    }

    src = SockAddr::from_native(&src_addr, src_addr_len).value_or(SockAddr());

    return NetResult{NetStatus::Success,
                        static_cast<std::size_t>(bytesReceived)};
}

PollStatus UDPEndpoint::poll(int timeout_ms) {

    if (internal_ == nullptr || internal_->sockfd == -1) {
        return PollStatus::Error;
    }

    struct pollfd pfd{};
    pfd.fd = internal_->sockfd;
    pfd.events = POLLIN;

    int ret = ::poll(&pfd, 1, timeout_ms);

    if (ret < 0) {
        return PollStatus::Error;
    } else if (ret == 0) {
        return PollStatus::Timeout;
    } else {
        if (pfd.revents & POLLIN) {
            return PollStatus::Ready;
        } else {
            return PollStatus::Error;
        }
    }
}

};  // namespace csics::io::net
