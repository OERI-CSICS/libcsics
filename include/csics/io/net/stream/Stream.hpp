#pragma once
#include <csics/Buffer.hpp>
#include <cstddef>

namespace csics::io::net {

enum class StreamStatus { Success, Timeout, Disconnected, Error };

struct StreamResult {
    StreamStatus status;
    std::size_t bytesTransferred;
};

};  // namespace csics::io::net
