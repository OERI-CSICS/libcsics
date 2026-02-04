#pragma once
#include <csics/io/compression/Compressor.hpp>

namespace csics::io::compression {

class ZSTDCompressor: public ICompressor {
   public:
    explicit ZSTDCompressor();
    ~ZSTDCompressor();
    CompressionResult compress_partial(BufferView in, BufferView out) override;
    CompressionResult compress_buffer(BufferView in, BufferView out) override;
    CompressionResult finish(BufferView in, BufferView out) override;

   private:
    void* stream_;
};
};  // namespace csics::io::compression
