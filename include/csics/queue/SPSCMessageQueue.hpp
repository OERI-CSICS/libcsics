
#include <optional>

#include "csics/queue/SPSCQueue.hpp"
namespace csics::queue {
template <typename T>
class SPSCMessageQueue {
   public:
    SPSCMessageQueue(size_t capacity) : queue_(capacity) {}
    SPSCMessageQueue(const SPSCMessageQueue&) = delete;
    SPSCMessageQueue& operator=(const SPSCMessageQueue&) = delete;
    SPSCMessageQueue(SPSCMessageQueue&&) noexcept = default;
    SPSCMessageQueue& operator=(SPSCMessageQueue&&) noexcept = default;
    ~SPSCMessageQueue() = default;

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
