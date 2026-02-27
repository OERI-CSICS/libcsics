
#include <optional>

#include "csics/queue/SPSCQueue.hpp"
namespace csics::queue {
template <typename T>
class SPSCMessageQueue {
   public:
    static constexpr std::size_t read_size = // wacky, but it ensures each message is aligned
        sizeof(T) > kCacheLineSize           // and ensures that at least capacity messages can fit in the queue
            ? sizeof(T) + (kCacheLineSize - sizeof(T) % kCacheLineSize)
            : kCacheLineSize;
    SPSCMessageQueue(size_t capacity) : queue_(capacity * read_size) {
        std::cerr << "Initialized SPSCMessageQueue with capacity for "
                  << capacity << " messages of size " << sizeof(T)
                  << " bytes each (total " << queue_.capacity() << " bytes)"
                  << std::endl;
    }
    SPSCMessageQueue(const SPSCMessageQueue&) = delete;
    SPSCMessageQueue& operator=(const SPSCMessageQueue&) = delete;
    SPSCMessageQueue(SPSCMessageQueue&&) noexcept
        : queue_(std::move(queue_)) {};
    SPSCMessageQueue& operator=(SPSCMessageQueue&& other) noexcept {
        if (this != &other) {
            queue_ = std::move(other.queue_);
        }
        return *this;
    }
    ~SPSCMessageQueue() {
        if (!queue_.empty()) {
            SPSCQueue::ReadSlot slot;
            while (queue_.acquire_read(slot) == SPSCError::None) {
                T* value_ptr = reinterpret_cast<T*>(slot.data);
                value_ptr->~T();
                queue_.commit_read(std::move(slot));
            }
        }
    }

    [[nodiscard]]
    SPSCError try_pop(T& msg) {
        SPSCQueue::ReadSlot slot;
        auto ret = queue_.acquire_read(slot);
        if (ret != SPSCError::None) {
            return ret;
        }
        T* value_ptr = reinterpret_cast<T*>(slot.data);
        // Move the value out of the slot and destroy the original
        msg = std::move(*value_ptr);
        value_ptr->~T();
        queue_.commit_read(std::move(slot));
        return SPSCError::None;
    }

    [[nodiscard]]
    SPSCError try_push(const T& value) {
        SPSCQueue::WriteSlot slot;
        auto ret = queue_.acquire_write(slot, sizeof(T));
        if (ret != SPSCError::None) {
            return ret;
        }
        new (slot.data) T(value);
        queue_.commit_write(std::move(slot));
        return SPSCError::None;
    }

    [[nodiscard]]
    SPSCError try_push(T&& value) {
        SPSCQueue::WriteSlot slot;
        auto ret = queue_.acquire_write(slot, sizeof(T));
        if (ret != SPSCError::None) {
            return ret;
        }
        new (slot.data) T(std::move(value));
        queue_.commit_write(std::move(slot));
        return SPSCError::None;
    }

    inline bool empty() const noexcept { return queue_.empty(); }

   private:
    SPSCQueue queue_;
};
};  // namespace csics::queue
