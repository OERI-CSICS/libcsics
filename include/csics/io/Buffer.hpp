#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
namespace csics::io {
class BufferView {
   public:
    using value_type = char;
    using iterator = char*;
    using const_iterator = const char*;

    explicit BufferView(void* buf, std::size_t size)
        : buf_(reinterpret_cast<char*>(buf)), size_(size) {};
    BufferView() : buf_(nullptr), size_(0) {};

    template <typename T>
    operator std::span<T>() const noexcept {
        return std::span<T>(reinterpret_cast<T*>(buf_), size_ / sizeof(T));
    };

    const char* data() const noexcept { return buf_; }
    const std::size_t size() const noexcept { return size_; }
    char* data() noexcept { return buf_; }

    const uint8_t* u8() const noexcept { return reinterpret_cast<const uint8_t*>(buf_); }
    uint8_t* u8() noexcept { return reinterpret_cast<uint8_t*>(buf_); }

    const unsigned char* uc() const noexcept { return reinterpret_cast<const unsigned char*>(buf_); }
    unsigned char* uc() noexcept { return reinterpret_cast<unsigned char*>(buf_); }

    bool empty() const noexcept { return size_ == 0; }

    BufferView subview(std::size_t offset,
                          std::size_t length) const noexcept {
        if (offset >= size_) {
            return BufferView(nullptr, 0);
        }
        if (offset + length > size_) {
            length = size_ - offset;
        }
        return BufferView(buf_ + offset, length);
    }

    BufferView head(std::size_t length) const noexcept {
        if (length > size_) {
            length = size_;
        }
        return BufferView(buf_, length);
    }

    BufferView tail(std::size_t length) const noexcept {
        if (length > size_) {
            length = size_;
        }
        return BufferView(buf_ + (size_ - length), length);
    }

    operator bool() const noexcept { return buf_ != nullptr && size_ > 0; }

    bool operator==(const BufferView& other) const noexcept {
        return buf_ == other.buf_ && size_ == other.size_;
    }

    bool operator!=(const BufferView& other) const noexcept {
        return !(*this == other);
    }

    BufferView operator+(std::size_t offset) const noexcept {
        if (offset > size_) {
            return BufferView(nullptr, 0);
        }
        return BufferView(buf_ + offset, size_ - offset);
    }

    BufferView& operator+=(std::size_t offset) noexcept {
        if (offset > size_) {
            buf_ = nullptr;
            size_ = 0;
        } else {
            buf_ += offset;
            size_ -= offset;
        }
        return *this;
    };

    char& operator[](std::size_t index) noexcept { return buf_[index]; }

    template <typename T>
    T& as() noexcept {
        return *reinterpret_cast<T*>(buf_);
    }

    char* begin() const noexcept { return buf_; }
    char* end() const noexcept { return buf_ + size_; }

    const char* cbegin() const noexcept { return buf_; }
    const char* cend() const noexcept { return buf_ + size_; }

   private:
    char* buf_;
    std::size_t size_;
};
};  // namespace csics::io
