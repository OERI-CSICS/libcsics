#pragma once
#include <csics/Buffer.hpp>
#include <csics/io/net/net.hpp>
#include <csics/io/net/stream/Stream.hpp>
#include <csics/io/net/stream/TCPEndpoint.hpp>

namespace csics::io::net {
template <typename Endpoint>
inline StreamResult send_stream(Endpoint* endpoint, BufferView<> data) {
    return endpoint->send(data);
};

template <>
inline StreamResult send_stream<TypeErasedEndpoint>(TypeErasedEndpoint* endpoint, BufferView<> data) {
    switch (static_cast<EndpointType>(endpoint->type)) {
        case EndpointType::TCP:
            return send_stream(reinterpret_cast<TCPEndpoint*>(endpoint->impl),
                               data);
        default:
            return StreamResult{StreamStatus::Error, 0};
    }
}

template <typename Endpoint, typename ConnectParameter>
inline StreamResult connect_stream(Endpoint* endpoint,
                                   ConnectParameter* parameter) {
    static_assert(std::is_convertible_v<ConnectParameter, typename Endpoint::ConnectionParams>,
                  "Connection parameter type must be convertible to "
                  "ConnectionParams for connect_stream");
    return endpoint->connect(
        *static_cast<Endpoint::ConnectionParams*>(parameter));
};

template <>
inline StreamResult connect_stream<TypeErasedEndpoint, ConnectionParams>(
    TypeErasedEndpoint* endpoint, ConnectionParams* parameter) {
    switch (static_cast<EndpointType>(endpoint->type)) {
        case EndpointType::TCP:
            return connect_stream(
                reinterpret_cast<TCPEndpoint*>(endpoint->impl), reinterpret_cast<SockAddr*>(parameter));
        default:
            return StreamResult{StreamStatus::Error, 0};
    }
}

inline TypeErasedEndpoint* create_endpoint(EndpointType type) {
    TypeErasedEndpoint* endpoint = new TypeErasedEndpoint();
    switch (type) {
        case EndpointType::TCP:
            endpoint->impl = new TCPEndpoint();
            endpoint->type = static_cast<uint8_t>(EndpointType::TCP);
            break;
        default:
            delete endpoint;
            return nullptr;
    }
    return endpoint;
}
};  // namespace csics::io::net
