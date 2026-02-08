#include "ZSTDCompressor.hpp"
#include <zstd.h>

namespace csics::io::compression {
ZSTDCompressor::ZSTDCompressor() : stream_(nullptr) {
    stream_ = ZSTD_createCStream();
    if (stream_ == nullptr) {
        throw std::runtime_error("Failed to create ZSTD compressor stream");
    }
    std::size_t ret = ZSTD_initCStream(static_cast<ZSTD_CStream*>(stream_), 3);
    if (ZSTD_isError(ret)) {
        ZSTD_freeCStream(static_cast<ZSTD_CStream*>(stream_));
        throw std::runtime_error("Failed to initialize ZSTD compressor stream");
    }
}

ZSTDCompressor::~ZSTDCompressor() {
    if (stream_ != nullptr) {
        ZSTD_freeCStream(static_cast<ZSTD_CStream*>(stream_));
        stream_ = nullptr;
    }
}

CompressionResult ZSTDCompressor::compress_partial(BufferView<> in,
                                                   BufferView<> out) {
    ZSTD_CStream* stream = static_cast<ZSTD_CStream*>(stream_);
    ZSTD_outBuffer o_buf{};
    o_buf.dst = out.data();
    o_buf.pos = 0;
    o_buf.size = out.size();

    ZSTD_inBuffer i_buf{};
    i_buf.src = in.data();
    i_buf.pos = 0;
    i_buf.size = in.size();

    size_t bytes =
        ZSTD_compressStream2(stream, &o_buf, &i_buf, ZSTD_e_continue);

    CompressionResult r{};
    r.compressed = o_buf.pos;
    r.input_consumed = i_buf.pos;

    if (ZSTD_isError(bytes)) {
        ZSTD_CCtx_reset(stream, ZSTD_reset_session_only);
        r.status = CompressionStatus::NonFatalError;
    } else if (out.size() == o_buf.pos) {
        r.status = CompressionStatus::OutputBufferFull;
    } else if (bytes != 0) {
        r.status = CompressionStatus::NeedsFlush;
    } else {
        r.status = i_buf.pos == i_buf.size
                       ? CompressionStatus::InputBufferFinished
                       : CompressionStatus::NeedsInput;
    }

    return r;
}

CompressionResult ZSTDCompressor::compress_buffer(BufferView<> in,
                                                  BufferView<> out) {
    CompressionStatus status = CompressionStatus::NeedsInput;
    std::size_t total_input_consumed = 0;
    std::size_t total_compressed = 0;
    do {
        CompressionResult r = compress_partial(in, out);
        while (r.status == CompressionStatus::NonFatalError) {
            r = compress_partial(in, out);
        }
        in += r.input_consumed;
        out += r.compressed;
        total_input_consumed += r.input_consumed;
        total_compressed += r.compressed;
        status = r.status;

        if (status == CompressionStatus::OutputBufferFull) {
            return CompressionResult{
                .compressed = total_compressed,
                .input_consumed = total_input_consumed,
                .status = CompressionStatus::OutputBufferFull
            };
        }

        if (status == CompressionStatus::FatalError) {
            return CompressionResult{
                .compressed = total_compressed,
                .input_consumed = total_input_consumed,
                .status = CompressionStatus::FatalError
            };
        }
    } while (status == CompressionStatus::NeedsFlush);

    return CompressionResult{
        .compressed = total_compressed,
        .input_consumed = total_input_consumed,
        .status = status
    };
};

CompressionResult ZSTDCompressor::finish(BufferView<> in, BufferView<> out) {
    ZSTD_CStream* stream = static_cast<ZSTD_CStream*>(stream_);
    ZSTD_inBuffer i_buf{};
    i_buf.src = in.data();
    i_buf.pos = 0;
    i_buf.size = in.size();
    std::size_t compressed_total = 0;
    std::size_t bytes = 0;

    do {
        ZSTD_outBuffer o_buf{};
        o_buf.dst = out.data();
        o_buf.pos = 0;
        o_buf.size = out.size();

        bytes = ZSTD_compressStream2(stream, &o_buf, &i_buf, ZSTD_e_end);

        if (ZSTD_isError(bytes)) {
            ZSTD_CCtx_reset(stream, ZSTD_reset_session_only);
            CompressionResult r{};
            r.compressed = compressed_total;
            r.input_consumed = 0;
            r.status = CompressionStatus::NonFatalError;
            return r;
        }

        compressed_total += o_buf.pos;
        out += o_buf.pos;

    } while (bytes != 0 && out.size() > 0);

    CompressionResult r{};
    r.compressed = compressed_total;
    r.input_consumed = 0;

    if (bytes != 0) {
        r.status = CompressionStatus::NeedsFlush;
    } else if (out.size() == 0) {
        r.status = CompressionStatus::OutputBufferFull;
    } else {
        r.status = CompressionStatus::InputBufferFinished;
    }

    return r;
}

};  // namespace csics::io::compression
