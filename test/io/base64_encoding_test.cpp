#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <csics/io/encdec/Base64.hpp>
#include <fstream>
#include <vector>
#include <random>
#include "../test_utils.hpp"
#include "compression_utils.hpp"
#include <openssl/evp.h>

TEST(CSICSEncDecTests, Base64EncodingTest) {
    using namespace csics::io::encdec;
    using namespace csics::io;

    Base64Encoder encoder;

    std::vector<uint8_t> input_data = { 'M', 'a', 'n' };
    BufferView input_buffer(input_data.data(), input_data.size());
    std::vector<uint8_t> output_data(4);
    BufferView output_buffer(output_data.data(), output_data.size());
    EXPECT_EQ(output_buffer.size(), 4);
    EXPECT_EQ(input_buffer.size(), 3);
    EncodingResult result = encoder.encode(input_buffer.as<char>(), output_buffer.as<char>());
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
    using namespace csics::io;

    Base64Encoder encoder;

    std::vector<uint8_t> input_data = { 'M', 'a' };
    BufferView input_buffer(input_data.data(), input_data.size());
    std::vector<uint8_t> output_data(4);
    BufferView output_buffer(output_data.data(), output_data.size());
    EXPECT_EQ(output_buffer.size(), 4);
    EXPECT_EQ(input_buffer.size(), 2);
    EncodingResult result = encoder.encode(input_buffer.as<char>(), output_buffer.as<char>());
    EXPECT_EQ(result.status, EncodingStatus::Ok);
    EXPECT_EQ(result.processed, 2);
    EXPECT_EQ(result.output, 0); // No output yet, need to finish for padding

    // Finish encoding to handle padding
    BufferView empty_input;
    result = encoder.finish(empty_input, output_buffer.as<char>());
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
    result = encoder.encode(input_buffer.as<char>(), output_buffer.as<char>());
    EXPECT_EQ(result.status, EncodingStatus::Ok);
    EXPECT_EQ(result.processed, 1);
    EXPECT_EQ(result.output, 0); // No output yet, need to finish for padding

    // Finish encoding to handle padding
    result = encoder.finish(empty_input, output_buffer.as<char>());
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
    using namespace csics::io;
    Base64Encoder encoder;
    BufferView<std::byte> input;
    BufferView<std::uint8_t> output;
    for (size_t i = 0; i < 100; i++) {
        auto rand_size = std::rand() % (int)2048;  //(size_t)4e6;
        auto generated_bytes = generate_random_bytes(rand_size);
        std::vector<uint8_t> output_buf(4 * (rand_size + 2) / 3, 0);
        input = BufferView(generated_bytes.get(), rand_size);
        output = BufferView(output_buf.data(), output_buf.size());

        auto r = encoder.finish(input.as<char>(), output.as<char>());
        ASSERT_THAT(r.status, EncodingStatus::Ok);
        output = BufferView(output.data(), r.output);

        std::vector<uint8_t> encoded_data(r.output, 0);

        int actual_len = EVP_EncodeBlock(encoded_data.data(), input.uc(), input.size());
        ASSERT_EQ(actual_len, r.output);

        ASSERT_THAT(output, ::testing::ElementsAreArray(encoded_data));

        std::vector<uint8_t> decoded_data(input.size(), 0);
        actual_len = EVP_DecodeBlock(decoded_data.data(), output.uc(), output.size());
        if (input.size() % 3 == 1) {
            EXPECT_EQ(actual_len, input.size() + 2); // EVP_DecodeBlock includes padding in output length
        } else if (input.size() % 3 == 2) {
            EXPECT_EQ(actual_len, input.size() + 1); // EVP_DecodeBlock includes padding in output length
        } else {
            EXPECT_EQ(actual_len, input.size()); // No padding, output length should match input size
        }

        ASSERT_THAT(input.as<uint8_t>(), ::testing::ElementsAreArray(decoded_data));
    }
}
