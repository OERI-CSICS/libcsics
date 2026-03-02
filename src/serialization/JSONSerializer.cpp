#include <charconv>
#include <csics/serialization/JSONSerializer.hpp>
#include <stack>

namespace csics::serialization {

// this implementation sucks
// TODO: replace with a more efficient version

enum class JSONState { Array, Object };
struct JSONSerializer::Impl {
    std::stack<JSONState> state_stack;
};
JSONSerializer::JSONSerializer() : impl_(std::make_unique<Impl>()) {};
JSONSerializer::~JSONSerializer() = default;

static void escape_char(const char c, MutableBufferView& bv) {
    if (c == '\\' || c == '"') {
        bv[0] = '\\';
        bv[1] = c;
        bv += 2;
    } else if (c < 0x20) {
        bv[0] = '\\';
        switch (c) {
            case '\b':
                bv[1] = 'b';
                break;
            case '\f':
                bv[1] = 'f';
                break;
            case '\n':
                bv[1] = 'n';
                break;
            case '\r':
                bv[1] = 'r';
                break;
            case '\t':
                bv[1] = 't';
                break;
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

SerializationStatus JSONSerializer::key(MutableBufferView& bv,
                                        std::string_view key) {
    if (*(bv.c() - 1) == '}' || *(bv.c() - 1) == ']') [[unlikely]] {
        bv[0] = ',';
        bv += 1;
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
SerializationStatus JSONSerializer::begin_obj(MutableBufferView& bv) {
    *bv.c() = '{';
    bv += 1;
    impl_->state_stack.push(JSONState::Object);
    return SerializationStatus::Ok;
}
SerializationStatus JSONSerializer::end_obj(MutableBufferView& bv) {
    impl_->state_stack.pop();
    if (*(bv.c() - 1) != '{') { // not empty object, trailing comma is present
        bv = MutableBufferView(bv.c() - 1, bv.size() + 1);
    }
    bv[0] = '}';
    bv += 1;
    if (!impl_->state_stack.empty()) {
        bv[0] = ',';
        bv += 1;
    }
    return SerializationStatus::Ok;
}
SerializationStatus JSONSerializer::write_number(MutableBufferView& bv,
                                                 double num) {
    // This is a very naive implementation and should be replaced with a proper
    // number to string conversion that handles edge cases and is efficient.

    auto [ptr, ec] = std::to_chars(bv.c(), bv.c() + bv.size(), num, std::chars_format::general);

    if (ec != std::errc{}) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    bv += ptr - bv.c();
    bv[0] = ',';
    bv += 1;
    return SerializationStatus::Ok;
}

SerializationStatus JSONSerializer::write_bool(MutableBufferView& bv,
                                               bool value) {
    const char* str = value ? true_str : false_str;
    std::size_t len = value ? 4 : 5;
    if (len >= bv.size()) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    std::memcpy(bv.c(), str, len);
    bv += len;
    bv[0] = ',';
    bv += 1;
    return SerializationStatus::Ok;
}

SerializationStatus JSONSerializer::write_string(MutableBufferView& bv,
                                                 std::string_view str) {
    if (bv.size() < 2) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    *bv.c() = '"';
    bv += 1;
    for (char c : str) {
        escape_char(c, bv);
    }
    *bv.c() = '"';
    bv += 1;
    bv[0] = ',';
    bv += 1;
    return SerializationStatus::Ok;
}
SerializationStatus JSONSerializer::begin_array(MutableBufferView& bv) {
    *bv.c() = '[';
    bv += 1;
    impl_->state_stack.push(JSONState::Array);
    return SerializationStatus::Ok;
}
SerializationStatus JSONSerializer::end_array(MutableBufferView& bv) {
    impl_->state_stack.pop();
    if (*(bv.c() - 1) != '[') { // not empty array, trailing comma is present
        bv = MutableBufferView(bv.c() - 1, bv.size() + 1);
    }
    bv[0] = ']';
    bv += 1;
    if (!impl_->state_stack.empty()) {
        bv[0] = ',';
        bv += 1;
    }
    return SerializationStatus::Ok;
}

SerializationStatus JSONSerializer::write_int(MutableBufferView& bv,
                                              std::int64_t num) {
    // This is a very naive implementation and should be replaced with a proper
    // number to string conversion that handles edge cases and is efficient.

    auto len = std::snprintf(bv.c(), bv.size(), "%ld", num);
    if (len < 0 || static_cast<std::size_t>(len) >= bv.size()) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    bv += len;
    bv[0] = ',';
    bv += 1;
    return SerializationStatus::Ok;
}

SerializationStatus JSONSerializer::write_null(MutableBufferView& bv) {
    if (4 >= bv.size()) {
        return SerializationStatus::BufferFull;  // Handle error appropriately
    }
    std::memcpy(bv.c(), null_str, 4);
    bv += 4;
    bv[0] = ',';
    bv += 1;
    return SerializationStatus::Ok;
}
};  // namespace csics::serialization
