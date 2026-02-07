#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <csics/io/compression/Compressor.hpp>

#include "../test_utils.hpp"
#include "gmock/gmock.h"
#include <zlib.h>

TEST(CSICSCompressionTests, ZLIBCompressionTest) {
    using namespace csics::io::compression;
    using namespace csics::io;

    auto compressor = ICompressor::create(CompressorType::ZLIB);

    auto generated_bytes = generate_random_bytes(1024 * 1024);
    std::vector<unsigned char> compressed_buffer(compressBound(1024 * 1024), 0);
    BufferView output_view(compressed_buffer.data(), compressed_buffer.size());
    BufferView input_view(generated_bytes.get(), 1024 * 1024);

    auto result = compressor->compress_buffer(input_view, output_view);

    ASSERT_EQ(result.status, CompressionStatus::NeedsInput);
    ASSERT_NE(result.compressed, 0); // Ensure progress was made
    ASSERT_NE(result.input_consumed, 0); // Ensure progress was made

    output_view += result.compressed;
    input_view += result.input_consumed;

    ASSERT_FALSE(output_view.empty());
    ASSERT_TRUE(input_view.empty());

    result = compressor->finish(input_view, output_view);
    output_view += result.compressed;
    auto compressed_size = output_view.uc() - compressed_buffer.data();

    ASSERT_EQ(result.status, CompressionStatus::InputBufferFinished);
    ASSERT_NE(compressed_size, 0); // Ensure progress was made
    std::cout << "Compressed size: " << compressed_size << " bytes" << std::endl;

    // decompress to test

    std::vector<unsigned char> decompressed_data(1024 * 1024, 0);
    z_stream stream{};
    inflateInit(&stream);

    stream.next_in = compressed_buffer.data();
    stream.avail_in = compressed_size;
    stream.total_in = 0;
    stream.next_out = decompressed_data.data();
    stream.total_out = 0;
    stream.avail_out = decompressed_data.size();

    int zout = inflate(&stream, Z_FINISH);

    ASSERT_EQ(zout, Z_STREAM_END) << "ZLIB did not return Z_STREAM_END. Error msg: " << stream.msg;

    ASSERT_THAT(
        decompressed_data,
        testing::ElementsAreArray(reinterpret_cast<char*>(generated_bytes.get()),
                             1024 * 1024));
}
