// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>

SLICK_NAMESPACE_BEGIN

/// Change flags for price level and order updates (regular enum for bitwise operations)
enum PriceLevelChangeFlag : uint8_t {
    PriceChanged = 0x01,    // Price level was added/moved
    QuantityChanged = 0x02, // Quantity at level changed
};

/// Level 2 price level update event
/// Note: quantity = 0 implies the price level should be deleted
struct PriceLevelUpdate {
    SymbolId symbol;        // Symbol identifier
    Side side;              // Buy or Sell
    Price price;            // Price level
    Quantity quantity;      // New total quantity at this level (0 = delete)
    Timestamp timestamp;    // Update timestamp
    uint16_t level_index;   // 0-based index in sorted order (0 = best, 1 = second best, etc.)
    uint8_t change_flags;   // Bitset of PriceLevelChangeFlag

    PriceLevelUpdate() noexcept = default;

    PriceLevelUpdate(SymbolId sym, Side s, Price p, Quantity q, Timestamp ts,
                     uint16_t idx = 0, uint8_t flags = 0) noexcept
        : symbol(sym), side(s), price(p), quantity(q), timestamp(ts),
          level_index(idx), change_flags(flags) {}

    /// Check if this is a delete action
    [[nodiscard]] constexpr bool isDelete() const noexcept {
        return quantity == 0;
    }

    /// Check if price changed (level was added or moved)
    [[nodiscard]] constexpr bool priceChanged() const noexcept {
        return (change_flags & PriceChanged) != 0;
    }

    /// Check if quantity changed
    [[nodiscard]] constexpr bool quantityChanged() const noexcept {
        return (change_flags & QuantityChanged) != 0;
    }

    /// Check if this update affects the top N levels
    [[nodiscard]] constexpr bool isTopN(uint16_t n) const noexcept {
        return level_index < n;
    }
};

/// Level 3 order update event
/// Note: quantity = 0 implies the order should be deleted
struct OrderUpdate {
    SymbolId symbol;            // Symbol identifier
    OrderId order_id;           // Unique order identifier
    Side side;                  // Buy or Sell
    Price price;                // Order price
    Quantity quantity;          // Order quantity (0 = delete)
    Timestamp timestamp;        // Update timestamp
    uint16_t price_level_index; // Index of the price level this order belongs to
    uint64_t priority;          // Order priority (typically timestamp or sequence number)
    uint8_t change_flags;       // Bitset of PriceLevelChangeFlag

    OrderUpdate() noexcept = default;

    OrderUpdate(SymbolId sym, OrderId id, Side s, Price p, Quantity q, Timestamp ts,
                uint16_t idx = 0, uint64_t prio = 0, uint8_t flags = 0) noexcept
        : symbol(sym), order_id(id), side(s), price(p), quantity(q), timestamp(ts),
          price_level_index(idx), priority(prio), change_flags(flags) {}

    /// Check if this is a delete action
    [[nodiscard]] constexpr bool isDelete() const noexcept {
        return quantity == 0;
    }

    /// Check if price changed (order was added or moved to different price)
    [[nodiscard]] constexpr bool priceChanged() const noexcept {
        return (change_flags & PriceChanged) != 0;
    }

    /// Check if quantity changed
    [[nodiscard]] constexpr bool quantityChanged() const noexcept {
        return (change_flags & QuantityChanged) != 0;
    }

    /// Check if this order's price level is in the top N levels
    [[nodiscard]] constexpr bool isTopN(uint16_t n) const noexcept {
        return price_level_index < n;
    }
};

/// Trade event - represents an executed trade
struct Trade {
    SymbolId symbol;                // Symbol identifier
    Price price;                    // Trade price
    Quantity quantity;              // Trade quantity
    Timestamp timestamp;            // Trade timestamp
    OrderId aggressive_order_id;    // OrderId of the aggressive (incoming) order
    OrderId passive_order_id;       // OrderId of the passive (resting) order
    Side aggressor_side;            // Side that initiated the trade (Buy = uptick, Sell = downtick)

    Trade() noexcept = default;

    Trade(SymbolId sym, Price p, Quantity q, Timestamp ts, Side aggressor, OrderId passive = 0, OrderId aggressive = 0) noexcept
        : symbol(sym), price(p), quantity(q), timestamp(ts), aggressive_order_id(aggressive), passive_order_id(passive), aggressor_side(aggressor) {}
};

/// Top-of-book snapshot - best bid and ask
struct TopOfBook {
    SymbolId symbol;        // Symbol identifier
    Price best_bid;         // Best bid price (highest buy price)
    Quantity bid_quantity;  // Total quantity at best bid
    Price best_ask;         // Best ask price (lowest sell price)
    Quantity ask_quantity;  // Total quantity at best ask
    Timestamp timestamp;    // Snapshot timestamp

    TopOfBook() noexcept
        : symbol(0), best_bid(0), bid_quantity(0), best_ask(0), ask_quantity(0), timestamp(0) {}

    TopOfBook(SymbolId sym, Price bid, Quantity bid_qty, Price ask, Quantity ask_qty, Timestamp ts) noexcept
        : symbol(sym), best_bid(bid), bid_quantity(bid_qty), best_ask(ask), ask_quantity(ask_qty), timestamp(ts) {}

    /// Calculate spread
    [[nodiscard]] Price spread() const noexcept {
        return (best_ask > best_bid) ? (best_ask - best_bid) : 0;
    }

    /// Calculate mid price
    [[nodiscard]] Price midPrice() const noexcept {
        return (best_bid + best_ask) / 2;
    }

    /// Check if book is crossed (bid >= ask, should not happen in normal conditions)
    [[nodiscard]] bool isCrossed() const noexcept {
        return best_bid >= best_ask && best_bid > 0 && best_ask > 0;
    }

    /// Check if book has valid bid
    [[nodiscard]] bool hasBid() const noexcept {
        return best_bid > 0 && bid_quantity > 0;
    }

    /// Check if book has valid ask
    [[nodiscard]] bool hasAsk() const noexcept {
        return best_ask > 0 && ask_quantity > 0;
    }

    /// Check if book has both valid bid and ask
    [[nodiscard]] bool isValid() const noexcept {
        return hasBid() && hasAsk();
    }
};

SLICK_NAMESPACE_END
