// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>

SLICK_DETAIL_NAMESPACE_BEGIN

/// Object pool for zero-allocation management of frequently created/destroyed objects
///
/// Benefits:
/// - Zero allocations in hot path (after initial pool creation)
/// - Cache-friendly memory layout (objects allocated in contiguous blocks)
/// - Fast allocation/deallocation (free-list based, O(1))
/// - Automatic memory management with RAII
///
/// Usage:
/// @code
/// ObjectPool<Order> pool(1024);  // Pre-allocate 1024 orders
/// Order* order = pool.allocate();
/// // ... use order
/// pool.deallocate(order);
/// @endcode
///
/// @tparam T Object type to pool
template<typename T>
class ObjectPool {
public:
    static_assert(std::is_nothrow_destructible_v<T>,
                  "ObjectPool requires nothrow destructible types");
    // Note: ObjectPool supports over-aligned types via std::align_val_t
    // (see operator delete calls with std::align_val_t{alignof(T)})

    /// Constructor - pre-allocates initial capacity
    /// @param initial_capacity Number of objects to pre-allocate
    explicit ObjectPool(std::size_t initial_capacity = 1024)
        : capacity_(0), size_(0), free_list_(nullptr) {
        if (initial_capacity > 0) {
            reserve(initial_capacity);
        }
    }

    /// Destructor - frees all memory blocks
    ~ObjectPool() noexcept {
        clear();
        for (Block& block : blocks_) {
            ::operator delete(block.memory, std::align_val_t{alignof(T)});
        }
    }

    // Non-copyable
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    // Movable
    ObjectPool(ObjectPool&& other) noexcept
        : blocks_(std::move(other.blocks_)),
          capacity_(other.capacity_),
          size_(other.size_),
          free_list_(other.free_list_) {
        other.capacity_ = 0;
        other.size_ = 0;
        other.free_list_ = nullptr;
    }

    ObjectPool& operator=(ObjectPool&& other) noexcept {
        if (this != &other) {
            clear();
            for (Block& block : blocks_) {
                ::operator delete(block.memory, std::align_val_t{alignof(T)});
            }
            blocks_ = std::move(other.blocks_);
            capacity_ = other.capacity_;
            size_ = other.size_;
            free_list_ = other.free_list_;
            other.capacity_ = 0;
            other.size_ = 0;
            other.free_list_ = nullptr;
        }
        return *this;
    }

    /// Allocate an object from the pool
    /// Returns a pointer to uninitialized memory
    /// @return Pointer to allocated memory, or nullptr if allocation fails
    [[nodiscard]] T* allocate() {
        // Try to get from free list first
        if (free_list_ != nullptr) {
            FreeNode* node = free_list_;
            free_list_ = node->next;
            ++size_;
            return reinterpret_cast<T*>(node);
        }

        // No free objects, need to grow the pool
        if (!grow()) {
            return nullptr;
        }

        // Try again from free list
        if (free_list_ != nullptr) {
            FreeNode* node = free_list_;
            free_list_ = node->next;
            ++size_;
            return reinterpret_cast<T*>(node);
        }

        return nullptr;
    }

    /// Construct an object in-place with arguments
    /// @param args Constructor arguments
    /// @return Pointer to constructed object, or nullptr if allocation fails
    template<typename... Args>
    [[nodiscard]] T* construct(Args&&... args) {
        T* ptr = allocate();
        if (ptr != nullptr) {
            try {
                new (ptr) T(std::forward<Args>(args)...);
            } catch (...) {
                deallocate(ptr);
                throw;
            }
        }
        return ptr;
    }

    /// Deallocate an object (does not call destructor)
    /// @param ptr Pointer to object to deallocate
    void deallocate(T* ptr) noexcept {
        if (ptr == nullptr) {
            return;
        }

        // Add to free list
        FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = free_list_;
        free_list_ = node;
        --size_;
    }

    /// Destroy and deallocate an object
    /// @param ptr Pointer to object to destroy
    void destroy(T* ptr) noexcept {
        if (ptr != nullptr) {
            ptr->~T();
            deallocate(ptr);
        }
    }

    /// Reserve capacity for at least n objects
    /// @param n Minimum capacity to reserve
    /// @return true if successful, false otherwise
    bool reserve(std::size_t n) {
        if (n <= capacity_) {
            return true;
        }

        std::size_t to_allocate = n - capacity_;
        return grow(to_allocate);
    }

    /// Get current capacity (total objects that can be stored)
    [[nodiscard]] std::size_t capacity() const noexcept {
        return capacity_;
    }

    /// Get current size (objects currently allocated)
    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

    /// Get number of available objects in free list
    [[nodiscard]] std::size_t available() const noexcept {
        return capacity_ - size_;
    }

    /// Check if pool is empty
    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    /// Clear all allocated objects (calls destructors if any are alive)
    /// Warning: This assumes all objects have been properly destroyed
    void clear() noexcept {
        // Reset free list to include all objects from all blocks
        free_list_ = nullptr;
        for (Block& block : blocks_) {
            for (std::size_t i = 0; i < block.count; ++i) {
                T* obj = reinterpret_cast<T*>(
                    static_cast<char*>(block.memory) + i * sizeof(T));
                FreeNode* node = reinterpret_cast<FreeNode*>(obj);
                node->next = free_list_;
                free_list_ = node;
            }
        }
        size_ = 0;
    }

private:
    /// Free list node (overlays object memory when object is free)
    struct FreeNode {
        FreeNode* next;
    };

    static_assert(sizeof(FreeNode) <= sizeof(T),
                  "ObjectPool requires sizeof(T) >= sizeof(void*)");

    /// Memory block holding multiple objects
    struct Block {
        void* memory;       // Pointer to allocated memory
        std::size_t count;  // Number of objects in this block
    };

    /// Grow the pool by allocating a new block
    /// @param min_count Minimum number of objects to allocate
    /// @return true if successful, false otherwise
    bool grow(std::size_t min_count = 0) {
        // Use exponential growth strategy
        // Start with 64, double each time, up to 8192 per block
        constexpr std::size_t kMinBlockSize = 64;
        constexpr std::size_t kMaxBlockSize = 8192;

        std::size_t block_size = kMinBlockSize;
        if (!blocks_.empty()) {
            block_size = std::min(blocks_.back().count * 2, kMaxBlockSize);
        }

        // Ensure we allocate at least min_count
        if (block_size < min_count) {
            block_size = min_count;
        }

        // Allocate new block
        void* memory = ::operator new(block_size * sizeof(T),
                                      std::align_val_t{alignof(T)},
                                      std::nothrow);
        if (memory == nullptr) {
            return false;
        }

        // Add block to list
        blocks_.push_back({memory, block_size});
        capacity_ += block_size;

        // Add all objects in this block to free list
        for (std::size_t i = 0; i < block_size; ++i) {
            T* obj = reinterpret_cast<T*>(
                static_cast<char*>(memory) + i * sizeof(T));
            FreeNode* node = reinterpret_cast<FreeNode*>(obj);
            node->next = free_list_;
            free_list_ = node;
        }

        return true;
    }

    std::vector<Block> blocks_;     // List of allocated memory blocks
    std::size_t capacity_;           // Total capacity across all blocks
    std::size_t size_;               // Number of currently allocated objects
    FreeNode* free_list_;            // Head of free list
};

SLICK_DETAIL_NAMESPACE_END
