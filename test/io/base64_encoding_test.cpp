#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <openssl/evp.h>

#include <csics/io/encdec/Base64.hpp>
#include <fstream>
#include <random>
#include <vector>

#include "../test_utils.hpp"
#include "compression_utils.hpp"

TEST(CSICSEncDecTests, Base64EncodingTest) {
    using namespace csics::io::encdec;
    using namespace csics;

    Base64Encoder encoder;

    std::vector<uint8_t> input_data = {'M', 'a', 'n'};
    BufferView input_buffer(input_data.data(), input_data.size());
    std::vector<uint8_t> output_data(4);
    BufferView output_buffer(output_data.data(), output_data.size());
    EXPECT_EQ(output_buffer.size(), 4);
    EXPECT_EQ(input_buffer.size(), 3);
    EncodingResult result = encoder.encode(input_buffer, output_buffer);
    EXPECT_EQ(result.status, EncodingStatus::Ok);
    EXPECT_EQ(result.processed, 3);
    EXPECT_EQ(result.output, 4);
    EXPECT_EQ(output_data[0], 'T');
    EXPECT_EQ(output_data[1], 'W');
    EXPECT_EQ(output_data[2], 'F');
    EXPECT_EQ(output_data[3], 'u');
}

TEST(CSICSEncDecTests, Base64EncodingWithPaddingTest) {
    using namespace csics::io::encdec;
    using namespace csics;

    Base64Encoder encoder;

    std::vector<uint8_t> input_data = {'M', 'a'};
    BufferView input_buffer(input_data.data(), input_data.size());
    std::vector<uint8_t> output_data(4);
    BufferView output_buffer(output_data.data(), output_data.size());
    EXPECT_EQ(output_buffer.size(), 4);
    EXPECT_EQ(input_buffer.size(), 2);
    EncodingResult result = encoder.encode(input_buffer, output_buffer);
    EXPECT_EQ(result.status, EncodingStatus::Ok);
    EXPECT_EQ(result.processed, 2);
    EXPECT_EQ(result.output, 0);  // No output yet, need to finish for padding

    // Finish encoding to handle padding
    BufferView empty_input;
    result = encoder.finish(empty_input, output_buffer);
    EXPECT_EQ(result.status, EncodingStatus::Ok);
    EXPECT_EQ(result.processed, 0);
    EXPECT_EQ(result.output, 4);
    EXPECT_EQ(output_data[0], 'T');
    EXPECT_EQ(output_data[1], 'W');
    EXPECT_EQ(output_data[2], 'E');
    EXPECT_EQ(output_data[3], '=');

    input_data = {'M'};
    input_buffer = BufferView(input_data.data(), input_data.size());
    output_buffer = BufferView(output_data.data(), output_data.size());
    EXPECT_EQ(output_buffer.size(), 4);
    EXPECT_EQ(input_buffer.size(), 1);
    result = encoder.encode(input_buffer, output_buffer);
    EXPECT_EQ(result.status, EncodingStatus::Ok);
    EXPECT_EQ(result.processed, 1);
    EXPECT_EQ(result.output, 0);  // No output yet, need to finish for padding

    // Finish encoding to handle padding
    result = encoder.finish(empty_input, output_buffer);
    EXPECT_EQ(result.status, EncodingStatus::Ok);
    EXPECT_EQ(result.processed, 0);
    EXPECT_EQ(result.output, 4);
    EXPECT_EQ(output_data[0], 'T');
    EXPECT_EQ(output_data[1], 'Q');
    EXPECT_EQ(output_data[2], '=');
    EXPECT_EQ(output_data[3], '=');
}

TEST(CSICSEncDecTests, Base64EncodingFuzzTest) {
    using namespace csics::io::encdec;
    using namespace csics;
    Base64Encoder encoder;
    BufferView input;
    BufferView output;
    
    for (size_t i = 0; i < 10000; i++) {
        auto rand_size = (std::rand() % (int)1000) + 1;  //(size_t)4e6;
        auto generated_bytes = generate_random_bytes(rand_size);
        std::vector<uint8_t> output_buf(4 * ((rand_size + 2) / 3) + 16, 0); // align to 16 bytes for EVP on apple silicon
        input = BufferView(generated_bytes);
        ASSERT_TRUE(input);
        output = BufferView(output_buf);
        ASSERT_TRUE(output);
        ASSERT_EQ(output.size(), output_buf.size());
        auto out_before = output;

        auto r = encoder.finish(input, output);
        ASSERT_LE(r.output, output.size());

        ASSERT_TRUE(std::equal(output_buf.begin() + r.output, output_buf.end(),
                               out_before.begin() + r.output));

        ASSERT_EQ(r.status, EncodingStatus::Ok);
        ASSERT_EQ(r.processed, input.size());
        ASSERT_THAT(r.output, ::testing::Le(output_buf.size()));
        auto written_output = output(0, r.output);
        auto untouched_output = output(r.output, output.size() - r.output);
        ASSERT_THAT(untouched_output,
                    ::testing::ElementsAreArray(std::vector<uint8_t>(
                        output_buf.begin() + r.output, output_buf.end())));
        ASSERT_TRUE(output);

        std::vector<uint8_t> encoded_data(output_buf.size(), 0);

        int actual_len =
            EVP_EncodeBlock(encoded_data.data(), input.u8(), input.size());
        ASSERT_EQ(actual_len, r.output);
        encoded_data.resize(r.output);  // Resize to actual encoded size

        ASSERT_THAT(std::span<uint8_t>(written_output.u8(), written_output.size()),
                    ::testing::ElementsAreArray(encoded_data));

        std::vector<uint8_t> decoded_data(3 * (r.output / 4), 0);
        actual_len =
            EVP_DecodeBlock(decoded_data.data(), output.u8(), r.output);
        auto padding_len = input.size() % 3;
        ASSERT_GE(actual_len, input.size());

        decoded_data.resize(input.size());  // Resize to expected decoded size

        ASSERT_TRUE(input);
        ASSERT_FALSE(input.empty());
        ASSERT_EQ(0,
                  std::memcmp(decoded_data.data(), input.u8(), input.size()));
    }
}
