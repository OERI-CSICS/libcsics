#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "csics/assert.hpp"
#include "csics/compiler.hpp"
namespace csics {

#ifdef CACHE_LINE_SIZE
constexpr size_t kCacheLineSize = CACHE_LINE_SIZE;
#elif defined(__cpp_lib_hardware_interference_size)

CSICS_DISABLE_WARNING_PUSH
CSICS_DISABLE_WARNING_GCC("-Winterference-size")
CSICS_DISABLE_WARNING_MSVC(5033)
constexpr size_t kCacheLineSize = std::hardware_destructive_interference_size;
CSICS_DISABLE_WARNING_POP
#else
constexpr size_t kCacheLineSize = 128;  // Safe assumption
#endif

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
        return size_ == other.size_ &&
               (buf_ == other.buf_ ||
                std::memcmp(buf_, other.buf_, size_) == 0);
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

    const T* begin() const noexcept { return buf_; }
    const T* end() const noexcept { return buf_ + size_; }
    T* begin() noexcept { return buf_; }
    T* end() noexcept { return buf_ + size_; }

    const T* cbegin() const noexcept { return buf_; }
    const T* cend() const noexcept { return buf_ + size_; }

    constexpr BasicBufferView() : buf_(nullptr), size_(0) {}

    template <std::ranges::contiguous_range R>
        requires(!IsByteType<T>) &&
                    (std::same_as<std::ranges::range_value_t<R>, T> ||
                     std::convertible_to<std::ranges::range_value_t<R>, T>)
    constexpr BasicBufferView(R& range) noexcept
        : buf_(reinterpret_cast<T*>(std::ranges::data(range))),
          size_(std::ranges::size(range)) {}

    template <std::ranges::contiguous_range R>
        requires std::same_as<std::ranges::range_value_t<R>, T> ||
                     std::convertible_to<std::ranges::range_value_t<R>, T>
    constexpr BasicBufferView(const R& range) noexcept
        : buf_(reinterpret_cast<T*>(std::ranges::data(range))),
          size_(std::ranges::size(range)) {}

    template <std::ranges::contiguous_range R>
        requires IsByteType<T>
    constexpr BasicBufferView(R& range) noexcept
        : buf_(reinterpret_cast<T*>(std::ranges::data(range))),
          size_(std::ranges::size(range) *
                sizeof(std::ranges::range_value_t<R>)) {}

    template <std::ranges::contiguous_range R>
        requires IsByteType<T>
    constexpr BasicBufferView(const R& range) noexcept
        : buf_(reinterpret_cast<T*>(std::ranges::data(range))),
          size_(std::ranges::size(range) *
                sizeof(std::ranges::range_value_t<R>)) {}

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

    // constexpr BasicBufferView(std::vector<T>& vec) noexcept
    //     : buf_(reinterpret_cast<T*>(vec.data())), size_(vec.size()) {}

    template <typename U>
        requires(SameConst<T, U> || ConstConvertible<T, U>)
    constexpr BasicBufferView(std::vector<U>& vec) noexcept
        : buf_(reinterpret_cast<T*>(vec.data())),
          size_(vec.size() * sizeof(U)) {}

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
    }

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
        std::size_t cache_line_size = kCacheLineSize) {
        return CapacityPolicy{Kind::CacheAligned, cache_line_size};
    }

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

    constexpr Buffer() : capacity_(0), size_(0), buf_(nullptr) {}

    Buffer(std::size_t size)
        : capacity_(adjust_capacity(size)),
          size_(size),
          buf_(static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                              std::align_val_t{Alignment}))) {}
    constexpr Buffer(std::size_t size, const T& value)
        : capacity_(adjust_capacity(size)),
          size_(size),
          buf_(static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                              std::align_val_t{Alignment}))) {
        if constexpr (!std::is_trivially_copyable_v<T>) {
            std::uninitialized_fill_n(buf_, size_, value);
        } else {
            for (std::size_t i = 0; i < size_; ++i) {
                std::memcpy(buf_ + i, &value, sizeof(T));
            }
        }
    }
    Buffer(const T* data, std::size_t size)
        : capacity_(adjust_capacity(size)),
          size_(size),
          buf_(static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                              std::align_val_t{Alignment}))) {
        if constexpr (!std::is_trivially_copyable_v<T>) {
            std::uninitialized_copy(data, data + size_, buf_);
        } else {
            std::memcpy(buf_, data, size_ * sizeof(T));
        }
    }

    Buffer(std::initializer_list<T> init)
        : capacity_(adjust_capacity(init.size())),
          size_(init.size()),
          buf_(static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                              std::align_val_t{Alignment}))) {
        if constexpr (!std::is_trivially_copyable_v<T>) {
            std::uninitialized_copy(init.begin(), init.end(), buf_);
        } else {
            std::memcpy(buf_, init.begin(), size_ * sizeof(T));
        }
    }

    ~Buffer() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(buf_, size_);
        }
        operator delete(buf_, std::align_val_t{Alignment});
    }

    template <size_t A = Alignment, CapacityPolicy P = Policy>
    Buffer(const Buffer<T, A, P>& other)
        : size_(other.size_),
          capacity_(adjust_capacity(other.size_)),
          buf_(static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                              std::align_val_t{Alignment}))) {
        if constexpr (!std::is_trivially_copyable_v<T>) {
            std::uninitialized_copy(other.buf_, other.buf_ + size_, buf_);
        } else {
            std::memcpy(buf_, other.buf_, size_ * sizeof(T));
        }
    }

    Buffer(const Buffer& other) {
        if (other.buf_ == nullptr) {
            buf_ = nullptr;
            size_ = 0;
            capacity_ = 0;
            return;
        }
        size_ = other.size_;
        capacity_ = adjust_capacity(size_);
        buf_ = static_cast<T*>(
            ::operator new(capacity_ * sizeof(T), std::align_val_t{Alignment}));
        if constexpr (!std::is_trivially_copyable_v<T>) {
            std::uninitialized_copy(other.buf_, other.buf_ + size_, buf_);
        } else {
            std::memcpy(buf_, other.buf_, size_ * sizeof(T));
        }
    }

    template <size_t A = Alignment, CapacityPolicy P = Policy>
    Buffer(Buffer<T, A, P>&& other) noexcept
        : capacity_(other.capacity_), size_(other.size_) {
        if constexpr (A != Alignment || P.kind != Policy.kind) {
            capacity_ = adjust_capacity(other.size_);

            buf_ = static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                                  std::align_val_t{Alignment}));
            if constexpr (!std::is_trivially_copyable_v<T>) {
                std::uninitialized_move(other.buf_, other.buf_ + size_, buf_);
            } else {
                std::memcpy(buf_, other.buf_, size_ * sizeof(T));
            }
        } else {
            if constexpr (!std::is_trivially_copyable_v<T>) {
                buf_ = static_cast<T*>(::operator new(
                    capacity_ * sizeof(T), std::align_val_t{Alignment}));
                std::uninitialized_move(other.buf_, other.buf_ + size_, buf_);
            } else {
                buf_ = other.buf_;
                other.buf_ = nullptr;
            }
        }
    }

    void swap_and_pop(iterator pos) {
        CSICS_RUNTIME_ASSERT(pos >= begin() && pos < end(),
                             "Iterator out of range");
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_at(pos);
        }
        if (pos != end() - 1) {
            *pos = std::move(*(end() - 1));
        }
        --size_;
    };

    void erase(iterator pos) { erase(pos, pos + 1); }

    void erase(iterator first, iterator last) {
        CSICS_RUNTIME_ASSERT(first >= buf_ && first < buf_ + size_,
                             "First iterator out of range, first: " +
                                 std::to_string((std::size_t)first) +
                                 ", buf_: " + std::to_string((std::size_t)buf_) +
                                 ", end: " + std::to_string((std::size_t)(end())));
        CSICS_RUNTIME_ASSERT(last >= buf_ && last <= buf_ + size_,
                             "Last iterator out of range");
        CSICS_RUNTIME_ASSERT(first <= last,
                             "First iterator must not be after last");
        // shift elements down to fill the gap
        auto dest = first;
        auto src = last;
        for (; src != end(); ++src, ++dest) {
            *dest = std::move(*src);
        }
        auto new_end = buf_ + (size_ - (last - first));
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(new_end, last - first);
        }
        size_ -= (last - first);
    };

    void clear() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(buf_, size_);
        }
        size_ = 0;
    };

    Buffer(Buffer&& other) noexcept {
        size_ = other.size_;
        capacity_ = other.capacity_;
        if constexpr (!std::is_trivially_copyable_v<T>) {
            buf_ = static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                                  std::align_val_t{Alignment}));
            std::uninitialized_move(other.buf_, other.buf_ + size_, buf_);
        } else {
            buf_ = other.buf_;
            other.buf_ = nullptr;
        }
    }

    template <size_t A = Alignment, CapacityPolicy P = Policy>
    Buffer& operator=(const Buffer<T, A, P>& other) {
        if (this != &other) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                std::destroy_n(buf_, size_);
            }
            operator delete(buf_, std::align_val_t{Alignment});
            size_ = other.size_;
            capacity_ = adjust_capacity(size_);
            buf_ = static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                                  std::align_val_t{Alignment}));
            if constexpr (!std::is_trivially_copyable_v<T>) {
                std::uninitialized_copy(other.buf_, other.buf_ + size_, buf_);
            } else {
                std::memcpy(buf_, other.buf_, size_ * sizeof(T));
            }
        }
        return *this;
    }

    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                std::destroy_n(buf_, size_);
            }
            operator delete(buf_, std::align_val_t{Alignment});
            size_ = other.size_;
            capacity_ = adjust_capacity(size_);
            buf_ = static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                                  std::align_val_t{Alignment}));
            if constexpr (!std::is_trivially_copyable_v<T>) {
                std::uninitialized_copy(other.buf_, other.buf_ + size_, buf_);
            } else {
                std::memcpy(buf_, other.buf_, size_ * sizeof(T));
            }
        }
        return *this;
    }

    template <size_t A = Alignment, CapacityPolicy P = Policy>
    Buffer& operator=(Buffer<T, A, P>&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(buf_, size_);
        }
        operator delete(buf_, std::align_val_t{Alignment});
        size_ = other.size_;

        if constexpr (A != Alignment || P.kind != Policy.kind) {
            capacity_ = adjust_capacity(other.size_);
            buf_ = static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                                  std::align_val_t{Alignment}));
            if constexpr (!std::is_trivially_copyable_v<T>) {
                std::uninitialized_move(other.buf_, other.buf_ + size_, buf_);
            } else {
                std::memcpy(buf_, other.buf_, size_ * sizeof(T));
            }
        } else {
            if constexpr (!std::is_trivially_copyable_v<T>) {
                buf_ = static_cast<T*>(::operator new(
                    capacity_ * sizeof(T), std::align_val_t{Alignment}));
                std::uninitialized_move(other.buf_, other.buf_ + size_, buf_);
            } else {
                buf_ = other.buf_;
                other.buf_ = nullptr;
            }
        }
        return *this;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(buf_, size_);
        }
        operator delete(buf_, std::align_val_t{Alignment});
        size_ = other.size_;
        capacity_ = other.capacity_;
        if constexpr (!std::is_trivially_copyable_v<T>) {
            buf_ = static_cast<T*>(::operator new(capacity_ * sizeof(T),
                                                  std::align_val_t{Alignment}));
            std::uninitialized_move(other.buf_, other.buf_ + size_, buf_);
        } else {
            buf_ = other.buf_;
            other.buf_ = nullptr;
        }
        return *this;
    }

    constexpr T* data() noexcept { return buf_; }
    constexpr const T* data() const noexcept { return buf_; }

    constexpr std::size_t size() const noexcept { return size_; }
    constexpr std::size_t capacity() const noexcept { return capacity_; }

    constexpr std::size_t alignment() const noexcept { return Alignment; }

    constexpr bool empty() const noexcept { return size_ == 0; }

    constexpr T& push_back(const T& value) & {
        add_capacity_for(1);
        T* slot = buf_ + size_;
        std::construct_at(slot, value);
        size_++;
        return buf_[size_ - 1];
    }

    constexpr T& push_back(T&& value) & {
        add_capacity_for(1);
        T* slot = buf_ + size_;
        std::construct_at(slot, std::move(value));
        size_++;
        return buf_[size_ - 1];
    }

    template <typename... Args>
    constexpr T& emplace_back(Args&&... args) {
        add_capacity_for(1);
        T* slot = buf_ + size_;
        std::construct_at(slot, std::forward<Args>(args)...);
        size_++;
        return buf_[size_ - 1];
    }

    constexpr T pop_back() {
        CSICS_RUNTIME_ASSERT(size_ > 0, "Cannot pop_back from an empty buffer");
        T ret = std::move(buf_[size_ - 1]);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_at(buf_ + size_ - 1);
        }
        size_--;
        return ret;
    }

    void resize(std::size_t new_size) noexcept(false) {
        if (new_size > capacity_) {
            reallocate(new_size);
        }
        if (new_size > size_) {
            if constexpr (!std::is_trivially_default_constructible_v<T>) {
                std::uninitialized_default_construct_n(buf_ + size_,
                                                       new_size - size_);
            }
        } else if (new_size < size_) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                std::destroy_n(buf_ + new_size, size_ - new_size);
            }
        }

        size_ = new_size;
    }

    void append(const T* data, std::size_t count) {
        CSICS_RUNTIME_ASSERT(data != nullptr, "Data pointer cannot be null");
        if (count == 0) [[unlikely]] {
            return;
        }

        add_capacity_for(count);
        if constexpr (!std::is_trivially_copyable_v<T>) {
            std::uninitialized_copy(data, data + count, buf_ + size_);
        } else {
            std::memcpy(buf_ + size_, data, count * sizeof(T));
        }
        size_ += count;
    }

    void append(const BufferView view) {
        if (view.empty()) [[unlikely]] {
            return;
        }
        return append(view.data(), view.size());
    }

    template <typename U = T>
        requires std::is_trivially_copyable_v<U>
    T* append_uninitialized(std::size_t count) {
        add_capacity_for(count);
        T* start = buf_ + size_;
        size_ += count;
        return start;
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

    BasicBufferView<T> operator()(std::size_t offset,
                                  std::size_t length) noexcept {
        if (offset >= size_) {
            return BasicBufferView<T>();
        }
        if (offset + length > size_) {
            length = size_ - offset;
        }
        return BasicBufferView<T>(buf_ + offset, length);
    }

    BasicBufferView<T> operator()(iterator start, iterator end) noexcept {
        CSICS_RUNTIME_ASSERT(start >= buf_ && start <= buf_ + size_,
                             "Start iterator out of range");
        CSICS_RUNTIME_ASSERT(end >= buf_ && end <= buf_ + size_,
                             "End iterator out of range");
        CSICS_RUNTIME_ASSERT(start <= end,
                             "Start iterator must not be after end");
        return BasicBufferView<T>(start, end - start);
    }

    operator bool() const noexcept { return buf_ != nullptr && size_ > 0; }

    T* begin() noexcept { return buf_; }
    T* end() noexcept { return buf_ + size_; }
    const T* begin() const noexcept { return buf_; }
    const T* end() const noexcept { return buf_ + size_; }
    const T* cbegin() const noexcept { return buf_; }
    const T* cend() const noexcept { return buf_ + size_; }

   private:
    std::size_t capacity_;
    std::size_t size_;
    T* buf_;

    void add_capacity_for(std::size_t additional_size) {
        if (size_ + additional_size > capacity_) {
            reallocate(size_ + additional_size);
        }
    }

    constexpr void destroy_and_copy_to(T* new_buf) {
        if (buf_ == nullptr) {
            return;
        }
        if constexpr (!std::is_trivially_copyable_v<T>) {
            std::uninitialized_move(buf_, buf_ + size_, new_buf);
            if constexpr (!std::is_trivially_destructible_v<T>) {
                std::destroy_n(buf_, size_);
            }
        } else {
            std::memcpy(new_buf, buf_, size_ * sizeof(T));
        }
    }

    void reallocate(std::size_t new_capacity) {
        new_capacity = adjust_capacity(new_capacity);
        CSICS_RUNTIME_ASSERT(
            new_capacity >= size_,
            "New capacity must be greater than or equal to current size");
        void* new_buf = ::operator new(new_capacity * sizeof(T),
                                       std::align_val_t{Alignment});

        destroy_and_copy_to(static_cast<T*>(new_buf));

        operator delete(buf_, std::align_val_t{Alignment});
        buf_ = static_cast<T*>(new_buf);
        capacity_ = new_capacity;
    }

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
            std::size_t new_capacity = std::ceil(requested_size * 1.5f);
            return new_capacity > requested_size ? new_capacity
                                                 : requested_size;
        } else {
            static_assert([] { return false; }(), "Unsupported CapacityPolicy");
        }
    }
};

template <typename T = char, size_t Alignment = alignof(T),
          CapacityPolicy Policy = CapacityPolicy::FiftyPercent(),
          typename MutexType = std::mutex>
class BufferGuard {
   public:
    BufferGuard(Buffer<T, Alignment, Policy>& buf, MutexType& mutex)
        : buf_(buf), lock_(mutex) {}

    ~BufferGuard() {}

    Buffer<T>* operator->() { return &buf_; }
    const Buffer<T>* operator->() const { return &buf_; }
    Buffer<T>& operator*() { return buf_; }
    const Buffer<T>& operator*() const { return buf_; }

    operator BasicBufferView<T>() const noexcept { return buf_; }

    BufferGuard(const BufferGuard&) = delete;
    BufferGuard& operator=(const BufferGuard&) = delete;
    BufferGuard(BufferGuard&&) = default;

   private:
    Buffer<T, Alignment, Policy>& buf_;
    std::unique_lock<MutexType> lock_;
};

};  // namespace csics
