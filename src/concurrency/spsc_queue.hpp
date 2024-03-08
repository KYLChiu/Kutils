#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace kcu {

// Single producer single consumer lock free queue using a ring-buffer.
// TODO: allow other allocators
template <typename T>
class spsc_queue {
#ifdef __cpp_lib_hardware_interference_size
    using std::hardware_destructive_interference_size;
#else
    constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

    using alloc_t = std::allocator<T>;

   public:
    explicit spsc_queue(const std::size_t capacity)
        : capacity_(capacity), alloc_(alloc_t()) {
        ring_buffer_ = alloc_.allocate(capacity_);
    }
    spsc_queue(const spsc_queue&) = delete;
    spsc_queue& operator=(const spsc_queue&) = delete;

    ~spsc_queue() {
        while (! empty()) {
            pop();
        }
        alloc_.deallocate(ring_buffer_, capacity_);
    }

    void push(T&& t) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        const auto write_idx = write_idx_.load(std::memory_order_relaxed);
        // Unsigned int automatically wraps around when overflowing
        const std::size_t new_write_idx = write_idx + 1;
        new (&ring_buffer_[write_idx]) T(std::forward<T>(t));
        write_idx_.store(new_write_idx, std::memory_order_release);
    }

    void pop() noexcept(std::is_nothrow_destructible_v<T>) {
        const auto read_idx = read_idx_.load(std::memory_order_relaxed);
        // Unsigned int automatically wraps around when overflowing
        const std::size_t new_read_idx = read_idx + 1;
        ring_buffer_[read_idx].~T();
        read_idx_.store(new_read_idx, std::memory_order_release);
    }

    T* front() noexcept {
        const auto read_idx = read_idx_.load(std::memory_order_relaxed);
        return &ring_buffer_[read_idx];
    }

    std::size_t size() const noexcept {
        long diff = write_idx_.load(std::memory_order_acquire) -
                    read_idx_.load(std::memory_order_acquire);
        if (diff < 0) diff += capacity_;
        return static_cast<std::size_t>(diff);
    }

    bool empty() const noexcept {
        return write_idx_.load(std::memory_order_acquire) ==
               read_idx_.load(std::memory_order_acquire);
    }

   private:
    std::size_t capacity_;
    std::allocator<T> alloc_;
    T* ring_buffer_;

    alignas(hardware_destructive_interference_size)
        std::atomic<std::size_t> write_idx_;
    alignas(hardware_destructive_interference_size)
        std::atomic<std::size_t> read_idx_;
};

}  // namespace kcu