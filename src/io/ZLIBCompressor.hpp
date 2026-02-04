#include <zlib.h>

#include <csics/io/compression/Compressor.hpp>

namespace csics::io::compression {
class ZLIBCompressor : public ICompressor {
   public:
    ZLIBCompressor();
    ~ZLIBCompressor();

    CompressionStatus init();
    CompressionResult compress_partial(BufferView in, BufferView out) override;
    CompressionResult compress_buffer(BufferView in, BufferView out) override;
    CompressionResult finish(BufferView in, BufferView out) override;

    inline CompressionResult operator()(BufferView in, BufferView out) {
        return compress_buffer(in, out);
    }

   private:
    void* zstream_;
    std::vector<char> leftover_;
    enum class State: uint8_t {
        Compressing,
        Finishing,
        Finished
    } state_ = State::Compressing;
};
};  // namespace csics::io::compression
