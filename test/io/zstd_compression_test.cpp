#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <zstd.h>

#include <csics/io/compression/Compressor.hpp>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

#include "../test_utils.hpp"
#include "compression_utils.hpp"

TEST(CSICSCompressionTests, ZSTDCompressorBasic) {
    using namespace csics::io::compression;
    using namespace csics;
    using ::testing::ElementsAreArray;

    auto compressor = ICompressor::create(CompressorType::ZSTD);
    constexpr std::size_t data_size = 1024 * 1024;  // 1 MB
    auto input_data = generate_random_bytes(data_size);
    std::vector<unsigned char> compressed_data;
    compressed_data.resize(ZSTD_compressBound(data_size));
    BufferView in_buffer(input_data);
    BufferView out_buffer(compressed_data);

    CompressionResult result{};
    std::size_t size = 0;

    result = compressor->compress_buffer(in_buffer,
                                         out_buffer);
    in_buffer += result.input_consumed;
    out_buffer += result.compressed;

    ASSERT_EQ(result.status, CompressionStatus::InputBufferFinished);
    size += result.compressed;

    result = compressor->finish(in_buffer, out_buffer);

    ASSERT_EQ(result.status, CompressionStatus::InputBufferFinished);
    size += result.compressed;

    std::ofstream outfile("temp_compressed.zst", std::ios::binary);

    outfile.write(reinterpret_cast<char*>(compressed_data.data()), size);
    outfile.flush();
    outfile.close();

    std::vector<char> decompressed_data =
        run_cmdline("zstd -d %s -o %s", "temp_compressed.zst");

    ASSERT_EQ(decompressed_data.size(), data_size);
    ASSERT_THAT(input_data, ::testing::ElementsAreArray(decompressed_data));

    std::filesystem::remove("temp_compressed.zst");
}
