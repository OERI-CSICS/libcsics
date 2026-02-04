#include <csics/io/compression/Compressor.hpp>
#include "ZLIBCompressor.hpp"
#include "ZSTDCompressor.hpp"

namespace csics::io::compression {

    std::unique_ptr<ICompressor> ICompressor::create(CompressorType type) {
        switch (type) {
#ifdef CSICS_USE_ZLIB
            case CompressorType::ZLIB:
                return std::make_unique<ZLIBCompressor>();
#endif
#ifdef CSICS_USE_ZSTD
            case CompressorType::ZSTD:
                return std::make_unique<ZSTDCompressor>();
#endif
            default:
                throw std::invalid_argument("Unsupported compressor type");
        }
    }
};
