// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>
#include <slick/orderbook/detail/intrusive_list.hpp>
#include <slick/orderbook/detail/order.hpp>

SLICK_DETAIL_NAMESPACE_BEGIN

/// Price level for Level 2 orderbook (aggregated)
/// Stores total quantity at a specific price point
struct PriceLevelL2 {
    Price price;          // Price level
    Quantity quantity;    // Total quantity at this price
    Timestamp timestamp;  // Last update timestamp

    constexpr PriceLevelL2() noexcept
        : price(0), quantity(0), timestamp(0) {}

    constexpr PriceLevelL2(Price p, Quantity q, Timestamp ts) noexcept
        : price(p), quantity(q), timestamp(ts) {}

    /// Check if this level is empty
    [[nodiscard]] constexpr bool isEmpty() const noexcept {
        return quantity == 0;
    }

    /// Comparison operators for sorting
    [[nodiscard]] constexpr bool operator==(const PriceLevelL2& other) const noexcept {
        return price == other.price;
    }

    [[nodiscard]] constexpr bool operator!=(const PriceLevelL2& other) const noexcept {
        return price != other.price;
    }

    [[nodiscard]] constexpr bool operator<(const PriceLevelL2& other) const noexcept {
        return price < other.price;
    }

    [[nodiscard]] constexpr bool operator>(const PriceLevelL2& other) const noexcept {
        return price > other.price;
    }

    [[nodiscard]] constexpr bool operator<=(const PriceLevelL2& other) const noexcept {
        return price <= other.price;
    }

    [[nodiscard]] constexpr bool operator>=(const PriceLevelL2& other) const noexcept {
        return price >= other.price;
    }
};

/// Price level for Level 3 orderbook (order-by-order)
/// Maintains a queue of individual orders at this price level
struct PriceLevelL3 {
    Price price;                    // Price level
    IntrusiveList<Order> orders;    // Orders at this price (sorted by priority)
    Quantity total_quantity;        // Cached total quantity for fast L2 aggregation

    explicit PriceLevelL3(Price p) noexcept
        : price(p), orders(), total_quantity(0) {}

    /// Get total quantity at this level
    [[nodiscard]] Quantity getTotalQuantity() const noexcept {
        return total_quantity;
    }

    /// Get number of orders at this level
    [[nodiscard]] std::size_t orderCount() const noexcept {
        return orders.size();
    }

    /// Check if this level is empty
    [[nodiscard]] bool isEmpty() const noexcept {
        return orders.empty();
    }

    /// Get best order (first in queue)
    [[nodiscard]] Order* getBestOrder() noexcept {
        return orders.empty() ? nullptr : orders.front();
    }

    [[nodiscard]] const Order* getBestOrder() const noexcept {
        return orders.empty() ? nullptr : orders.front();
    }

    /// Insert order maintaining priority order
    void insertOrder(Order* order) noexcept {
        // Insert in priority order (lower priority value = higher priority)
        auto it = orders.begin();
        while (it != orders.end() && it->priority <= order->priority) {
            ++it;
        }

        if (it == orders.end()) {
            orders.push_back(order);
        } else {
            orders.insert_before(&(*it), order);
        }

        total_quantity += order->quantity;
    }

    /// Remove order from the level
    void removeOrder(Order* order) noexcept {
        total_quantity -= order->quantity;
        orders.erase(order);
    }

    /// Update order quantity (for modify operations)
    void updateOrderQuantity(Quantity old_quantity, Quantity new_quantity) noexcept {
        total_quantity = total_quantity - old_quantity + new_quantity;
    }

    /// Comparison operators for sorting by price
    [[nodiscard]] constexpr bool operator==(const PriceLevelL3& other) const noexcept {
        return price == other.price;
    }

    [[nodiscard]] constexpr bool operator!=(const PriceLevelL3& other) const noexcept {
        return price != other.price;
    }

    [[nodiscard]] constexpr bool operator<(const PriceLevelL3& other) const noexcept {
        return price < other.price;
    }

    [[nodiscard]] constexpr bool operator>(const PriceLevelL3& other) const noexcept {
        return price > other.price;
    }
};

/// Comparator for sorting bid levels (descending price)
struct BidComparator {
    [[nodiscard]] constexpr bool operator()(const PriceLevelL2& a, const PriceLevelL2& b) const noexcept {
        return a.price > b.price;  // Descending: highest bid first
    }

    [[nodiscard]] constexpr bool operator()(Price a, const PriceLevelL2& b) const noexcept {
        return a > b.price;
    }

    [[nodiscard]] constexpr bool operator()(const PriceLevelL2& a, Price b) const noexcept {
        return a.price > b;
    }

    [[nodiscard]] constexpr bool operator()(const PriceLevelL3& a, const PriceLevelL3& b) const noexcept {
        return a.price > b.price;   // Descending: highest bid first
    }

    [[nodiscard]] constexpr bool operator()(Price a, const PriceLevelL3& b) const noexcept {
        return a > b.price;
    }

    [[nodiscard]] constexpr bool operator()(const PriceLevelL3& a, Price b) const noexcept {
        return a.price > b;
    }

    [[nodiscard]] constexpr bool operator()(Price a, Price b) const noexcept {
        return a > b;
    }
};

/// Comparator for sorting ask levels (ascending price)
struct AskComparator {
    [[nodiscard]] constexpr bool operator()(const PriceLevelL2& a, const PriceLevelL2& b) const noexcept {
        return a.price < b.price;  // Ascending: lowest ask first
    }

    [[nodiscard]] constexpr bool operator()(Price a, const PriceLevelL2& b) const noexcept {
        return a < b.price;
    }

    [[nodiscard]] constexpr bool operator()(const PriceLevelL2& a, Price b) const noexcept {
        return a.price < b;
    }

    [[nodiscard]] constexpr bool operator()(const PriceLevelL3& a, const PriceLevelL3& b) const noexcept {
        return a.price < b.price;  // Ascending: lowest ask first
    }

    [[nodiscard]] constexpr bool operator()(Price a, const PriceLevelL3& b) const noexcept {
        return a < b.price;
    }

    [[nodiscard]] constexpr bool operator()(const PriceLevelL3& a, Price b) const noexcept {
        return a.price < b;
    }

    [[nodiscard]] constexpr bool operator()(Price a, Price b) const noexcept {
        return a < b;
    }
};

SLICK_DETAIL_NAMESPACE_END
