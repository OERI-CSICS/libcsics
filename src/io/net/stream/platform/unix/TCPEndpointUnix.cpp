#include <sys/socket.h>
#include <unistd.h>

#include <csics/io/net/TCPEndpoint.hpp>

namespace csics::io::net {
struct TCPEndpoint::Internal {
    int sockfd;
    Internal() : sockfd(-1) {}
    ~Internal() {
        if (sockfd != -1) {
            close(sockfd);
        }
    }
};

TCPEndpoint::TCPEndpoint() : internal_(new Internal()) {}

TCPEndpoint::~TCPEndpoint() { delete internal_; }

TCPEndpoint::TCPEndpoint(TCPEndpoint&& other) noexcept
    : internal_(other.internal_) {
    other.internal_ = nullptr;
}

TCPEndpoint& TCPEndpoint::operator=(TCPEndpoint&& other) noexcept {
    if (this != &other) {
        delete internal_;
        internal_ = other.internal_;
        other.internal_ = nullptr;
    }
    return *this;
};

NetResult TCPEndpoint::send(BufferView data) {
    if (internal_ == nullptr || internal_->sockfd == -1) {
        return NetResult{NetStatus::Error, 0};
    }
    ssize_t bytesSent = ::send(internal_->sockfd, data.data(), data.size(), 0);
    if (bytesSent < 0) {
        return NetResult{NetStatus::Error, 0};
    }
    return NetResult{NetStatus::Success,
                        static_cast<std::size_t>(bytesSent)};
};

NetStatus TCPEndpoint::connect_(SockAddr addr) {
    if (internal_ == nullptr) {
        return NetStatus::Error;
    }
    // Create socket
    internal_->sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (internal_->sockfd < 0) {
        return NetStatus::Error;
    }

    // Connect to the server
    int result = ::connect(internal_->sockfd,
                           reinterpret_cast<const struct sockaddr*>(&addr),
                           sizeof(addr));
    if (result < 0) {
        close(internal_->sockfd);
        internal_->sockfd = -1;
        return NetStatus::Error;
    }

    return NetStatus::Success;
}

NetResult TCPEndpoint::recv(BufferView buffer) {
    if (internal_ == nullptr || internal_->sockfd == -1) {
        return NetResult{NetStatus::Error, 0};
    }
    ssize_t bytesReceived =
        ::recv(internal_->sockfd, buffer.data(), buffer.size(), 0);
    if (bytesReceived < 0) {
        return NetResult{NetStatus::Error, 0};
    } else if (bytesReceived == 0) {
        return NetResult{NetStatus::Disconnected, 0};
    }
    return NetResult{NetStatus::Success,
                        static_cast<std::size_t>(bytesReceived)};
}
};  // namespace csics::io::net
