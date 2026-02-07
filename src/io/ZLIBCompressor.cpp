#include <zlib.h>

#include "ZLIBCompressor.hpp"
#include <cstring>

namespace csics::io::compression {

ZLIBCompressor::ZLIBCompressor(): zstream_(new z_stream), state_(State::Compressing) {
    z_stream* zstream = static_cast<z_stream*>(zstream_);
    std::memset(zstream, 0, sizeof(z_stream));
    int ret = deflateInit(zstream, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        throw std::runtime_error("Failed to initialize ZLIB compressor");
    }
}

CompressionStatus ZLIBCompressor::init() {
    auto zstream = static_cast<z_streamp>(zstream_);
    int ret = deflateReset(zstream);
    if (ret != Z_OK) {
        return CompressionStatus::FatalError;
    }
    state_ = State::Compressing;
    return CompressionStatus::Ok;
}

ZLIBCompressor::~ZLIBCompressor() {
    if (zstream_ != nullptr) {
        auto zstream = static_cast<z_streamp>(zstream_);
        deflateEnd(zstream);
        delete zstream;
        zstream_ = nullptr;
    }
}

static void set_zstream(z_streamp z, BufferView in, BufferView out) {
    z->next_in = in.uc();
    z->avail_in = in.size();
    z->next_out = out.uc();
    z->avail_out = out.size();
}

CompressionResult ZLIBCompressor::compress_partial(BufferView in,
                                                   BufferView out) {
    auto* zstream = static_cast<z_streamp>(zstream_);
    set_zstream(zstream, in, out);

    int zout = deflate(zstream, Z_NO_FLUSH);

    CompressionResult ret{};
    ret.compressed = zstream->next_out - out.uc();
    ret.input_consumed = zstream->next_in - in.uc();

    switch (zout) {
        case Z_OK:
            ret.status = CompressionStatus::Ok;
            if (zstream->avail_out == 0) {
                ret.status = CompressionStatus::OutputBufferFull;
            } else if (zstream->avail_in == 0) {
                ret.status = CompressionStatus::NeedsInput;
            }
            break;
        case Z_BUF_ERROR:
            if (zstream->avail_out == 0) {
                ret.status = CompressionStatus::OutputBufferFull;
            } else if (zstream->avail_in == 0) {
                ret.status = CompressionStatus::NeedsInput;
            }
            break;
        case Z_STREAM_ERROR:
            ret.status = CompressionStatus::NonFatalError;
            break;
    };

    return ret;
}
CompressionResult ZLIBCompressor::compress_buffer(BufferView in,
                                                  BufferView out) {
    size_t total_compressed = 0;
    size_t total_consumed = 0;

    CompressionResult r{};
    r.status = CompressionStatus::NeedsFlush;

    while (r.status != CompressionStatus::OutputBufferFull &&
           r.status != CompressionStatus::NeedsInput) {
        r = compress_partial(in, out);

        total_compressed += r.compressed;
        total_consumed += r.input_consumed;
        in += r.input_consumed;
        out += r.compressed;
    }

    r.compressed = total_compressed;
    r.input_consumed = total_consumed;

    return r;
}
CompressionResult ZLIBCompressor::finish(BufferView in, BufferView out) {
    if (state_ == State::Finished) {
        return CompressionResult{
            .compressed = 0,
            .input_consumed = 0,
            .status = CompressionStatus::InvalidState
        };
    } else {
        state_ = State::Finishing;
    }
    auto* zstream = static_cast<z_streamp>(zstream_);
    set_zstream(zstream, in, out);

    CompressionResult ret{};
    int zout = Z_OK;
    while (zout != Z_STREAM_END) {
        zout = deflate(zstream, Z_FINISH);
        if (zout == Z_STREAM_ERROR) {
            ret.status = CompressionStatus::FatalError;
            return ret;
        }
        if (zstream->avail_out == 0 && zout == Z_BUF_ERROR) {
            ret.status = CompressionStatus::OutputBufferFull;
            return ret;
        }
    };

    ret.compressed = zstream->next_out - out.uc();
    ret.input_consumed = 0;
    ret.status = CompressionStatus::InputBufferFinished;

    return ret;
}
};  // namespace csics::io::compression
