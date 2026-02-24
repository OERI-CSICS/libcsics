
#pragma once

#include "csics/Buffer.hpp"
namespace csics::serialization {
enum class SerializationStatus {
    Ok,
    BufferFull,
    NonFatalError,
    Failed,
};

enum class DeserializationStatus {
    Ok,
    BufferEmpty,
    InvalidData,
};

struct SerializationResult {
    MutableBufferView written_view;
    SerializationStatus status;

    constexpr SerializationResult(MutableBufferView written_view,
                                  SerializationStatus status)
        : written_view(written_view), status(status) {}
};

};
