#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>
namespace csics {

template <typename T>
concept IsByteType = std::same_as<std::remove_cv_t<T>, uint8_t> ||
                     std::same_as<std::remove_cv_t<T>, unsigned char> ||
                     std::same_as<std::remove_cv_t<T>, char> ||
                     std::same_as<std::remove_cv_t<T>, std::byte>;

template <typename T, typename U>
concept ConstConvertible =
    std::is_const_v<T> && std::is_convertible_v<std::remove_const_t<T>, U>;

template <typename T, typename U>
concept SameConst = std::is_const_v<T> == std::is_const_v<U>;

template <typename T>
class BasicBufferView {
   public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

    const T* data() const noexcept { return buf_; }
    std::size_t size() const noexcept { return size_; }
    T* data() noexcept { return buf_; }

    auto* u8() noexcept {
        static_assert(IsByteType<T>, "u8() is only valid for byte types");
        if constexpr (std::is_const_v<T>) {
            return reinterpret_cast<const uint8_t*>(buf_);
        } else {
            return reinterpret_cast<uint8_t*>(buf_);
        }
    }

    auto* uc() noexcept {
        static_assert(IsByteType<T>, "uc() is only valid for byte types");
        if constexpr (std::is_const_v<T>) {
            return reinterpret_cast<const unsigned char*>(buf_);
        } else {
            return reinterpret_cast<unsigned char*>(buf_);
        }
    }

    auto* c() noexcept {
        static_assert(IsByteType<T>, "uc() is only valid for byte types");
        if constexpr (std::is_const_v<T>) {
            return reinterpret_cast<const char*>(buf_);
        } else {
            return reinterpret_cast<char*>(buf_);
        }
    }

    bool empty() const noexcept { return size_ == 0; }

    BasicBufferView subview(std::size_t offset,
                            std::size_t length) const noexcept {
        if (offset >= size_) {
            return BasicBufferView();
        }
        if (offset + length > size_) {
            length = size_ - offset;
        }
        return BasicBufferView(buf_ + offset, length);
    }

    BasicBufferView head(std::size_t length) const noexcept {
        if (length > size_) {
            length = size_;
        }
        return BasicBufferView(buf_, length);
    }

    BasicBufferView tail(std::size_t length) const noexcept {
        if (length > size_) {
            length = size_;
        }
        return BasicBufferView(buf_ + (size_ - length), length);
    }

    operator bool() const noexcept { return buf_ != nullptr && size_ > 0; }

    bool operator==(const BasicBufferView& other) const noexcept {
        return buf_ == other.buf_ && size_ == other.size_;
    }

    bool operator!=(const BasicBufferView& other) const noexcept {
        return !(*this == other);
    }

    BasicBufferView operator+(std::size_t offset) const noexcept {
        if (offset > size_) {
            return BasicBufferView();
        }
        return BasicBufferView(buf_ + offset, size_ - offset);
    }

    BasicBufferView& operator+=(std::size_t offset) noexcept {
        if (offset > size_) {
            buf_ = nullptr;
            size_ = 0;
        } else {
            buf_ += offset;
            size_ -= offset;
        }
        return *this;
    }

    BasicBufferView& operator++() noexcept {
        if (!empty()) {
            ++buf_;
            --size_;
        }
        return *this;
    }

    BasicBufferView operator++(int) noexcept {
        BasicBufferView temp = *this;
        ++(*this);
        return temp;
    }

    T& operator[](std::size_t index) noexcept { return buf_[index]; }
    const T& operator[](std::size_t index) const noexcept {
        return buf_[index];
    }

    BasicBufferView operator()(std::size_t offset,
                               std::size_t length) const noexcept {
        return subview(offset, length);
    }

    const char* begin() const noexcept { return buf_; }
    const char* end() const noexcept { return buf_ + size_; }
    char* begin() noexcept { return buf_; }
    char* end() noexcept { return buf_ + size_; }

    const char* cbegin() const noexcept { return buf_; }
    const char* cend() const noexcept { return buf_ + size_; }

    constexpr BasicBufferView() : buf_(nullptr), size_(0) {}

    constexpr BasicBufferView(const BasicBufferView& other) noexcept
        : buf_(other.buf_), size_(other.size_) {}
    constexpr BasicBufferView& operator=(
        const BasicBufferView& other) noexcept {
        if (this != &other) {
            buf_ = other.buf_;
            size_ = other.size_;
        }
        return *this;
    }

    constexpr BasicBufferView(BasicBufferView&& other) noexcept
        : buf_(other.buf_), size_(other.size_) {
        other.buf_ = nullptr;
        other.size_ = 0;
    }
    constexpr BasicBufferView& operator=(BasicBufferView&& other) noexcept {
        if (this != &other) {
            buf_ = other.buf_;
            size_ = other.size_;
            other.buf_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    explicit constexpr BasicBufferView(T& obj) noexcept
        : buf_(reinterpret_cast<T*>(&obj)), size_(1) {}

    // implicit conversion from MutableBufferView to BufferView
    template <typename U = T>
        requires ConstConvertible<T, U>
    constexpr BasicBufferView(
        const BasicBufferView<std::remove_const_t<T>>& other) noexcept
        : buf_(other.data()), size_(other.size()) {}

    template <typename U>
        requires IsByteType<U> && (SameConst<T, U> || ConstConvertible<T, U>)
    constexpr BasicBufferView(U* data, std::size_t size) noexcept
        : buf_(reinterpret_cast<T*>(data)), size_(size) {}

    constexpr BasicBufferView(std::vector<T>& vec) noexcept
        : buf_(reinterpret_cast<T*>(vec.data())), size_(vec.size()) {}

    template <typename U>
        requires (SameConst<T, U> || ConstConvertible<T, U>)
        constexpr BasicBufferView(std::vector<U>& vec) noexcept
        : buf_(reinterpret_cast<T*>(vec.data())), size_(vec.size() * sizeof(U)) {}

    template <typename U>
        requires IsByteType<U> && IsByteType<T> && SameConst<T, U>
    constexpr BasicBufferView(BasicBufferView<U> byte_view) noexcept
        : BasicBufferView(BasicBufferView(
              reinterpret_cast<T*>(byte_view.data()), byte_view.size())) {}

    template <std::size_t N>
    constexpr BasicBufferView(const std::array<T, N>& arr) noexcept
        : buf_(const_cast<T*>(arr.data())), size_(N * sizeof(T)) {}

    template <std::size_t N>
    constexpr BasicBufferView(T (&arr)[N]) noexcept : buf_(arr), size_(N) {}

    template <typename U>
        requires IsByteType<U> && (SameConst<T, U> || ConstConvertible<U, T>)
    constexpr BasicBufferView<U> as_bytes() const noexcept {
        return BasicBufferView<U>(reinterpret_cast<U*>(buf_),
                                  size_ * sizeof(T));
    }

    template <typename U>
        requires IsByteType<T> && (SameConst<T, U> || ConstConvertible<U, T>)
    constexpr BasicBufferView<U> as_type() const noexcept {
        if (size_ % sizeof(U) != 0) {
            return BasicBufferView<U>(nullptr, 0);
        }
        if (reinterpret_cast<std::uintptr_t>(buf_) % alignof(U) != 0) {
            return BasicBufferView<U>(nullptr, 0);
        }
        return BasicBufferView<U>(reinterpret_cast<U*>(buf_),
                                  size_ / sizeof(U));
    };

   private:
    T* buf_;
    std::size_t size_;
};

using BufferView = BasicBufferView<const char>;
using MutableBufferView = BasicBufferView<char>;

struct CapacityPolicy {
   public:
    enum class Kind {
        Exact,         // Allocate exactly the requested size
        FiftyPercent,  // Round up to the next multiple of 1.5x
        PowerOfTwo,    // Round up to the next power of two
        CacheAligned,  // Round up to the next multiple of cache line size
        Custom         // Use a custom function to determine capacity
    } kind;
    std::size_t size = 0;
    using CapacityPolicyFunc = std::size_t (*)(std::size_t);
    CapacityPolicyFunc func = nullptr;

    static constexpr CapacityPolicy Exact(std::size_t size) {
        return CapacityPolicy{Kind::Exact, size};
    }

    static constexpr CapacityPolicy FiftyPercent() {
        return CapacityPolicy{Kind::FiftyPercent};
    }

    static constexpr CapacityPolicy PowerOfTwo() {
        return CapacityPolicy{Kind::PowerOfTwo};
    }
    static constexpr CapacityPolicy CacheAligned(
        std::size_t cache_line_size =
            std::hardware_destructive_interference_size) {
        return CapacityPolicy{Kind::CacheAligned, cache_line_size};
    };

    static constexpr CapacityPolicy Custom(CapacityPolicyFunc func) {
        return CapacityPolicy{Kind::Custom, 0, func};
    }
};
template <typename T = char, size_t Alignment = alignof(T),
          CapacityPolicy Policy = CapacityPolicy::FiftyPercent()>
class Buffer {
   public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

    constexpr Buffer() : size_(0), buf_(nullptr) {}
    Buffer(std::size_t size)
        : capacity_(adjust_capacity(size)),
          size_(size),
          buf_(static_cast<T*>(
              ::operator new(capacity_, std::align_val_t{Alignment}))) {}
    Buffer(const T* data, std::size_t size)
        : capacity_(adjust_capacity(size)),
          size_(size),
          buf_(static_cast<T*>(
              ::operator new(capacity_, std::align_val_t{Alignment}))) {
        std::memcpy(buf_, data, size_ * sizeof(T));
    }

    ~Buffer() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (std::size_t i = 0; i < size_; ++i) {
                buf_[i].~T();
            }
        }
        operator delete[](buf_, std::align_val_t{Alignment});
    }

    template <typename U = T, size_t A = Alignment, CapacityPolicy P = Policy>
    Buffer(const Buffer<U, A, P>& other) {
        size_ = other.size_;
        capacity_ = adjust_capacity(size_);
        buf_ = static_cast<T*>(
            ::operator new(capacity_, std::align_val_t{Alignment}));
        if constexpr (!std::is_trivially_copyable_v<U>) {
            for (std::size_t i = 0; i < size_; ++i) {
                new (buf_ + i) T(other.buf_[i]);
            }
        } else {
            std::memcpy(buf_, other.buf_, size_ * sizeof(T));
        }
    }

    template <typename U = T, size_t A = Alignment, CapacityPolicy P = Policy>
    Buffer(Buffer<U, A, P>&& other) noexcept
        : capacity_(other.capacity_), size_(other.size_), buf_(other.buf_) {
        if constexpr (A != Alignment || P.kind != Policy.kind) {
            // If the other buffer has different alignment or capacity policy,
            // we need to reallocate and copy the data to ensure correct
            // behavior
            capacity_ = adjust_capacity(size_);
            void* new_buf = ::operator new(capacity_ * sizeof(T),
                                           std::align_val_t{Alignment});
            std::memcpy(new_buf, buf_, size_ * sizeof(T));
            operator delete(buf_, std::align_val_t{Alignment});
            buf_ = static_cast<T*>(new_buf);
        }
        other.buf_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    template <typename U = T, size_t A = Alignment, CapacityPolicy P = Policy>
    Buffer& operator=(const Buffer<U, A, P>& other) {
        if (this != &other) {
            operator delete(buf_, std::align_val_t{Alignment});
            size_ = other.size_;
            capacity_ = adjust_capacity(size_);
            buf_ = static_cast<T*>(
                ::operator new(capacity_, std::align_val_t{Alignment}));
            if constexpr (!std::is_trivially_copyable_v<U>) {
                for (std::size_t i = 0; i < size_; ++i) {
                    new (buf_ + i) T(other.buf_[i]);
                }
            } else {
                std::memcpy(buf_, other.buf_, size_ * sizeof(T));
            }
        }
        return *this;
    }

    template <typename U = T, size_t A = Alignment, CapacityPolicy P = Policy>
    Buffer& operator=(Buffer<U, A, P>&& other) noexcept {
        if (this != &other) {
            operator delete(buf_, std::align_val_t{Alignment});
            if constexpr (A != Alignment || P.kind != Policy.kind) {
                // If the other buffer has different alignment or capacity
                // policy, we need to reallocate and copy the data to ensure
                // correct behavior
                capacity_ = adjust_capacity(other.size_);
                void* new_buf = ::operator new(capacity_ * sizeof(T),
                                               std::align_val_t{Alignment});
                std::memcpy(new_buf, other.buf_, other.size_ * sizeof(T));
                operator delete(buf_, std::align_val_t{Alignment});
                buf_ = static_cast<T*>(new_buf);
            } else {
                buf_ = other.buf_;
                size_ = other.size_;
                capacity_ = other.capacity_;
            }
            other.buf_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    constexpr T* data() noexcept { return buf_; }
    constexpr const T* data() const noexcept { return buf_; }

    constexpr std::size_t size() const noexcept { return size_; }
    constexpr std::size_t capacity() const noexcept { return capacity_; }

    constexpr std::size_t alignment() const noexcept { return Alignment; }

    constexpr bool empty() const noexcept { return size_ == 0; }

    constexpr T& push_back(T&& value) {
        if (size_ >= capacity_) {
            resize(size_ + 1);
        } else {
            ++size_;
        }
        new (buf_ + size_ - 1) T(std::forward<T>(value));
        return buf_[size_ - 1];
    }

    template <typename... Args>
    constexpr T& emplace_back(Args&&... args) {
        if (size_ >= capacity_) {
            resize(size_ + 1);
        } else {
            ++size_;
        }
        new (buf_ + size_ - 1) T(std::forward<Args>(args)...);
        return buf_[size_ - 1];
    }

    void resize(std::size_t new_size) noexcept(false) {
        if (new_size > capacity_) {
            std::size_t new_capacity = adjust_capacity(new_size);
            if (new_capacity < new_size) {
                throw std::bad_alloc();
            }
            void* new_buf = ::operator new(new_capacity * sizeof(T),
                                           std::align_val_t{Alignment});
            std::memcpy(new_buf, buf_, size_ * sizeof(T));
            operator delete(buf_, std::align_val_t{Alignment});
            buf_ = static_cast<T*>(new_buf);
            capacity_ = new_capacity;
        }
        size_ = new_size;
    }

    constexpr operator BasicBufferView<T>() noexcept {
        return BasicBufferView<T>(buf_, size_);
    }

    constexpr operator BasicBufferView<const T>() const noexcept {
        return BasicBufferView<const T>(buf_, size_);
    }

    T& at(std::size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Buffer index out of range");
        }
        return buf_[index];
    }

    const T& at(std::size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Buffer index out of range");
        }
        return buf_[index];
    }

    T& operator[](std::size_t index) noexcept { return buf_[index]; }
    const T& operator[](std::size_t index) const noexcept {
        return buf_[index];
    }

    operator bool() const noexcept { return buf_ != nullptr && size_ > 0; }

    T* begin() noexcept { return buf_; }
    T* end() noexcept { return buf_ + size_; }
    const T* cbegin() const noexcept { return buf_; }
    const T* cend() const noexcept { return buf_ + size_; }

   private:
    std::size_t capacity_;
    std::size_t size_;
    T* buf_;

    static constexpr std::size_t adjust_capacity(std::size_t requested_size) {
        if constexpr (Policy.kind == CapacityPolicy::Kind::Exact) {
            return requested_size;
        } else if constexpr (Policy.kind == CapacityPolicy::Kind::PowerOfTwo) {
            return std::bit_ceil(requested_size);
        } else if constexpr (Policy.kind ==
                             CapacityPolicy::Kind::CacheAligned) {
            constexpr std::size_t alignment = Policy.size;
            return ((requested_size + alignment - 1) / alignment) * alignment;
        } else if constexpr (Policy.kind == CapacityPolicy::Kind::Custom) {
            return Policy.func(requested_size);
        } else if constexpr (Policy.kind ==
                             CapacityPolicy::Kind::FiftyPercent) {
            std::size_t new_capacity = requested_size + (requested_size / 2);
            return new_capacity > requested_size ? new_capacity
                                                 : requested_size;
        } else {
            static_assert([] { return false; }(), "Unsupported CapacityPolicy");
        }
    }
};

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

    const char* begin() const noexcept { return buf_; }
    const char* end() const noexcept { return buf_ + size_; }

   private:
    const char* buf_;
    std::size_t size_;
};

class String {
   public:
    String() : buf_(0) {}
    explicit String(const StringView& v) : buf_() {
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

    explicit String(const char* str) : String(StringView(str)) {}
    explicit String(char* str) : String(StringView(str)) {}
    explicit String(const char* str, std::size_t size)
        : String(StringView(str, size)) {}
    explicit String(char* str, std::size_t size)
        : String(StringView(str, size)) {}

    String(String&& other) noexcept : buf_(std::move(other.buf_)) {}
    String& operator=(String&& other) noexcept {
        if (this != &other) {
            buf_ = std::move(other.buf_);
        }
        return *this;
    }

    operator StringView() const noexcept {
        return StringView(buf_.data(), buf_.size());
    }

    std::size_t size() const noexcept { return buf_.size(); }

    const char* c_str() const noexcept { return buf_.data(); }
    const char* data() const noexcept { return buf_.data(); }

    char operator[](std::size_t index) const noexcept { return buf_[index]; }

    String operator()(std::size_t offset, std::size_t length) const noexcept {
        return String(StringView(buf_.data() + offset, length));
    }

    const char* begin() const noexcept { return buf_.data(); }
    const char* end() const noexcept { return buf_.data() + buf_.size(); }

    bool operator!=(const String& other) const noexcept {
        return !(*this == other);
    }

    operator bool() const noexcept { return buf_; }

   private:
    Buffer<char> buf_;
};

inline bool operator==(const StringView& a, const StringView& o) noexcept {
    return a.size() == o.size() &&
           std::memcmp(a.data(), o.data(), a.size()) == 0;
}

inline String StringView::str() const { return String(*this); }

};  // namespace csics
