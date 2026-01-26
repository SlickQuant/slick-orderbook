// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>
#include <slick/orderbook/events.hpp>
#include <slick/orderbook/observer.hpp>
#include <slick/orderbook/detail/price_level.hpp>
#include <slick/orderbook/detail/level_container.hpp>
#include <memory>
#include <vector>
#include <array>

SLICK_NAMESPACE_BEGIN

/// Level 2 OrderBook - aggregated price levels
///
/// Maintains aggregated order book state with price levels and total quantities.
/// Does not track individual orders.
///
/// Features:
/// - O(log n) updates (binary search in sorted vector)
/// - O(1) best bid/ask queries
/// - Cache-friendly memory layout
/// - Observer pattern for notifications
///
/// Thread Safety: Single writer, multiple readers (lock-free reads planned)
///
/// Usage:
/// @code
/// OrderBookL2 book(symbol_id);
/// book.updateLevel(Side::Buy, 10000, 100, timestamp);
/// auto best_bid = book.getBestBid();
/// @endcode
class OrderBookL2 {
public:
    /// Constructor
    /// @param symbol Symbol identifier
    /// @param initial_capacity Initial capacity for price levels per side
    explicit OrderBookL2(SymbolId symbol, std::size_t initial_capacity = 32);

    /// Destructor
    ~OrderBookL2() = default;

    // Non-copyable, movable
    OrderBookL2(const OrderBookL2&) = delete;
    OrderBookL2& operator=(const OrderBookL2&) = delete;
    OrderBookL2(OrderBookL2&&) noexcept = default;
    OrderBookL2& operator=(OrderBookL2&&) noexcept = default;

    /// Get symbol ID
    [[nodiscard]] SymbolId symbol() const noexcept { return symbol_; }

    /// Add or update a price level
    /// If quantity is 0, the level is deleted
    /// @param side Buy or Sell
    /// @param price Price level
    /// @param quantity Total quantity at this level (0 = delete)
    /// @param timestamp Update timestamp
    void updateLevel(Side side, Price price, Quantity quantity, Timestamp timestamp);

    /// Delete a price level
    /// @param side Buy or Sell
    /// @param price Price level to delete
    /// @return true if level was found and deleted
    bool deleteLevel(Side side, Price price) noexcept;

    /// Clear all levels on one side
    /// @param side Buy or Sell
    void clearSide(Side side) noexcept;

    /// Clear all levels (both sides)
    void clear() noexcept;

    /// Get best bid (highest buy price)
    /// @return Pointer to best bid level, or nullptr if no bids
    [[nodiscard]] const detail::PriceLevelL2* getBestBid() const noexcept;

    /// Get best ask (lowest sell price)
    /// @return Pointer to best ask level, or nullptr if no asks
    [[nodiscard]] const detail::PriceLevelL2* getBestAsk() const noexcept;

    /// Get top-of-book snapshot
    /// @return TopOfBook structure with best bid/ask
    [[nodiscard]] TopOfBook getTopOfBook() const noexcept;

    /// Get price levels for a side
    /// @param side Buy or Sell
    /// @param depth Maximum number of levels (0 = all)
    /// @return Vector of price levels
    [[nodiscard]] std::vector<detail::PriceLevelL2> getLevels(Side side, std::size_t depth = 0) const;

    /// Get number of levels on a side
    /// @param side Buy or Sell
    /// @return Number of levels
    [[nodiscard]] std::size_t levelCount(Side side) const noexcept;

    /// Check if a side is empty
    /// @param side Buy or Sell
    /// @return true if side has no levels
    [[nodiscard]] bool isEmpty(Side side) const noexcept;

    /// Check if book is empty (both sides)
    /// @return true if both sides are empty
    [[nodiscard]] bool isEmpty() const noexcept;

    /// Observer management
    void addObserver(std::shared_ptr<IOrderBookObserver> observer) {
        observers_.addObserver(std::move(observer));
    }

    bool removeObserver(const std::shared_ptr<IOrderBookObserver>& observer) {
        return observers_.removeObserver(observer);
    }

    void clearObservers() noexcept {
        observers_.clearObservers();
    }

    [[nodiscard]] std::size_t observerCount() const noexcept {
        return observers_.observerCount();
    }

private:
    /// Notify observers of price level update
    void notifyPriceLevelUpdate(Side side, Price price, Quantity quantity, Timestamp timestamp) const;

    /// Notify observers of top-of-book update if best changed
    void notifyTopOfBookIfChanged(const TopOfBook& old_tob, Timestamp timestamp) const;

    SymbolId symbol_;                                                    // Symbol identifier
    std::array<detail::LevelContainer, SideCount> sides_;              // Bid and ask sides (indexed by Side enum)
    ObserverManager observers_;                                          // Observer notifications
};

SLICK_NAMESPACE_END
