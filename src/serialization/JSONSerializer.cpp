#include <csics/serialization/JSONSerializer.hpp>

namespace csics::serialization {
    enum class JSONState {Default, First, Array};
    struct JSONSerializer::Impl {
        JSONState state = JSONState::Default;
    };
    JSONSerializer::JSONSerializer():
        impl_(std::make_unique<Impl>()) {};
    JSONSerializer::~JSONSerializer() = default;

static void escape_char(char c, BufferView& bv) {
    if (c == '\\' || c == '"') {
        bv[0] = '\\';
        bv[1] = c;
        bv += 2;
    } else if (c < 0x20) {
        bv[0] = '\\';
        switch (c) {
            case '\b': bv[1] = 'b'; break;
            case '\f': bv[1] = 'f'; break;
            case '\n': bv[1] = 'n'; break;
            case '\r': bv[1] = 'r'; break;
            case '\t': bv[1] = 't'; break;
            default:
                // For other control characters, use \u00XX format
                bv[1] = 'u';
                bv[2] = '0';
                bv[3] = '0';
                const char hex[] = "0123456789abcdef";
                bv[4] = hex[(c >> 4) & 0xF];
                bv[5] = hex[c & 0xF];
                bv += 6;
                return;
        }
        bv += 2;
    } else {
        bv[0] = c;
        bv++;
    }
};

constexpr const char* true_str = "true";
constexpr const char* false_str = "false";
constexpr const char* null_str = "null";

SerializationStatus JSONSerializer::key(BufferView& bv, std::string_view key) {
    if (impl_->state == JSONState::Default) [[likely]] {
        bv[0] = ',';
        bv += 1;
    } else {
        impl_->state = JSONState::Default;
    }
    bv[0] = '"';
    bv += 1;
    for (char c : key) {
        escape_char(c, bv);
    }
    bv[0] = '"';
    bv[1] = ':';
    bv += 2;
    return SerializationStatus::Ok;
}
SerializationStatus JSONSerializer::begin_obj(BufferView& bv) {
    *bv.data() = '{';
    bv += 1;
    impl_->state = JSONState::First;
    return SerializationStatus::Ok;
}
SerializationStatus JSONSerializer::end_obj(BufferView& bv) {
    *bv.data() = '}';
    bv += 1;
    impl_->state = JSONState::Default;
    return SerializationStatus::Ok;
}
SerializationStatus JSONSerializer::write_number(BufferView& bv, double num) {
    // This is a very naive implementation and should be replaced with a proper
    // number to string conversion that handles edge cases and is efficient.
    
    int len = std::snprintf(bv.data(), bv.size(), "%g", num);
    if (len < 0 || static_cast<std::size_t>(len) >= bv.size()) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    bv += len;
    if (impl_->state == JSONState::Array) [[unlikely]] {
        bv[0] = ',';
        bv += 1;
    }
    return SerializationStatus::Ok;
}

SerializationStatus JSONSerializer::write_bool(BufferView& bv, bool value) {
    const char* str = value ? true_str : false_str;
    std::size_t len = value ? 4 : 5;
    if (len >= bv.size()) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    std::memcpy(bv.data(), str, len);
    bv += len;
    if (impl_->state == JSONState::Array) [[unlikely]] {
        bv[0] = ',';
        bv += 1;
    }
    return SerializationStatus::Ok;
}

SerializationStatus JSONSerializer::write_string(BufferView& bv, std::string_view str) {
    if (bv.size() < 2) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    *bv.data() = '"';
    bv += 1;
    for (char c : str) {
        escape_char(c, bv);
    }
    *bv.data() = '"';
    bv += 1;
    if (impl_->state == JSONState::Array) [[unlikely]] {
        bv[0] = ',';
        bv += 1;
    }
    return SerializationStatus::Ok;
}
SerializationStatus JSONSerializer::begin_array(BufferView& bv) {
    *bv.data() = '[';
    bv += 1;
    impl_->state = JSONState::Array;
    return SerializationStatus::Ok;
}
SerializationStatus JSONSerializer::end_array(BufferView& bv) {
    bv[-1] = ']';  // Replace last comma with closing bracket
    impl_->state = JSONState::Default;
    return SerializationStatus::Ok;
}

SerializationStatus JSONSerializer::write_int(BufferView& bv, std::int64_t num) {
    // This is a very naive implementation and should be replaced with a proper
    // number to string conversion that handles edge cases and is efficient.
    
    auto len = std::snprintf(bv.data(), bv.size(), "%ld", num);
    if (len < 0 || static_cast<std::size_t>(len) >= bv.size()) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    bv += len;
    if (impl_->state == JSONState::Array) [[unlikely]] {
        bv[0] = ',';
        bv += 1;
    }
    return SerializationStatus::Ok;
}

SerializationStatus JSONSerializer::write_null(BufferView& bv) {
    if (4 >= bv.size()) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    std::memcpy(bv.data(), null_str, 4);
    bv += 4;
    if (impl_->state == JSONState::Array) [[unlikely]] {
        bv[0] = ',';
        bv += 1;
    }
    return SerializationStatus::Ok;
}
};
