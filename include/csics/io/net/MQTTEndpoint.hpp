#pragma once

#ifndef CSICS_USE_MQTT
#error "MQTT support is not enabled. Please define CSICS_USE_MQTT to use MQTTEndpoint."
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
    MQTTMessage(MQTTMessage&& other) noexcept = default;
    MQTTMessage& operator=(MQTTMessage&& other) noexcept = default;

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
    void* internal_msg_;  // pointer to the MQTTAsync_message struct for cleanup if needed
    int qos_;
    bool retained_;

    friend class MQTTEndpoint;
};

class MQTTEndpoint {
   public:
    struct Internal;
    using ConnectionParams = URI;

    MQTTEndpoint(StringView client_id);
    ~MQTTEndpoint();
    MQTTEndpoint(const MQTTEndpoint&) = delete;
    MQTTEndpoint& operator=(const MQTTEndpoint&) = delete;
    MQTTEndpoint(MQTTEndpoint&& other) noexcept;
    MQTTEndpoint& operator=(MQTTEndpoint&& other) noexcept;

    NetStatus connect(const URI& broker_uri);
    NetResult publish(MQTTMessage&& message);

    NetStatus subscribe(const StringView topic);

    NetStatus recv(const StringView topic, MQTTMessage& message);

    PollStatus poll(const StringView topic, int timeoutMs);

    static void conn_lost(void* context, char* cause);
    static int msg_arvd(void* context, char* topicName, int topicLen,
                                void* message);
    static void dlv_cmplt(void* context, int token);

   private:
    Internal* internal_;

};
};  // namespace csics::io::net
