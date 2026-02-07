#include <csics/io/encdec/Base64.hpp>
#include <cstring>

namespace csics::io::encdec {
constexpr uint8_t base64_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
Base64Encoder::Base64Encoder() {
    holdover_[0] = 0;
    holdover_[1] = 0;
    holdover_[2] = 0;
}

static inline void get_4_chars(uint8_t* in, uint8_t* out) {
    out[0] = in[0] >> 2;
    out[1] = ((in[0] & 0b00000011) << 4) | ((in[1] & 0b11110000) >> 4);
    out[2] = ((in[1] & 0b00001111) << 2) | ((in[2] & 0b11000000) >> 6);
    out[3] = in[2] & 0b00111111;
    (out)[0] = base64_table[(out)[0]];
    (out)[1] = base64_table[(out)[1]];
    (out)[2] = base64_table[(out)[2]];
    (out)[3] = base64_table[(out)[3]];
}

static inline EncodingResult encode_b64_no_holdover(BufferView in,
                                                    BufferView out,
                                                    uint8_t holdover[3]) {
    uint8_t input[3];
    auto* in_ptr = in.data();
    auto* out_ptr = out.data();
    while (in.size() >= 3) {
        if (out.size() < 4) {
            EncodingResult result{};
            result.processed = in.size();
            result.output = out.size();
            result.status = EncodingStatus::OutputBufferFull;
            return result;
        }

        input[0] = in.u8()[0];
        input[1] = in.u8()[1];
        input[2] = in.u8()[2];
        get_4_chars(input, out.u8());
        out += 4;
        in += 3;
    }

    auto leftover = in.size() % 3;
    holdover[0] = leftover;
    for (size_t i = 0; i < leftover; i++) {
        holdover[i + 1] = in.data()[i];
    }
    in += leftover;

    EncodingResult result{};
    result.processed = in.data() - in_ptr;
    result.output = out.data() - out_ptr;
    result.status = EncodingStatus::Ok;
    return result;
}

static inline EncodingResult encode_64_1_holdover(BufferView in, BufferView out,
                                                  uint8_t holdover[3]) {
    if (in.size() < 2) {
        EncodingResult result{};
        result.processed = 1;
        result.output = 0;
        result.status = EncodingStatus::Ok;
        holdover[0] = 2;
        holdover[2] = in.data()[0];
        return result;
    }

    if (out.size() < 4) {
        EncodingResult result{};
        result.processed = 0;
        result.output = 0;
        result.status = EncodingStatus::OutputBufferFull;
        return result;
    }

    uint8_t input[3];
    input[0] = holdover[1];
    input[1] = in.u8()[0];
    input[2] = in.u8()[1];
    get_4_chars(input, out.u8());
    out += 4;
    in += 2;
    auto r = encode_b64_no_holdover(in, out, holdover);
    r.processed += 2;
    r.output += 4;
    return r;
}

static inline EncodingResult encode_64_2_holdover(BufferView in, BufferView out,
                                                  uint8_t holdover[3]) {
    if (in.size() < 1) {
        EncodingResult result{};
        result.processed = 0;
        result.output = 0;
        result.status = EncodingStatus::NeedsInput;
        return result;
    }

    if (out.size() < 4) {
        EncodingResult result{};
        result.processed = 0;
        result.output = 0;
        result.status = EncodingStatus::OutputBufferFull;
        return result;
    }

    uint8_t input[3];
    input[0] = (holdover)[1];
    input[1] = (holdover)[2];
    input[2] = in.u8()[0];
    get_4_chars(input, out.u8());
    out += 4;
    in += 1;
    auto r = encode_b64_no_holdover(in, out, holdover);
    r.processed += 1;
    r.output += 4;
    return r;
}

EncodingResult Base64Encoder::encode(BufferView in, BufferView out) {
    EncodingResult result{};
    if (in.empty()) {
        result.processed = 0;
        result.output = 0;
        result.status = EncodingStatus::Ok;
        return result;
    }

    switch (holdover_[0]) {
        case 0:
            result = encode_b64_no_holdover(in, out, holdover_);
            break;
        case 1:
            result = encode_64_1_holdover(in, out, holdover_);
            break;
        case 2:
            result = encode_64_2_holdover(in, out, holdover_);
            break;
    }

    return result;
}

EncodingResult Base64Encoder::finish(BufferView in, BufferView out) {
    auto r = encode(in, out);
    if (r.status == EncodingStatus::OutputBufferFull) {
        return r;
    }
    out += r.output;
    // Handle padding
    switch (holdover_[0]) {
        case 1: {
            if (out.size() < 3) {
                r.status = EncodingStatus::OutputBufferFull;
                return r;
            }
            out.data()[0] = base64_table[holdover_[1] >> 2];
            out.data()[1] = base64_table[(holdover_[1] & 0b00000011) << 4];
            out.data()[2] = '=';
            out.data()[3] = '=';
            break;
        }
        case 2: {
            if (out.size() < 4) {
                r.status = EncodingStatus::OutputBufferFull;
                return r;
            }
            out.data()[0] = base64_table[holdover_[1] >> 2];
            out.data()[1] = base64_table[((holdover_[1] & 0b00000011) << 4) | ((holdover_[2] & 0b11110000) >> 4)];
            out.data()[2] = base64_table[(holdover_[2] & 0b00001111) << 2];
            out.data()[3] = '=';
            break;
        }
    }

    r.output += holdover_[0] != 0 ? 4 : 0;
    r.status = EncodingStatus::Ok;
    std::memset(holdover_, 0, sizeof(holdover_));
    return r;
}
};  // namespace csics::io::encdec
