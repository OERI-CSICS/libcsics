#pragma once

#include <atomic>
#include <cstddef>
#include <iterator>
#include <new>

namespace csics::queue {

#ifdef CACHE_LINE_SIZE
constexpr size_t kCacheLineSize = CACHE_LINE_SIZE;
#else
constexpr size_t kCacheLineSize = std::hardware_destructive_interference_size;
#endif
class SPSCQueue;

enum class SPSCError {
    None,
    Full,
    Empty,
    TooBig,
};

// Single Producer Single Consumer Queue
// Uses a circular buffer with atomic indices for read and write.
// API uses acquire/commit semantics for both read and write.
// Used for low-level communication between threads with minimal overhead.
// Supports blocking and non-blocking acquire methods.
// Supports stopping the queue to unblock waiting threads.
class SPSCQueue {
   public:
    struct ReadSlot;
    struct WriteSlot;
    class ReadHandle;
    class WriteHandle;

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    explicit SPSCQueue(size_t capacity) noexcept;
    ~SPSCQueue() noexcept;

    // Acquire a read slot.
    // ReadSlot will be populated with the data pointer and size.
    // Returns true if successful, false otherwise.
    // Will return false if the queue is stopped and there is no data to read.
    [[nodiscard]]
    SPSCError acquire_read(ReadSlot& slot) noexcept;

    // Release a previously acquired read slot.
    void commit_read(ReadSlot&& slot) noexcept;

    // Acquire a write slot.
    // WriteSlot will be populated with the data pointer and size.
    // Blocks until a slot is available or the queue is stopped.
    [[nodiscard]]
    SPSCError acquire_write(WriteSlot& slot, std::size_t size) noexcept;

    // Release a previously acquired write slot.
    void commit_write(WriteSlot&& slot) noexcept;

    inline std::size_t capacity() const noexcept { return capacity_; }

    inline bool has_pending_data() const noexcept {
        return read_index_.load(std::memory_order_acquire) <
               write_index_.load(std::memory_order_acquire);
    }

    inline ReadHandle get_read_handle() & noexcept {
        return ReadHandle(*this);
    }

    inline WriteHandle get_write_handle() & noexcept {
        return WriteHandle(*this);
    }

   private:
    std::size_t capacity_;
    std::byte* buffer_;

    struct QueueSlotHeader {  // extendable header, realistically only a size.
        uint64_t padded : 1;
        uint64_t size : 63;
    };

#ifdef _MSC_VER
#pragma warning(disable : 4324)  // disable MSVC warning 4324. We don't care
                                 // about the padding here
#endif
    alignas(kCacheLineSize) std::atomic<size_t> read_index_;
    alignas(kCacheLineSize) std::atomic<size_t> write_index_;

    inline bool is_full();

   public:
    class ReadHandle {
        public:
            [[nodiscard]]
            inline SPSCError acquire(ReadSlot& slot) noexcept {
                return queue_.acquire_read(slot);
            }

            inline void commit(ReadSlot&& slot) noexcept {
                queue_.commit_read(std::move(slot));
            }

            ReadHandle(const ReadHandle&) = delete;
            ReadHandle& operator=(const ReadHandle&) = delete;
            ReadHandle(ReadHandle&&) = default;
            ReadHandle& operator=(ReadHandle&&) = delete;

        protected:
            explicit ReadHandle(SPSCQueue& queue) : queue_(queue) {}
        private:
            SPSCQueue& queue_;
            friend class SPSCQueue;
    };

    class WriteHandle {
        public:
            [[nodiscard]]
            inline SPSCError acquire(WriteSlot& slot, std::size_t size) noexcept {
                return queue_.acquire_write(slot, size);
            }

            inline void commit(WriteSlot&& slot) noexcept {
                queue_.commit_write(std::move(slot));
            }

            WriteHandle(const WriteHandle&) = delete;
            WriteHandle& operator=(const WriteHandle&) = delete;
            WriteHandle(WriteHandle&&) = default;
            WriteHandle& operator=(WriteHandle&&) = delete;

        protected:
            explicit WriteHandle(SPSCQueue& queue) : queue_(queue) {}
        private:
            SPSCQueue& queue_;
            friend class SPSCQueue;
    };

    struct ReadSlot {
        std::byte* data;
        size_t size;

        ReadSlot() : data(nullptr), size(0) {}
        ReadSlot(const ReadSlot& other) = delete;
        ReadSlot& operator=(const ReadSlot& other) = delete;
        ReadSlot(ReadSlot&& other) noexcept
            : data(other.data), size(other.size) {
            other.data = nullptr;
            other.size = 0;
        }
        ReadSlot& operator=(ReadSlot&& other) = delete;


        template <typename T>
        const T* as() const noexcept {
            return *reinterpret_cast<T*>(data);
        }

        template<typename Header, typename Data>
        void as_block(Header*& header, Data*& data) const noexcept {
            header = reinterpret_cast<Header*>(this->data);
            data = reinterpret_cast<Data*>(this->data + sizeof(Header));
        }
    };

    struct WriteSlot {
        std::byte* data;
        size_t size;

        WriteSlot() : data(nullptr), size(0) {}
        WriteSlot(const WriteSlot& other) = delete;
        WriteSlot& operator=(const WriteSlot& other) = delete;
        WriteSlot(WriteSlot&& other) noexcept
            : data(other.data), size(other.size) {
            other.data = nullptr;
            other.size = 0;
        }
        WriteSlot& operator=(WriteSlot&& other) = delete;

        template <typename T>
        T* as() {
            return *static_cast<T*>(data);
        }

        template <typename Header, typename Data>
        void as_block(Header*& header, Data*& data) const noexcept {
            header = reinterpret_cast<Header*>(this->data);
            data = reinterpret_cast<Data*>(this->data + sizeof(Header));
        }
    };
};
};  // namespace csics::queue
