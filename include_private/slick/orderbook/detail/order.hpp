// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>

SLICK_DETAIL_NAMESPACE_BEGIN

/// Order structure for Level 3 orderbook
/// Uses intrusive list design for zero-allocation order queues at each price level
struct Order {
    OrderId order_id;           // Unique order identifier
    Price price;                // Order price
    Quantity quantity;          // Order quantity
    Side side;                  // Buy or Sell
    Timestamp timestamp;        // Order timestamp
    uint64_t priority;          // Priority for sorting at same price level (lower = higher priority)

    // Intrusive list pointers (managed by IntrusiveList)
    Order* prev;
    Order* next;

    /// Constructor with all fields
    Order(OrderId id, Price p, Quantity qty, Side s, Timestamp ts, uint64_t prio) noexcept
        : order_id(id),
          price(p),
          quantity(qty),
          side(s),
          timestamp(ts),
          priority(prio),
          prev(nullptr),
          next(nullptr) {
    }

    /// Constructor with timestamp as priority (common case)
    Order(OrderId id, Price p, Quantity qty, Side s, Timestamp ts) noexcept
        : Order(id, p, qty, s, ts, ts) {  // Use timestamp as priority by default
    }

    /// Default constructor (for pool allocation)
    Order() noexcept
        : order_id(0),
          price(0),
          quantity(0),
          side(Side::Buy),
          timestamp(0),
          priority(0),
          prev(nullptr),
          next(nullptr) {
    }

    /// Comparison operator for priority-based sorting
    /// Lower priority value = higher priority in queue
    [[nodiscard]] bool operator<(const Order& other) const noexcept {
        return priority < other.priority;
    }

    [[nodiscard]] bool operator>(const Order& other) const noexcept {
        return priority > other.priority;
    }
};

SLICK_DETAIL_NAMESPACE_END
