#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>
namespace csics {

template <typename T>
concept BufferType = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

class BufferView {
   public:
    using value_type = char;
    using iterator = char*;
    using const_iterator = const char*;

    const char* data() const noexcept { return buf_; }
     std::size_t size() const noexcept { return size_; }
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
    }

    BufferView& operator++() noexcept {
        if (!empty()) {
            ++buf_;
            --size_;
        }
        return *this;
    }

    BufferView operator++(int) noexcept {
        BufferView temp = *this;
        ++(*this);
        return temp;
    }

    char& operator[](std::size_t index) noexcept { return buf_[index]; }
    const char& operator[](std::size_t index) const noexcept {
        return buf_[index];
    }

    BufferView operator()(std::size_t offset,
                          std::size_t length) const noexcept {
        return subview(offset, length);
    }

    const char* begin() const noexcept { return buf_; }
    const char* end() const noexcept { return buf_ + size_; }
    char* begin() noexcept { return buf_; }
    char* end() noexcept { return buf_ + size_; }

    const char* cbegin() const noexcept { return buf_; }
    const char* cend() const noexcept { return buf_ + size_; }

    explicit BufferView(void* buf, std::size_t size)
        : buf_(reinterpret_cast<char*>(buf)), size_(size) {}
    BufferView() : buf_(nullptr), size_(0) {}

    BufferView(const BufferView& other) noexcept
        : buf_(other.buf_), size_(other.size_) {}
    BufferView& operator=(const BufferView& other) noexcept {
        if (this != &other) {
            buf_ = other.buf_;
            size_ = other.size_;
        }
        return *this;
    }

    BufferView(BufferView&& other) noexcept
        : buf_(other.buf_), size_(other.size_) {
        other.buf_ = nullptr;
        other.size_ = 0;
    }
    BufferView& operator=(BufferView&& other) noexcept {
        if (this != &other) {
            buf_ = other.buf_;
            size_ = other.size_;
            other.buf_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    template<typename T>
        requires BufferType<T>
    BufferView(std::vector<T>& vec) noexcept {
        if (vec.empty()) {
            buf_ = nullptr;
            size_ = 0;
        } else {
            buf_ = reinterpret_cast<char*>(vec.data());
            size_ = vec.size() * sizeof(T);
        }
    }

   private:
    char* buf_;
    std::size_t size_;
};

template <BufferType T = char>
class TypedView {
   public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

    static TypedView from_bytes(const void* data, std::size_t size) noexcept {
        if (data == nullptr || size == 0 || size % sizeof(T) != 0) {
            return TypedView(nullptr, 0);
        }
        return TypedView(reinterpret_cast<T*>(const_cast<void*>(data)), size / sizeof(T));
    }

    template <typename U>
    TypedView<U> as() const noexcept {
        if (size_ % sizeof(U) != 0) {
            return TypedView<U>();
        }
        return TypedView<U>(reinterpret_cast<U*>(buf_), size_ * sizeof(T) / sizeof(U));
    }

    template <typename U>
    operator std::span<U>() const noexcept {
        return std::span<U>(reinterpret_cast<U*>(buf_), size_ * sizeof(T) / sizeof(U));
    }

    const T* data() const noexcept { return buf_; }
    std::size_t size() const noexcept { return size_; }
    T* data() noexcept { return buf_; }

    const uint8_t* u8() const noexcept {
        return reinterpret_cast<const uint8_t*>(buf_);
    }
    uint8_t* u8() noexcept { return reinterpret_cast<uint8_t*>(buf_); }

    const unsigned char* uc() const noexcept {
        return reinterpret_cast<const unsigned char*>(buf_);
    }
    unsigned char* uc() noexcept {
        return reinterpret_cast<unsigned char*>(buf_);
    }

    bool empty() const noexcept { return size_ == 0; }

    TypedView subview(std::size_t offset, std::size_t length) const noexcept {
        if (offset >= size_) {
            return TypedView(nullptr, 0);
        }
        if (offset + length > size_) {
            length = size_ - offset;
        }
        return TypedView(buf_ + offset, length);
    }

    TypedView head(std::size_t length) const noexcept {
        if (length > size_) {
            length = size_;
        }
        return TypedView(buf_, length);
    }

    TypedView tail(std::size_t length) const noexcept {
        if (length > size_) {
            length = size_;
        }
        return TypedView(buf_ + (size_ - length), length);
    }

    bool invalid() const noexcept { return buf_ == nullptr; }

    operator bool() const noexcept { return buf_ != nullptr && size_ > 0; }

    bool operator==(const TypedView& other) const noexcept {
        return buf_ == other.buf_ && size_ == other.size_;
    }

    bool operator!=(const TypedView& other) const noexcept {
        return !(*this == other);
    }

    TypedView operator+(std::size_t offset) const noexcept {
        if (offset > size_) {
            return TypedView(nullptr, 0);
        }
        return TypedView(buf_ + offset, size_ - offset);
    }

    TypedView& operator+=(std::size_t offset) noexcept {
        if (offset > size_) {
            buf_ = nullptr;
            size_ = 0;
        } else {
            buf_ += offset;
            size_ -= offset;
        }
        return *this;
    }

    T& operator[](std::size_t index) noexcept { return buf_[index]; }
    const T& operator[](std::size_t index) const noexcept {
        return buf_[index];
    }

    TypedView operator()(std::size_t offset,
                          std::size_t length) const noexcept {
        return subview(offset, length);
    }

    iterator begin() const noexcept { return buf_; }
    iterator end() const noexcept { return buf_ + size_; }

    const_iterator cbegin() const noexcept { return buf_; }
    const_iterator cend() const noexcept { return buf_ + size_; }

    constexpr TypedView() : buf_(nullptr), size_(0) {}
    template <BufferType U>
    constexpr TypedView(std::vector<U>& vec) noexcept {
        if (vec.empty() || sizeof(U) != sizeof(T)) {
            buf_ = nullptr;
            size_ = 0;
        } else {
            buf_ = reinterpret_cast<T*>(vec.data());
            size_ = vec.size() * sizeof(U);
        }
    }

    constexpr TypedView(T* data, std::size_t size) noexcept
        : buf_(data), size_(size) {}

    constexpr TypedView(const TypedView& other) noexcept
        : buf_(other.buf_), size_(other.size_) {}

    constexpr TypedView& operator=(const TypedView& other) noexcept {
        if (this != &other) {
            buf_ = other.buf_;
            size_ = other.size_;
        }
        return *this;
    }

    constexpr TypedView(TypedView&& other) noexcept
        : buf_(other.buf_), size_(other.size_) {
        other.buf_ = nullptr;
        other.size_ = 0;
    }

    constexpr TypedView& operator=(TypedView&& other) noexcept {
        if (this != &other) {
            buf_ = other.buf_;
            size_ = other.size_;
            other.buf_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

   private:
    T* buf_;
    std::size_t size_;
};

enum class CapacityPolicy {
    Exact,       // Allocate exactly the requested size
    PowerOfTwo,  // Round up to the next power of two
};

template <BufferType T = char, size_t Alignment = alignof(T),
          CapacityPolicy Policy = CapacityPolicy::Exact>
class Buffer {
   public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

    constexpr Buffer() : buf_(nullptr), size_(0) {}
    Buffer(std::size_t size)
        : capacity_(adjust_capacity(size)),
          size_(size),
          buf_(::operator new(capacity_, std::align_val_t{Alignment})) {}

    ~Buffer() { operator delete(buf_, std::align_val_t{Alignment}); }

    Buffer(const Buffer& other)
        : capacity_(adjust_capacity(other.size_)), size_(other.size_) {
        buf_ =
            ::operator new(capacity_ * sizeof(T), std::align_val_t{Alignment});
        std::memcpy(buf_, other.buf_, size_ * sizeof(T));
    }

    Buffer(Buffer&& other) noexcept
        : capacity_(other.capacity_), size_(other.size_), buf_(other.buf_) {
        other.buf_ = nullptr;
        other.size_ = 0;
    }

    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            operator delete(buf_, std::align_val_t{Alignment});
            size_ = other.size_;
            buf_ =
                ::operator new(size_ * sizeof(T), std::align_val_t{Alignment});
            std::memcpy(buf_, other.buf_, size_ * sizeof(T));
        }
        return *this;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            operator delete(buf_, std::align_val_t{Alignment});
            buf_ = other.buf_;
            size_ = other.size_;
            other.buf_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    T* data() noexcept { return buf_; }
    const T* data() const noexcept { return buf_; }

    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }

    constexpr std::size_t alignment() const noexcept { return Alignment; }

    BufferView view() const noexcept { return BufferView(buf_, size_); }

    BufferView subview(std::size_t offset,
                          std::size_t length) const noexcept {
        if (offset >= size_) {
            return BufferView();
        }
        if (offset + length > size_) {
            length = size_ - offset;
        }
        return BufferView(buf_ + offset, length);
    }

    void resize(std::size_t new_size) {
        if (new_size > capacity_) {
            std::size_t new_capacity = adjust_capacity(new_size);
            void* new_buf = ::operator new(new_capacity * sizeof(T),
                                           std::align_val_t{Alignment});
            std::memcpy(new_buf, buf_, size_ * sizeof(T));
            operator delete(buf_, std::align_val_t{Alignment});
            buf_ = static_cast<T*>(new_buf);
            capacity_ = new_capacity;
        }
        size_ = new_size;
    }

    operator BufferView() noexcept {
        return BufferView(buf_, size_ * sizeof(T));
    }

    operator BufferView() const noexcept {
        return BufferView(buf_, size_ * sizeof(T));
    }

    T& operator[](std::size_t index) noexcept { return buf_[index]; }
    const T& operator[](std::size_t index) const noexcept {
        return buf_[index];
    }

    BufferView operator()(std::size_t offset,
                             std::size_t length) const noexcept {
        return subview(offset, length);
    }

    T* begin() noexcept { return buf_; }
    T* end() noexcept { return buf_ + size_; }
    const T* cbegin() const noexcept { return buf_; }
    const T* cend() const noexcept { return buf_ + size_; }

   private:
    std::size_t capacity_;
    std::size_t size_;
    T* buf_;

    static constexpr std::size_t adjust_capacity(std::size_t requested_size) {
        if constexpr (Policy == CapacityPolicy::Exact) {
            return requested_size;
        } else if constexpr (Policy == CapacityPolicy::PowerOfTwo) {
            return std::bit_ceil(requested_size);
        } else {
            static_assert(Policy == CapacityPolicy::Exact ||
                              Policy == CapacityPolicy::PowerOfTwo,
                          "Unsupported CapacityPolicy");
        }
    }
};

};  // namespace csics::io
