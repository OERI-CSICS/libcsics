
#include <gtest/gtest.h>
#include <csics/csics.hpp>
#include "../test_utils.hpp"


TEST(CSICSQueueTests, BasicMessageQueue) {
    using namespace csics::queue;

    SPSCMessageQueue<std::string> q(1024);

    std::string msg = "Hello, SPSCMessageQueue!";
    ASSERT_EQ(q.try_push(msg), SPSCError::None);

    std::string received;
    ASSERT_EQ(q.try_pop(received), SPSCError::None);
    ASSERT_EQ(received, msg);
}

TEST(CSICSQueueTests, FuzzMessageQueueSingleThreaded) {
    using namespace csics::queue;

    SPSCMessageQueue<std::vector<uint8_t>> q(1024);

    csics::Random random;
    std::vector<std::vector<uint8_t>> patterns;
    for (size_t i = 0; i < 1000; i++) {
        size_t size = random.uniform<uint8_t>();
        auto pattern = generate_random_bytes(size);
        patterns.push_back(pattern);
        ASSERT_EQ(q.try_push(pattern), SPSCError::None) << "Failed to push pattern of size " << size << " on iteration " << i;
    }

    for (size_t i = 0; i < patterns.size(); i++) {
        std::vector<uint8_t> received;
        ASSERT_EQ(q.try_pop(received), SPSCError::None);
        ASSERT_EQ(received, patterns[i]);
    }
}

struct NonTrivial {
    NonTrivial() = default;
    NonTrivial(int n) : data(n, n) {}
    std::vector<int> data;
    bool operator==(const NonTrivial& other) const {
        return data == other.data;
    }
};

TEST(CSICSQueueTests, MessageQueueInMap) {
    using namespace csics::queue;

    std::unordered_map<std::string, SPSCMessageQueue<NonTrivial>> map;
    map.try_emplace("queue1", 1024);
    map.try_emplace("queue2", 1024);

    ASSERT_EQ(map.at("queue1").try_push(42), SPSCError::None);
    ASSERT_EQ(map.at("queue2").try_push(84), SPSCError::None);

    NonTrivial val1, val2;
    ASSERT_EQ(map.at("queue1").try_pop(val1), SPSCError::None);
    ASSERT_EQ(map.at("queue2").try_pop(val2), SPSCError::None);

    ASSERT_EQ(val1, 42);
    ASSERT_EQ(val2, 84);
}
