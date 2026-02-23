
#pragma once
#include "csics/Buffer.hpp"
#include "csics/io/encdec/EncodeDecode.hpp"
namespace csics::io::encdec {

class Base64Encoder {
   public:
    Base64Encoder();
    EncodingResult encode(BufferView in, MutableBufferView out);
    EncodingResult finish(BufferView in, MutableBufferView out);

   private:
    uint8_t holdover_[3]; // 0: number of bytes held over, 1: first byte, 2: second byte
};
};  // namespace csics::io::encdec
