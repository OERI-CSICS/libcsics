#pragma once

#ifndef CSICS_USE_MQTT
#error \
    "MQTT support is not enabled. Please define CSICS_USE_MQTT to use MQTTEndpoint."
#endif

#include "csics/Buffer.hpp"
#include "csics/io/net/NetTypes.hpp"

namespace csics::io::net {

class MQTTMessage {
   public:
    MQTTMessage();
    MQTTMessage(StringView topic, BufferView payload);
    ~MQTTMessage();
    MQTTMessage(const MQTTMessage&) = delete;
    MQTTMessage& operator=(const MQTTMessage&) = delete;
    MQTTMessage(MQTTMessage&& other) noexcept;
    MQTTMessage& operator=(MQTTMessage&& other) noexcept;

    const StringView topic() const { return StringView(topic_); }
    const BufferView payload() const { return payload_; }
    void topic(StringView topic) { topic_ = topic; }
    void payload(BufferView payload) { payload_ = payload; }

    void retain(bool retain) { retained_ = retain; }
    void qos(int qos) { qos_ = qos; }

   private:
    // these can be views because MQTT allocates its own memory for these
    // and we can just point to it
    BufferView payload_;
    StringView topic_;
    void* internal_msg_ = nullptr;  // pointer to the MQTTAsync_message struct
                                    // for cleanup if needed
    int qos_ = 0;
    bool retained_ = false;

    friend class MQTTEndpoint;
};

class MQTTEndpoint {
   public:
    struct Internal;

    MQTTEndpoint(StringView client_id);
    ~MQTTEndpoint();
    MQTTEndpoint(const MQTTEndpoint&) = delete;
    MQTTEndpoint& operator=(const MQTTEndpoint&) = delete;
    MQTTEndpoint(MQTTEndpoint&& other) noexcept;
    MQTTEndpoint& operator=(MQTTEndpoint&& other) noexcept;

    template <typename... Args>
    NetStatus connect(Args&&... args) {
        auto uri = MQTTConnectionParams::from(std::forward<Args>(args)...);
        if (!uri.has_value()) {
            return NetStatus::Error;  // Invalid URI format
        }
        return connect_(uri.value());
    }

    NetResult publish(MQTTMessage&& message);

    NetStatus subscribe(const StringView topic, int qos = 0);

    NetStatus recv(const StringView topic, MQTTMessage& message);

    PollStatus poll(const StringView topic, int timeoutMs);

    static void conn_lost(void* context, char* cause);
    static int msg_arvd(void* context, char* topicName, int topicLen,
                        void* message);
    static void dlv_cmplt(void* context, int token);

    struct MQTTConnectionParams {
        URI broker_uri;
        int qos = 0;
        StringView username;
        StringView password;

        MQTTConnectionParams() = default;

        MQTTConnectionParams(URI broker_uri, int qos = 0,
                             StringView username = {}, StringView password = {})
            : broker_uri(broker_uri),
              qos(qos),
              username(username),
              password(password) {}

        static std::optional<MQTTConnectionParams> from(
            StringView uri, int qos = 0, StringView username = {},
            StringView password = {}) {
            auto parsed_uri = URI::from(uri);
            if (!parsed_uri.has_value()) {
                return std::nullopt;  // Invalid URI format
            }
            return MQTTConnectionParams(parsed_uri.value(), qos, username,
                                        password);
        }
    };

   private:
    Internal* internal_;

    NetStatus connect_(const MQTTConnectionParams& params);
};
};  // namespace csics::io::net
