#include <gtest/gtest.h>

#include <csics/queue/SPSCQueue.hpp>
#include <cstring>
#include <memory>
#include <random>
// right now, instantation testing
struct HeaderTest {
    int a;
    float b;
};

struct DataTest {
    double x;
    char y;
};

static bool binary_arr_eq(std::byte* arr1, std::byte* arr2, std::size_t size) {
    for (std::size_t i = 0; i < size; i++) {
        if (arr1[i] != arr2[i]) {
            return false;
        }
    }
    return true;
}

static std::unique_ptr<std::byte[]> generate_random_bytes(std::size_t size) {
    auto buffer = std::make_unique<std::byte[]>(size);

    // Thread-local to avoid reseeding costs if used frequently
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint32_t> dist(0, 255);

    for (std::size_t i = 0; i < size; ++i) {
        buffer[i] = static_cast<std::byte>(dist(rng));
    }

    return buffer;
}

TEST(CSICSQueueTests, BasicReadWrite) {
    using namespace csics::queue;

    SPSCQueue q(1024);

    SPSCQueue::WriteSlot ws{};

    ASSERT_EQ(q.acquire_write(ws, 512), SPSCError::None);

    const char mystr[] = "Hello world!";

    memcpy(ws.data, mystr, sizeof(mystr));
    q.commit_write(std::move(ws));

    SPSCQueue::ReadSlot rs{};

    ASSERT_EQ(q.acquire_read(rs), SPSCError::None);
    ASSERT_EQ(rs.size, ws.size);
    ASSERT_STREQ(reinterpret_cast<char*>(rs.data), mystr);
}

TEST(CSICSQueueTests, BasicReadWriteSmall) {
    using namespace csics::queue;
    SPSCQueue::WriteSlot ws{};
    SPSCQueue::ReadSlot rs{};
    SPSCQueue q(5);

    std::byte pattern[] = {std::byte{0}, std::byte{1}, std::byte{2},
                           std::byte{3}};

    std::size_t size = 4;
    ASSERT_EQ(q.acquire_write(ws, size), SPSCError::None);
    std::memcpy(ws.data, pattern, size);
    q.commit_write(std::move(ws));
    ASSERT_EQ(q.acquire_read(rs), SPSCError::None);
    ASSERT_EQ(rs.size, size);
    ASSERT_PRED3(binary_arr_eq, rs.data, &pattern[0], size);
    q.commit_read(std::move(rs));

    pattern[0] = std::byte{8};

    ASSERT_EQ(q.acquire_write(ws, size), SPSCError::None);
    std::memcpy(ws.data, pattern, size);
    q.commit_write(std::move(ws));
    ASSERT_EQ(q.acquire_read(rs), SPSCError::None);
    ASSERT_PRED3(binary_arr_eq, rs.data, &pattern[0], size);
    ASSERT_EQ(rs.size, size);
    q.commit_read(std::move(rs));
}

TEST(CSICSQueueTests, FuzzReadWriteSingleThreaded) {
    using namespace csics::queue;
    SPSCQueue::WriteSlot ws{};
    SPSCQueue::ReadSlot rs{};
    SPSCQueue q(1053);
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> dist(1, 1052 / 2);
    std::size_t total_size = 0;

    for (std::size_t i = 0; i < 1000000; i++) {
        std::size_t size = dist(rng);
        total_size += size;
        auto pattern = generate_random_bytes(size);
        ASSERT_EQ(q.acquire_write(ws, size), SPSCError::None);
        std::memcpy(ws.data, pattern.get(), size);
        q.commit_write(std::move(ws));
        ASSERT_EQ(q.acquire_read(rs), SPSCError::None);
        ASSERT_EQ(rs.size, size)
            << "Error on iteration " << i << " \nWith total size " << total_size
            << " \nws.data: " << ws.data << ", rs.data: " << rs.data;
        ASSERT_PRED3(binary_arr_eq, rs.data, &pattern[0], size);
        q.commit_read(std::move(rs));
    }
}

TEST(CSICSQueueTests, ReadWriteMultiThreaded) {
#ifdef _MSC_VER
    GTEST_SKIP() << "Doesn't run in MSVC";
#endif
    using namespace csics::queue;
    SPSCQueue q(1053);
    std::size_t iterations = 10000000;

    auto t1 = std::thread([&]() {
        SPSCQueue::WriteSlot ws{};
        for (std::size_t i = 0; i < iterations; i++) {
            auto result = q.acquire_write(ws, sizeof(std::size_t));
            ASSERT_TRUE(result == SPSCError::None || result == SPSCError::Full);
            if (result == SPSCError::Full) {
                i--;
                continue;
            }
            std::memcpy(ws.data, &i, sizeof(std::size_t));
            q.commit_write(std::move(ws));
        }
    });

    auto t2 = std::thread([&]() {
        SPSCQueue::ReadSlot rs{};
        for (std::size_t i = 0; i < iterations; i++) {
            auto result = q.acquire_read(rs);
            ASSERT_TRUE(result == SPSCError::None || result == SPSCError::Empty);
            if (result == SPSCError::Empty) {
                i--;
                continue;
            }
            std::size_t val = *reinterpret_cast<std::size_t*>(rs.data);
            ASSERT_EQ(val, i);
            q.commit_read(std::move(rs));
        }
    });

    t1.join();
    t2.join();
}
