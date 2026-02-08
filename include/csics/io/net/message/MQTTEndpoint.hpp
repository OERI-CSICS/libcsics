

#include "csics/io/net/message/message.hpp"
#include "csics/io/net/net.hpp"
namespace csics::io::net {

class MQTTEndpoint {
   public:
    using ConnectionParams = URI;
    struct MQTTMessageParameters {
        BufferView<> topic;
        int qos;
        int retain;
        int msg_token;
    };
    using MessageParameters = MQTTMessageParameters;

    constexpr MQTTEndpoint() : internal_(nullptr) {};
    ~MQTTEndpoint() {};

    MQTTEndpoint(const MQTTEndpoint&) = delete;
    MQTTEndpoint& operator=(const MQTTEndpoint&) = delete;
    MQTTEndpoint(MQTTEndpoint&& other) noexcept;
    MQTTEndpoint& operator=(MQTTEndpoint&& other) noexcept;

    MessageResult send(Payload message, MessageParameters* parameter);
    MessageResult recv(Payload buffer, MessageParameters* parameter);
    template <typename T>
    MessageResult connect(T&& addr) {
        static_assert(std::is_convertible_v<T, URI>,
                      "Address type must be convertible to URI for "
                      "MQTTEndpoint connection");
        return connect_(static_cast<URI>(addr));
    }

    static PollStatus poll(const MQTTEndpoint* endpoint, int timeoutMs);

   private:
    struct Internal;
    Internal* internal_;

    MessageResult connect_(URI addr);
};
};  // namespace csics::io::net
