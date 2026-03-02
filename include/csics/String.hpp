#pragma once

#include <ostream>
#include "csics/Buffer.hpp"
#include "csics/assert.hpp"

namespace csics {
class String;

class StringView {
   public:
    using value_type = char;
    using iterator = const char*;
    using const_iterator = const char*;

    const char* data() const noexcept { return buf_; }
    std::size_t size() const noexcept { return size_; }

    bool empty() const noexcept { return size_ == 0; }

    template <std::size_t N>
    constexpr StringView(const char (&str)[N]) : buf_(str), size_(N - 1) {}

    StringView(const char* str) : buf_(str), size_(std::strlen(str)) {}
    StringView(char* str) : buf_(str), size_(std::strlen(str)) {}
    StringView(const char* str, std::size_t size) : buf_(str), size_(size) {}
    StringView() : buf_(nullptr), size_(0) {}
    StringView(const StringView& other)
        : buf_(other.buf_), size_(other.size_) {}
    StringView& operator=(const StringView& other) {
        if (this != &other) {
            buf_ = other.buf_;
            size_ = other.size_;
        }
        return *this;
    }

    StringView(StringView&& other) noexcept
        : buf_(other.buf_), size_(other.size_) {
        other.buf_ = nullptr;
        other.size_ = 0;
    }

    StringView& operator=(StringView&& other) noexcept {
        if (this != &other) {
            buf_ = other.buf_;
            size_ = other.size_;
            other.buf_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    StringView(const std::string& str) : buf_(str.data()), size_(str.size()) {}
    StringView(std::string&& str) : buf_(str.data()), size_(str.size()) {}

    operator std::string() const { return std::string(buf_, size_); }

    StringView subview(std::size_t offset, std::size_t length) const noexcept {
        if (offset >= size_) {
            return StringView(nullptr, 0);
        }
        if (offset + length > size_) {
            length = size_ - offset;
        }
        return StringView(buf_ + offset, length);
    }

    StringView head(std::size_t length) const noexcept {
        if (length > size_) {
            length = size_;
        }
        return StringView(buf_, length);
    }

    StringView tail(std::size_t length) const noexcept {
        if (length > size_) {
            length = size_;
        }
        return StringView(buf_ + (size_ - length), length);
    }

    String str() const;

    operator bool() const noexcept { return buf_ != nullptr && size_ > 0; }

    bool operator!=(const StringView& other) const noexcept {
        return !(*this == other);
    }

    StringView operator+(std::size_t offset) const noexcept {
        if (offset > size_) {
            return StringView(nullptr, 0);
        }
        return StringView(buf_ + offset, size_ - offset);
    }

    StringView& operator+=(std::size_t offset) noexcept {
        if (offset > size_) {
            buf_ = nullptr;
            size_ = 0;
        } else {
            buf_ += offset;
            size_ -= offset;
        }
        return *this;
    }

    char operator[](std::size_t index) const noexcept { return buf_[index]; }

    StringView operator()(std::size_t offset,
                          std::size_t length) const noexcept {
        return subview(offset, length);
    }

    friend std::ostream& operator<<(std::ostream& os, const StringView& v) {
        return os.write(v.data(), v.size());
    }

    const char* begin() const noexcept { return buf_; }
    const char* end() const noexcept { return buf_ + size_; }

   private:
    const char* buf_;
    std::size_t size_;
};

class String {
   public:
    String() : buf_(1) {}
    String(const StringView& v) : buf_() {
        buf_.resize(v.size() + 1);
        std::memcpy(buf_.data(), v.data(), v.size());
        buf_[v.size()] = '\0';
    }
    String(const String& other) : buf_(other.buf_) {}
    String& operator=(const String& other) {
        if (this != &other) {
            buf_ = other.buf_;
        }
        return *this;
    }

    template <std::size_t N>
    String(const char (&str)[N]) : String(StringView(str)) {}

    explicit String(const char* str) : String(StringView(str)) {}
    explicit String(char* str) : String(StringView(str)) {}
    explicit String(const char* str, std::size_t size)
        : String(StringView(str, size)) {}
    explicit String(char* str, std::size_t size)
        : String(StringView(str, size)) {}

    String(const std::string& str)
        : String(StringView(str.data(), str.size())) {}

    String(String&& other) noexcept : buf_(std::move(other.buf_)) {}
    String& operator=(String&& other) noexcept {
        if (this != &other) {
            buf_ = std::move(other.buf_);
        }
        return *this;
    }

    operator StringView() const noexcept {
        return StringView(buf_.data(), buf_.size()-1); // exclude null terminator
    }

    void append(const StringView& v) {
        if (!v || v.empty()) {
            return;
        }
        buf_.pop_back();  // remove \0
        buf_.append(v.data(), v.size());
        buf_.push_back('\0');
    }

    std::size_t size() const noexcept { return buf_.size(); }

    const char* c_str() const noexcept { return buf_.data(); }
    const char* data() const noexcept { return buf_.data(); }

    char operator[](std::size_t index) const noexcept { return buf_[index]; }

    String operator()(std::size_t offset, std::size_t length) const noexcept {
        return String(StringView(buf_.data() + offset, length));
    }

    bool empty() const noexcept { return buf_.size() <= 1; }  // only null terminator

    const char* begin() const noexcept { return buf_.data(); }
    const char* end() const noexcept { return buf_.data() + buf_.size(); }

    bool operator!=(const String& other) const noexcept {
        return !(*this == other);
    }

    operator bool() const noexcept { return buf_; }

    friend std::ostream& operator<<(std::ostream& os, const String& s) {
        return os.write(s.data(), s.size());
    }

   private:
    Buffer<char> buf_;
};

inline bool operator==(const StringView& a, const StringView& o) noexcept {
    return a.size() == o.size() &&
           std::memcmp(a.data(), o.data(), a.size()) == 0;
}

inline String StringView::str() const { return String(*this); }
};
