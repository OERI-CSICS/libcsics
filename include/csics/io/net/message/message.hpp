

#include "csics/Buffer.hpp"
namespace csics::io::net {
    using Payload = BufferView<>;
    using MessageParameters = void;

    enum class MessageStatus { Success, Timeout, Disconnected, Error };

    struct MessageResult {
        MessageStatus status;
        void* raw_status;
        std::size_t bytesTransferred;

    };

};
