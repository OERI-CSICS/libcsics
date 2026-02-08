#include <atomic>
#include <csics/queue/SPSCQueue.hpp>
#include <cstring>
#include <new>
#include <algorithm>

namespace csics::queue {

// Next power of two taken from:
// https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
inline static constexpr std::size_t get_next_power_of_two(std::size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    if constexpr (sizeof(std::size_t) == 8)
        v |= v >> 32;  // since this is used as a heap allocation size,
                       // hoping this doesn't do anything
    return ++v;
}

SPSCQueue::SPSCQueue(size_t capacity) noexcept
    : capacity_(std::max(
          kCacheLineSize,
          get_next_power_of_two(capacity))),  // align to next power of 2
      buffer_(reinterpret_cast<std::byte*>(operator new(
          capacity_, std::align_val_t{kCacheLineSize}))),
      read_index_(0),
      write_index_(0) {}

SPSCQueue::~SPSCQueue() noexcept {
    operator delete(buffer_, std::align_val_t{kCacheLineSize});
};

SPSCError SPSCQueue::acquire_write(WriteSlot& slot, std::size_t size) noexcept {
    if (size > capacity_) {
        return SPSCError::TooBig;
    }

    const std::size_t read_index = read_index_.load(std::memory_order_acquire);
    const std::size_t write_index =
        write_index_.load(std::memory_order_relaxed);

    std::size_t mod_index = write_index & (capacity_ - 1);
    std::size_t pad_size = 0;
    std::size_t required_bytes = size + sizeof(QueueSlotHeader);
    QueueSlotHeader hdr{};

    if (mod_index + size + sizeof(QueueSlotHeader) >= capacity_) {
        pad_size = capacity_ - mod_index - sizeof(QueueSlotHeader);
        required_bytes += pad_size + sizeof(QueueSlotHeader);
    }

    if (write_index - read_index + required_bytes >= capacity_) {
        return SPSCError::Full;
    }

    if (pad_size > 0) {
        hdr.size = pad_size;
        hdr.padded = 1;
        std::memcpy(&buffer_[mod_index], &hdr, sizeof(QueueSlotHeader));
        mod_index = 0;
        write_index_.fetch_add(pad_size + sizeof(QueueSlotHeader),
                               std::memory_order_release);
    }

    hdr.size = size;
    hdr.padded = 0;
    std::memcpy(&buffer_[mod_index], &hdr, sizeof(QueueSlotHeader));
    slot.data = &buffer_[mod_index] + sizeof(QueueSlotHeader);
    slot.size = size;

    return SPSCError::None;
};

SPSCError SPSCQueue::acquire_read(ReadSlot& slot) noexcept {
    const std::size_t read_index = read_index_.load(std::memory_order_relaxed);
    const std::size_t write_index =
        write_index_.load(std::memory_order_acquire);

    std::size_t mod_index = read_index & (capacity_ - 1);

    if (read_index == write_index) {
        return SPSCError::Empty;
    }

    QueueSlotHeader* hdr =
        reinterpret_cast<QueueSlotHeader*>(&buffer_[mod_index]);

    if (hdr->padded) {
        mod_index = 0;
        read_index_.fetch_add(hdr->size + sizeof(QueueSlotHeader),
                              std::memory_order_release);
        hdr = reinterpret_cast<QueueSlotHeader*>(&buffer_[mod_index]);
    }
    slot.size = hdr->size;
    slot.data = &buffer_[mod_index] + sizeof(QueueSlotHeader);
    return SPSCError::None;
}

void SPSCQueue::commit_write(WriteSlot&& slot) noexcept {
    auto new_index = write_index_.load(std::memory_order_acquire) + slot.size +
                     sizeof(QueueSlotHeader);
    new_index = (new_index + kCacheLineSize - 1) & ~(kCacheLineSize - 1);

    write_index_.store(new_index, std::memory_order_release);
}

void SPSCQueue::commit_read(ReadSlot&& slot) noexcept {
    auto new_index = read_index_.load(std::memory_order_acquire) + slot.size +
                     sizeof(QueueSlotHeader);
    new_index = (new_index + kCacheLineSize - 1) & ~(kCacheLineSize - 1);
    read_index_.store(new_index, std::memory_order_release);
}
};  // namespace csics::queue
