#pragma once
#ifndef CSICS_BUILD_IO
#error "IO support is not enabled. Please define CSICS_BUILD_IO to use encoding/decoding features."
#endif
#include <cstddef>
#include <cstdint>
namespace csics::io::encdec {
    enum class EncodingStatus : uint8_t {
        Ok,
        OutputBufferFull,
        NeedsInput,
        NeedsFlush,
        InvalidInput,
        FatalError
    };

    struct EncodingResult {
        std::size_t processed; // How many bytes were processed from the input buffer
        std::size_t output;    // How many bytes were written to the output buffer
        EncodingStatus status;
    };
};
