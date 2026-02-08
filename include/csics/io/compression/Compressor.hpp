#pragma once
#include <csics/Buffer.hpp>
#include <cstddef>
#include <memory>

namespace csics::io::compression {
enum class CompressionStatus : uint8_t {
    Ok,
    NeedsInput,
    OutputBufferFull,
    NeedsFlush,
    InputBufferFinished,
    FatalError = (uint8_t)(-128),
    NonFatalError,
    InvalidState
};

enum class CompressorType : uint8_t {
#ifdef CSICS_USE_ZLIB
    ZLIB,
#endif
#ifdef CSICS_USE_ZSTD
    ZSTD,
#endif
};

struct CompressionResult {
    std::size_t
        compressed;  // How many bytes were put into the compressed buffer
    std::size_t
        input_consumed;  // How many bytes were consumed from the input buffer
    CompressionStatus status;
};

class ICompressor {
   public:
    virtual ~ICompressor() = default;
    virtual CompressionResult compress_partial(BufferView<> in,
                                               BufferView<> out) = 0;
    virtual CompressionResult compress_buffer(BufferView<> in,
                                              BufferView<> out) = 0;
    virtual CompressionResult finish(BufferView<> in, BufferView<> out) = 0;

    static std::unique_ptr<ICompressor> create(CompressorType type);
};

};  // namespace csics::io::compression
