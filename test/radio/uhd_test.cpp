#include <gtest/gtest.h>
#include <csics/radio/Radio.hpp>
#include <csics/radio/RadioRx.hpp>


static inline std::unique_ptr<csics::radio::IRadioRx> create_usrp_radio() {
    csics::radio::UsrpArgs args;
    csics::radio::RadioConfiguration config;
    return csics::radio::IRadioRx::create_radio_rx(args, config);
}

TEST(CSICSRadioTests, UHDBasicTest) {
    auto radio = create_usrp_radio();
    if (radio == nullptr) {
        GTEST_SKIP() << "Failed to create USRP radio receiver. Is a USRP device connected?";
    }
}

TEST(CSICSRadioTests, UHDBasicRxTest) {
    using namespace std::chrono_literals;
    using namespace std::chrono;
    auto radio = create_usrp_radio();
    if (radio == nullptr) {
        GTEST_SKIP() << "Failed to create USRP radio receiver. Is a USRP device connected?";
    }

    csics::radio::StreamConfiguration stream_config;
    stream_config.sample_length = csics::radio::SampleLength(1024);
    auto start_status = radio->start_stream(stream_config);
    ASSERT_EQ(start_status.code, csics::radio::IRadioRx::StartStatus::Code::SUCCESS)
        << "Failed to start USRP radio stream. Error code: " << static_cast<int>(start_status.code);

    auto read = std::move(start_status.rx_handle);
    csics::queue::SPSCQueue::ReadSlot rs{};
    csics::queue::SPSCError result = csics::queue::SPSCError::Empty;
    auto dur = 10s;
    auto start_time = steady_clock::now();
    while ((result = read->acquire(rs)) == csics::queue::SPSCError::Empty) {
        if (steady_clock::now() - start_time > dur) {
            FAIL() << "Failed to acquire read slot from USRP radio stream after 5 seconds. Stream may not be producing data.";
        }
    }
    ASSERT_TRUE(result == csics::queue::SPSCError::None || result == csics::queue::SPSCError::Empty)
        << "Failed to acquire read slot from USRP radio stream. Error code: " << static_cast<int>(result);
    ASSERT_EQ(rs.size, 1024 * sizeof(std::complex<int16_t>) + sizeof(csics::radio::IRadioRx::BlockHeader))
        << "Received block size does not match expected size. Expected: " << 1024 * sizeof(std::complex<int16_t>) << ", Got: " << rs.size;
    csics::radio::IRadioRx::BlockHeader* hdr;
    csics::radio::SDRRawSample* samples;

    rs.as_block(hdr, samples);
    ASSERT_NE(hdr, nullptr) << "Block header pointer is null.";
    ASSERT_NE(samples, nullptr) << "Samples pointer is null.";
    ASSERT_EQ(hdr->num_samples, 1024) << "Number of samples in block header does not match expected value. Expected: 1024, Got: " << hdr->num_samples;

    radio->stop_stream();
}
