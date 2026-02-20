
#pragma once
#include <MQTTAsync.h>

#include <csics/io/net/MQTTEndpoint.hpp>

#include "csics/queue/queue.hpp"

namespace csics::io::net {

MQTTMessage::MQTTMessage() : payload_(), topic_() {};
MQTTMessage::~MQTTMessage() {
    if (payload_) {
        delete[] payload_.data();
    }
    if (topic_) {
        delete[] topic_.data();
    }
};

MQTTMessage::MQTTMessage(MQTTMessage&& other) noexcept
    : payload_(other.payload_), topic_(other.topic_) {
    other.payload_ = BufferView();
    other.topic_ = StringView();
};

MQTTMessage& MQTTMessage::operator=(MQTTMessage&& other) noexcept {
    if (this != &other) {
        if (payload_) {
            delete[] payload_.data();
        }
        if (topic_) {
            delete[] topic_.data();
        }
        payload_ = other.payload_;
        topic_ = other.topic_;
        other.payload_ = BufferView();
        other.topic_ = StringView();
    }
    return *this;
};

MQTTMessage::MQTTMessage(StringView topic, BufferView payload)
    : payload_(payload), topic_(topic) {};

struct MQTTEndpoint::Internal {
    MQTTAsync client;
    std::unordered_map<std::string, queue::SPSCMessageQueue<MQTTMessage>>
        topic_queues;
};
MQTTEndpoint::MQTTEndpoint(BufferView client_id) : internal_(new Internal()) {};
MQTTEndpoint::~MQTTEndpoint() { delete internal_; };

MQTTEndpoint::MQTTEndpoint(MQTTEndpoint&& other) noexcept
    : internal_(other.internal_) {
    other.internal_ = nullptr;
};

MQTTEndpoint& MQTTEndpoint::operator=(MQTTEndpoint&& other) noexcept {
    if (this != &other) {
        delete internal_;
        internal_ = other.internal_;
        other.internal_ = nullptr;
    }
    return *this;
};

NetStatus MQTTEndpoint::connect(const URI& broker_uri) {

    if (broker_uri.scheme() == "ssl" || broker_uri.scheme() == "mqtts") {
    };
    return NetStatus::Success;
}


};  // namespace csics::io::net
