
#include <MQTTAsync.h>

#include <chrono>
#include <csics/io/net/MQTTEndpoint.hpp>
#include <thread>
#include <unordered_map>

#include "csics/queue/queue.hpp"

namespace csics::io::net {

MQTTMessage::MQTTMessage() : payload_(), topic_(), internal_msg_(nullptr) {};
MQTTMessage::~MQTTMessage() {
    if (internal_msg_) {
        MQTTAsync_message* msg = static_cast<MQTTAsync_message*>(internal_msg_);
        MQTTAsync_freeMessage(&msg);
        MQTTAsync_free(const_cast<char*>(topic_.data()));
    };
};

MQTTMessage::MQTTMessage(StringView topic, BufferView payload)
    : payload_(payload), topic_(topic) {};

struct MQTTEndpoint::Internal {
    MQTTAsync client;
    std::unordered_map<std::string, queue::SPSCMessageQueue<MQTTMessage>>
        topic_queues;
    std::mutex topic_queues_mutex;
    using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;
    std::vector<std::tuple<TimeStamp, MQTTAsync_token, MQTTMessage>>
        pending_messages;
    String client_id;
};

void conn_lost_cb(void* ctx, char* cause) {
    MQTTEndpoint::conn_lost(ctx, cause);
};

int message_arrived_cb(void* context, char* topicName, int topicLen,
                       MQTTAsync_message* message) {
    return MQTTEndpoint::msg_arvd(context, topicName, topicLen, message);
};

void delivery_complete_cb(void* context, MQTTAsync_token token) {
    MQTTEndpoint::dlv_cmplt(context, token);
};

MQTTEndpoint::MQTTEndpoint(StringView client_id) : internal_(new Internal()) {
    internal_->client_id = String(client_id);
};
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

NetStatus MQTTEndpoint::connect_(const URI& broker_uri) {
    std::cerr << "Connecting to MQTT broker at " << broker_uri.str()
              << " with client ID " << internal_->client_id << std::endl;
    MQTTAsync_create(&internal_->client, broker_uri.c_str(),
                     internal_->client_id.c_str(), MQTTCLIENT_PERSISTENCE_NONE,
                     nullptr);

    std::unique_ptr<MQTTAsync_SSLOptions> ssl_opts = nullptr;

    if (broker_uri.scheme() == "ssl" || broker_uri.scheme() == "mqtts") {
        ssl_opts = std::make_unique<MQTTAsync_SSLOptions>();
        *ssl_opts = MQTTAsync_SSLOptions_initializer;
    };

    std::atomic<int> connected{0};
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 30;
    conn_opts.cleansession = 1;
    conn_opts.ssl = ssl_opts.get();
    conn_opts.context = reinterpret_cast<void*>(&connected);

    conn_opts.onSuccess = [](void* context, MQTTAsync_successData*) {
        static_cast<std::atomic<int>*>(context)->store(
            1, std::memory_order_release);
        std::cerr << "MQTT connection successful" << std::endl;
    };

    conn_opts.onFailure = [](void* context, MQTTAsync_failureData*) {
        static_cast<std::atomic<int>*>(context)->store(
            -1, std::memory_order_release);
    };

    MQTTAsync_setCallbacks(internal_->client, static_cast<void*>(&internal_),
                           &conn_lost_cb, &message_arrived_cb,
                           &delivery_complete_cb);

    int err = MQTTASYNC_SUCCESS;
    if ((err = MQTTAsync_connect(internal_->client, &conn_opts)) !=
        MQTTASYNC_SUCCESS) {
        return NetStatus::Error;
    }

    auto now = std::chrono::steady_clock::now();
    while (connected.load(std::memory_order_acquire) == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (std::chrono::steady_clock::now() - now > std::chrono::seconds(10)) {
            return NetStatus::Timeout;
        }
    }
    if (connected.load(std::memory_order_acquire) == -1) {
        return NetStatus::Error;
    }

    return NetStatus::Success;
}

void MQTTEndpoint::conn_lost(void*, char*) {
    // TODO: Handle connection lost
}

void MQTTEndpoint::dlv_cmplt(void* context, int token) {
    auto* internal = static_cast<MQTTEndpoint::Internal*>(context);
    std::erase_if(internal->pending_messages, [token](const auto& entry) {
        return std::get<1>(entry) == token;
    });
}

int MQTTEndpoint::msg_arvd(void* context, char* topicName, int topicLen,
                           void* message_) {
    auto* internal = static_cast<MQTTEndpoint::Internal*>(context);
    CSICS_RUNTIME_ASSERT(topicName != nullptr,
                         "Received message with null topic");
    CSICS_RUNTIME_ASSERT(message_ != nullptr, "Received null message");
    CSICS_RUNTIME_ASSERT(internal != nullptr,
                         "MQTT message arrived with null context");

    MQTTMessage msg{};
    msg.internal_msg_ = message_;
    topicLen = std::min(topicLen, static_cast<int>(strlen(topicName)));
    msg.topic_ = StringView(topicName, topicLen);
    auto key = std::string(topicName, topicLen);

    std::lock_guard<std::mutex> lock(internal->topic_queues_mutex);
    internal->topic_queues.try_emplace(key, 1024);

    auto ret =
        internal->topic_queues.find(key)->second.try_push(std::move(msg));
    if (ret != queue::SPSCError::None) {
        // TODO: handle queue overflow, e.g., by dropping the message or logging
        // an error
        return 0;  // Indicate failure to process the message
    }
    //std::cerr << "Successfully received MQTT message for topic " << key << std::endl;

    return 1;
}

NetResult MQTTEndpoint::publish(MQTTMessage&& message) {
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    int err = MQTTAsync_send(internal_->client, message.topic().data(),
                             message.payload().size(), message.payload().data(),
                             message.qos_, message.retained_, &opts);
    if (err != MQTTASYNC_SUCCESS) {
        return {NetStatus::Error, 0};
    }

    internal_->pending_messages.emplace_back(std::chrono::steady_clock::now(),
                                             opts.token, std::move(message));

    return {NetStatus::Success, message.payload().size()};
};

NetStatus MQTTEndpoint::subscribe(const StringView topic) {
    int err = MQTTAsync_subscribe(internal_->client, topic.data(), 0, nullptr);
    if (err != MQTTASYNC_SUCCESS) {
        return NetStatus::Error;
    }
    auto key = std::string(topic.data(), topic.size());
    std::lock_guard<std::mutex> lock(internal_->topic_queues_mutex);
    internal_->topic_queues.try_emplace(key, 1024);
    return NetStatus::Success;
}

NetStatus MQTTEndpoint::recv(const StringView topic, MQTTMessage& message) {
    std::lock_guard<std::mutex> lock(internal_->topic_queues_mutex);
    auto queue =
        internal_->topic_queues.find(std::string(topic.data(), topic.size()));
    if (queue == internal_->topic_queues.end()) {
        return NetStatus::Error;  // Not subscribed to this topic
    }

    if (queue->second.empty()) {
        return NetStatus::Empty;
    }

    MQTTMessage msg{};
    auto ret = queue->second.try_pop(msg);
    if (ret == queue::SPSCError::Empty) {
        return NetStatus::Empty;
    } else if (ret != queue::SPSCError::None) {
        return NetStatus::Error;
    }
    message = std::move(msg);
    return NetStatus::Success;
};

PollStatus MQTTEndpoint::poll(const StringView topic, int timeoutMs) {
    {
        std::lock_guard<std::mutex> lock(internal_->topic_queues_mutex);
        auto queue = internal_->topic_queues.find(
            std::string(topic.data(), topic.size()));
        if (queue == internal_->topic_queues.end()) {
            return PollStatus::Error;  // Not subscribed to this topic
        }
    }

    // simple check if the queue is not empty
    auto now = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - now <
           std::chrono::milliseconds(timeoutMs)) {

        {
            std::lock_guard<std::mutex> lock(internal_->topic_queues_mutex);

            auto queue_ref = internal_->topic_queues.find(
                    std::string(topic.data(), topic.size()));

            if (queue_ref != internal_->topic_queues.end() &&
                    !queue_ref->second.empty()) {
                return PollStatus::Ready;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    };

    return PollStatus::Timeout;
}

};  // namespace csics::io::net
