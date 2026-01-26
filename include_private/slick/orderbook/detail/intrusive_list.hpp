// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <cstddef>
#include <iterator>
#include <type_traits>

SLICK_DETAIL_NAMESPACE_BEGIN

/// Intrusive doubly-linked list for zero-allocation order management
///
/// This list requires elements to have `next` and `prev` pointer members.
/// Benefits:
/// - Zero allocations for list operations (O(1) insert/remove)
/// - Cache-friendly iteration (elements control their own layout)
/// - Elements can only be in one list at a time (intrusive design)
///
/// Usage:
/// @code
/// struct Order {
///     Order* next = nullptr;
///     Order* prev = nullptr;
///     // ... other fields
/// };
///
/// IntrusiveList<Order> orders;
/// orders.push_back(&my_order);
/// @endcode
///
/// @tparam T Element type (must have next/prev pointers)
/// @tparam NextPtr Pointer-to-member for next pointer
/// @tparam PrevPtr Pointer-to-member for prev pointer
template<typename T,
         T* T::*NextPtr = &T::next,
         T* T::*PrevPtr = &T::prev>
class IntrusiveList {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;

    /// Forward iterator for intrusive list
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        constexpr Iterator() noexcept : current_(nullptr) {}
        constexpr explicit Iterator(T* node) noexcept : current_(node) {}

        [[nodiscard]] constexpr reference operator*() const noexcept {
            return *current_;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept {
            return current_;
        }

        constexpr Iterator& operator++() noexcept {
            current_ = current_->*NextPtr;
            return *this;
        }

        constexpr Iterator operator++(int) noexcept {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        constexpr Iterator& operator--() noexcept {
            current_ = current_->*PrevPtr;
            return *this;
        }

        constexpr Iterator operator--(int) noexcept {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] constexpr bool operator==(const Iterator& other) const noexcept {
            return current_ == other.current_;
        }

        [[nodiscard]] constexpr bool operator!=(const Iterator& other) const noexcept {
            return !(*this == other);
        }

    private:
        T* current_;
    };

    /// Const iterator
    class ConstIterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        constexpr ConstIterator() noexcept : current_(nullptr) {}
        constexpr explicit ConstIterator(const T* node) noexcept : current_(node) {}
        constexpr ConstIterator(Iterator it) noexcept : current_(it.operator->()) {}

        [[nodiscard]] constexpr reference operator*() const noexcept {
            return *current_;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept {
            return current_;
        }

        constexpr ConstIterator& operator++() noexcept {
            current_ = current_->*NextPtr;
            return *this;
        }

        constexpr ConstIterator operator++(int) noexcept {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        constexpr ConstIterator& operator--() noexcept {
            current_ = current_->*PrevPtr;
            return *this;
        }

        constexpr ConstIterator operator--(int) noexcept {
            ConstIterator tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] constexpr bool operator==(const ConstIterator& other) const noexcept {
            return current_ == other.current_;
        }

        [[nodiscard]] constexpr bool operator!=(const ConstIterator& other) const noexcept {
            return !(*this == other);
        }

    private:
        const T* current_;
    };

    using iterator = Iterator;
    using const_iterator = ConstIterator;

    /// Constructor
    constexpr IntrusiveList() noexcept : head_(nullptr), tail_(nullptr), size_(0) {}

    /// Destructor - does not delete elements (intrusive design)
    ~IntrusiveList() noexcept = default;

    // Non-copyable (elements can only be in one list)
    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    // Movable
    constexpr IntrusiveList(IntrusiveList&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    constexpr IntrusiveList& operator=(IntrusiveList&& other) noexcept {
        if (this != &other) {
            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;
            other.head_ = nullptr;
            other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    /// Check if list is empty
    [[nodiscard]] constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    /// Get number of elements
    [[nodiscard]] constexpr size_type size() const noexcept {
        return size_;
    }

    /// Get front element
    [[nodiscard]] constexpr pointer front() noexcept {
        return head_;
    }

    [[nodiscard]] constexpr const_pointer front() const noexcept {
        return head_;
    }

    /// Get back element
    [[nodiscard]] constexpr pointer back() noexcept {
        return tail_;
    }

    [[nodiscard]] constexpr const_pointer back() const noexcept {
        return tail_;
    }

    /// Add element to front (newest order, used for LIFO if needed)
    constexpr void push_front(T* node) noexcept {
        node->*PrevPtr = nullptr;
        node->*NextPtr = head_;

        if (head_ != nullptr) {
            head_->*PrevPtr = node;
        } else {
            tail_ = node;
        }

        head_ = node;
        ++size_;
    }

    /// Add element to back (oldest order, FIFO time priority)
    constexpr void push_back(T* node) noexcept {
        node->*NextPtr = nullptr;
        node->*PrevPtr = tail_;

        if (tail_ != nullptr) {
            tail_->*NextPtr = node;
        } else {
            head_ = node;
        }

        tail_ = node;
        ++size_;
    }

    /// Remove front element
    constexpr pointer pop_front() noexcept {
        if (empty()) {
            return nullptr;
        }

        T* node = head_;
        head_ = head_->*NextPtr;

        if (head_ != nullptr) {
            head_->*PrevPtr = nullptr;
        } else {
            tail_ = nullptr;
        }

        node->*NextPtr = nullptr;
        node->*PrevPtr = nullptr;
        --size_;

        return node;
    }

    /// Remove back element
    constexpr pointer pop_back() noexcept {
        if (empty()) {
            return nullptr;
        }

        T* node = tail_;
        tail_ = tail_->*PrevPtr;

        if (tail_ != nullptr) {
            tail_->*NextPtr = nullptr;
        } else {
            head_ = nullptr;
        }

        node->*NextPtr = nullptr;
        node->*PrevPtr = nullptr;
        --size_;

        return node;
    }

    /// Remove specific element from list
    /// O(1) operation - no search required
    constexpr void erase(T* node) noexcept {
        if (node == nullptr) {
            return;
        }

        T* prev = node->*PrevPtr;
        T* next = node->*NextPtr;

        if (prev != nullptr) {
            prev->*NextPtr = next;
        } else {
            head_ = next;
        }

        if (next != nullptr) {
            next->*PrevPtr = prev;
        } else {
            tail_ = prev;
        }

        node->*NextPtr = nullptr;
        node->*PrevPtr = nullptr;
        --size_;
    }

    /// Insert element before given position
    /// O(1) operation
    constexpr void insert_before(T* pos, T* node) noexcept {
        if (pos == nullptr) {
            push_back(node);
            return;
        }

        if (pos == head_) {
            push_front(node);
            return;
        }

        T* prev = pos->*PrevPtr;

        node->*PrevPtr = prev;
        node->*NextPtr = pos;

        if (prev != nullptr) {
            prev->*NextPtr = node;
        }

        pos->*PrevPtr = node;
        ++size_;
    }

    /// Insert element after given position
    /// O(1) operation
    constexpr void insert_after(T* pos, T* node) noexcept {
        if (pos == nullptr) {
            push_front(node);
            return;
        }

        if (pos == tail_) {
            push_back(node);
            return;
        }

        T* next = pos->*NextPtr;

        node->*PrevPtr = pos;
        node->*NextPtr = next;

        if (next != nullptr) {
            next->*PrevPtr = node;
        }

        pos->*NextPtr = node;
        ++size_;
    }

    /// Clear list (does not delete elements)
    constexpr void clear() noexcept {
        // Unlink all elements
        T* current = head_;
        while (current != nullptr) {
            T* next = current->*NextPtr;
            current->*NextPtr = nullptr;
            current->*PrevPtr = nullptr;
            current = next;
        }

        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

    /// Iterators
    [[nodiscard]] constexpr iterator begin() noexcept {
        return iterator(head_);
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return const_iterator(head_);
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
        return const_iterator(head_);
    }

    [[nodiscard]] constexpr iterator end() noexcept {
        return iterator(nullptr);
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return const_iterator(nullptr);
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept {
        return const_iterator(nullptr);
    }

private:
    T* head_;
    T* tail_;
    size_type size_;
};

SLICK_DETAIL_NAMESPACE_END
