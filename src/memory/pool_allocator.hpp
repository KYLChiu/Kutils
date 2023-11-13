#include <cstddef>
#include <exception>
#include <iostream>

template <std::size_t PoolSize>
class memory_pool {
   public:
    memory_pool() {
        static_assert(PoolSize >= sizeof(block),
                      "Pool size must be large enough for at least one block");
        initialize_memory_pool();
    }

    ~memory_pool() = default;

    void* allocate(std::size_t size) {
        if (size == 0 || size > pool_size_) {
            throw std::bad_alloc();
        }

        if (! free_list_ || size + sizeof(block) > free_list_->size) {
            return allocate_new_block(size);
        }

        block* allocated_block = free_list_;

        free_list_ = free_list_->next;

        return reinterpret_cast<void*>(
            reinterpret_cast<char*>(allocated_block) + sizeof(block));
    }

    void deallocate(void* ptr) {
        if (! ptr) {
            return;
        }

        block* deallocated_block = reinterpret_cast<block*>(
            reinterpret_cast<char*>(ptr) - sizeof(block));
        std::cout << "Deallocating..." << &deallocated_block << std::endl;

        // Add the deallocated block back to the free list
        deallocated_block->next = free_list_;
        free_list_ = deallocated_block;
    }

   private:
    struct block {
        std::size_t size;
        block* next;
    };

    alignas(alignof(std::max_align_t)) char memory_pool_[PoolSize];
    block* free_list_;
    std::size_t pool_size_;
    std::size_t
        offset_;  // Keep track of the offset within the fixed-size array

    void initialize_memory_pool() {
        block* initial_block = reinterpret_cast<block*>(memory_pool_);
        initial_block->size = PoolSize - sizeof(block);
        initial_block->next = nullptr;
        free_list_ = initial_block;
        pool_size_ = PoolSize;
        offset_ = 0;
    }

    void* allocate_new_block(std::size_t size) {
        std::size_t block_size = size + sizeof(block);

        if (block_size > pool_size_) {
            throw std::bad_alloc();
        }

        block* new_block = reinterpret_cast<block*>(memory_pool_ + offset_);
        new_block->size = pool_size_ - offset_ - sizeof(block);
        new_block->next = nullptr;

        // Update free list to point to the new block
        free_list_ = new_block;

        // Update offset to the next available address
        offset_ += block_size;

        // Return the address immediately after the block metadata
        return reinterpret_cast<void*>(reinterpret_cast<char*>(new_block) +
                                       sizeof(block));
    }
};

template <typename T, std::size_t PoolSize>
class pool_allocator {
   public:
    using value_type = T;

    template <typename U>
    struct rebind {
        using other = pool_allocator<U, PoolSize>;
    };

    pool_allocator(memory_pool<PoolSize>& memory_pool)
        : memory_pool_(memory_pool) {}

    T* allocate(std::size_t n) {
        void* memory = memory_pool_.allocate(n * sizeof(T));
        if (! memory) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(memory);
    }

    void deallocate(T* p, std::size_t /**/) noexcept {
        memory_pool_.deallocate(p);
    }

   private:
    memory_pool<PoolSize>& memory_pool_;
};