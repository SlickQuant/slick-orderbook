// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>
#include <slick/orderbook/detail/order.hpp>
#include <unordered_map>

SLICK_DETAIL_NAMESPACE_BEGIN

/// Hash table for fast OrderId -> Order* lookup
/// Uses std::unordered_map for simplicity and performance
/// Provides O(1) average-case lookup, insertion, and deletion
class OrderMap {
public:
    using MapType = std::unordered_map<OrderId, Order*>;
    using iterator = MapType::iterator;
    using const_iterator = MapType::const_iterator;

    /// Constructor with initial capacity
    explicit OrderMap(std::size_t initial_capacity = 1024) {
        map_.reserve(initial_capacity);
    }

    /// Insert order into map
    /// @param order Pointer to order (must not be null)
    /// @return true if insertion succeeded, false if OrderId already exists
    bool insert(Order* order) noexcept {
        SLICK_ASSERT(order != nullptr);
        // Avoid structured bindings to work around C++23 tuple conversion issues with Clang 17 + GCC 14 libstdc++
        auto result = map_.try_emplace(order->order_id, order);
        return result.second;
    }

    /// Remove order from map
    /// @param order_id OrderId to remove
    /// @return true if order was found and removed
    bool erase(OrderId order_id) noexcept {
        return map_.erase(order_id) > 0;
    }

    /// Find order by OrderId
    /// @param order_id OrderId to find
    /// @return Pointer to order, or nullptr if not found
    [[nodiscard]] Order* find(OrderId order_id) noexcept {
        auto it = map_.find(order_id);
        return (it != map_.end()) ? it->second : nullptr;
    }

    [[nodiscard]] const Order* find(OrderId order_id) const noexcept {
        auto it = map_.find(order_id);
        return (it != map_.end()) ? it->second : nullptr;
    }

    /// Check if order exists
    [[nodiscard]] bool contains(OrderId order_id) const noexcept {
        return map_.contains(order_id);
    }

    /// Get number of orders in map
    [[nodiscard]] std::size_t size() const noexcept {
        return map_.size();
    }

    /// Check if map is empty
    [[nodiscard]] bool empty() const noexcept {
        return map_.empty();
    }

    /// Reserve capacity for orders
    void reserve(std::size_t capacity) {
        map_.reserve(capacity);
    }

    /// Clear all orders
    void clear() noexcept {
        map_.clear();
    }

    /// Iterators
    [[nodiscard]] iterator begin() noexcept { return map_.begin(); }
    [[nodiscard]] iterator end() noexcept { return map_.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return map_.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return map_.end(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return map_.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return map_.cend(); }

private:
    MapType map_;  // OrderId -> Order* mapping
};

SLICK_DETAIL_NAMESPACE_END
