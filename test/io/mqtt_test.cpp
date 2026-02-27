#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <csics/csics.hpp>

TEST(CSICSNetTests, MQTTTest) {
    using namespace csics::io::net;

    URI uri = *URI::from("mqtt://broker.hivemq.com:1883");
    ASSERT_EQ(uri.scheme(), csics::StringView("mqtt"));
    ASSERT_EQ(uri.host(), csics::StringView("broker.hivemq.com"));
    ASSERT_EQ(uri.port(), 1883);
    ASSERT_TRUE(uri.path().empty());

    csics::String topic = "csics/test/basic";

    MQTTEndpoint client("csics_test_client_basic");

    auto res = client.connect("mqtt://broker.hivemq.com:1883");
    EXPECT_EQ(res, NetStatus::Success);
    client.subscribe(topic);
    res =
        client
            .publish(MQTTMessage(topic, csics::BufferView("Hello, MQTT!", 12)))
            .status;
    EXPECT_EQ(res, NetStatus::Success);
    MQTTMessage msg;
    PollStatus poll_res = client.poll(topic, 10000);
    EXPECT_EQ(poll_res, PollStatus::Ready);

    res = client.recv(topic, msg);
    ASSERT_EQ(res, NetStatus::Success);
    ASSERT_EQ(msg.topic(), topic);
    ASSERT_EQ(msg.payload(), csics::BufferView("Hello, MQTT!", 12));
}
