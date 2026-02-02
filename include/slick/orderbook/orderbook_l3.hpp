// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>
#include <slick/orderbook/events.hpp>
#include <slick/orderbook/observer.hpp>
#include <slick/orderbook/detail/price_level.hpp>
#include <slick/orderbook/detail/order.hpp>
#include <slick/orderbook/detail/order_map.hpp>
#include <slick/orderbook/detail/memory_pool.hpp>
#include <slick/orderbook/detail/flat_map.hpp>
#include <slick/orderbook/detail/intrusive_list.hpp>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <tuple>
#include <functional>

SLICK_NAMESPACE_BEGIN

/// Level 3 OrderBook - Individual order tracking with price-time priority
///
/// Maintains full order-by-order state with individual order visibility.
/// Provides automatic aggregated L2 view.
///
/// Features:
/// - O(1) order lookup by OrderId
/// - O(log n) price level operations
/// - Priority-based order queuing at each price level
/// - Automatic L2 aggregation from L3 data
/// - Observer pattern for notifications
/// - Zero-allocation order management via object pool
///
/// Thread Safety: Single writer, multiple readers (lock-free reads planned)
///
/// Usage:
/// @code
/// OrderBookL3 book(symbol_id);
/// book.addOrder(order_id, Side::Buy, 10000, 100, timestamp, priority);
/// book.modifyOrder(order_id, 10100, 150); // Update price and quantity
///
/// // Iterate L3 levels (zero-copy)
/// for (const auto& [price, level] : book.getLevelsL3(Side::Buy)) {
///     for (const auto& order : level.orders) {
///         // Process individual order...
///     }
/// }
///
/// // Get aggregated L2 view
/// auto l2_levels = book.getLevelsL2(Side::Buy, 10); // Top 10 levels
///
/// book.deleteOrder(order_id);
/// @endcode
class OrderBookL3 {
public:
    struct PriceComparator {
        using Comparator = std::function<bool(Price, Price)>;
        Comparator compare_;
        explicit PriceComparator(Side side)
            : compare_(side == Side::Buy
                ? Comparator{detail::BidComparator{}}
                : Comparator{detail::AskComparator{}}
            )
        {}
        bool operator()(Price a, Price b) const noexcept {
            return compare_(a, b);
        }
    };
    using PriceLevelMap = detail::FlatMap<Price, detail::PriceLevelL3, PriceComparator>;

    /// Constructor
    /// @param symbol Symbol identifier
    /// @param initial_order_capacity Initial capacity for order pool
    /// @param initial_level_capacity Initial capacity for price levels per side
    explicit OrderBookL3(SymbolId symbol,
                        std::size_t initial_order_capacity = 1024,
                        std::size_t initial_level_capacity = 32);

    /// Destructor
    ~OrderBookL3();

    // Non-copyable, movable
    OrderBookL3(const OrderBookL3&) = delete;
    OrderBookL3& operator=(const OrderBookL3&) = delete;
    OrderBookL3(OrderBookL3&&) noexcept = default;
    OrderBookL3& operator=(OrderBookL3&&) noexcept = default;

    /// Get symbol ID
    [[nodiscard]] SymbolId symbol() const noexcept { return symbol_; }

    /// Add a new order or modify existing order if OrderId already exists
    /// If the order already exists, modifies its price and/or quantity
    /// @param order_id Unique order identifier
    /// @param side Buy or Sell (must match existing order's side if it exists)
    /// @param price Order price
    /// @param quantity Order quantity
    /// @param timestamp Order timestamp
    /// @param priority Priority for sorting (used only when adding new order)
    /// @param is_last_in_batch Set to true if this is the last update in an external batch
    ///                         (enables batching of TopOfBook updates)
    /// @return true if order was added or modified successfully
    bool addOrModifyOrder(OrderId order_id, Side side, Price price, Quantity quantity,
                          Timestamp timestamp, uint64_t priority,
                          bool is_last_in_batch = true);

    /// Add or modify order with timestamp as priority (convenience)
    bool addOrModifyOrder(OrderId order_id, Side side, Price price, Quantity quantity, Timestamp timestamp,
                          bool is_last_in_batch = true) {
        return addOrModifyOrder(order_id, side, price, quantity, timestamp, timestamp, is_last_in_batch);
    }

    /// Add a new order (strict - fails if order already exists)
    /// @param order_id Unique order identifier
    /// @param side Buy or Sell
    /// @param price Order price
    /// @param quantity Order quantity
    /// @param timestamp Order timestamp
    /// @param priority Priority for sorting (defaults to timestamp)
    /// @param is_last_in_batch Set to true if this is the last update in an external batch
    ///                         (enables batching of TopOfBook updates)
    /// @return true if order was added successfully, false if OrderId already exists
    bool addOrder(OrderId order_id, Side side, Price price, Quantity quantity,
                  Timestamp timestamp, uint64_t priority = 0, bool is_last_in_batch = true);

    /// Modify an existing order's price and/or quantity
    /// Compares new values with old to determine what changed
    /// @param order_id Order identifier
    /// @param new_price New price
    /// @param new_quantity New quantity (0 = delete)
    /// @param is_last_in_batch Set to true if this is the last update in an external batch
    ///                         (enables batching of TopOfBook updates)
    /// @return true if order was found and modified
    bool modifyOrder(OrderId order_id, Price new_price, Quantity new_quantity,
                     bool is_last_in_batch = true);

    /// Delete an order
    /// @param order_id Order identifier
    /// @param is_last_in_batch Set to true if this is the last update in an external batch
    ///                         (enables batching of TopOfBook updates)
    /// @return true if order was found and deleted
    bool deleteOrder(OrderId order_id, bool is_last_in_batch = true) noexcept;

    /// Execute (partially fill) an order
    /// @param order_id Order identifier
    /// @param executed_quantity Quantity executed
    /// @param is_last_in_batch Set to true if this is the last update in an external batch
    ///                         (enables batching of TopOfBook updates)
    /// @return true if order was found and executed
    bool executeOrder(OrderId order_id, Quantity executed_quantity,
                      bool is_last_in_batch = true);

    /// Find order by OrderId
    /// @param order_id Order identifier
    /// @return Pointer to order, or nullptr if not found
    [[nodiscard]] const detail::Order* findOrder(OrderId order_id) const noexcept;

    /// Get best bid (highest buy price)
    /// @return Pointer to best bid level, or nullptr if no bids
    [[nodiscard]] const detail::PriceLevelL3* getBestBid() const noexcept;

    /// Get best ask (lowest sell price)
    /// @return Pointer to best ask level, or nullptr if no asks
    [[nodiscard]] const detail::PriceLevelL3* getBestAsk() const noexcept;

    /// Get top-of-book snapshot (aggregated L2 view)
    /// @return TopOfBook structure with best bid/ask
    [[nodiscard]] TopOfBook getTopOfBook() const noexcept;

    /// Get L3 price levels for a side (zero-copy access to full order data)
    /// Returns reference to flat_map, iterate with range-for
    /// @param side Buy or Sell
    /// @return Reference to L3 price level map
    [[nodiscard]] const PriceLevelMap& getLevelsL3(Side side) const noexcept {
        SLICK_ASSERT(side < SideCount);
        return levels_[side];
    }

    /// Get aggregated L2 price levels for a side
    /// Converts L3 data to L2 by aggregating quantities at each price
    /// @param side Buy or Sell
    /// @param depth Maximum number of levels (0 = all)
    /// @return Vector of aggregated L2 price levels
    [[nodiscard]] std::vector<detail::PriceLevelL2> getLevelsL2(Side side, std::size_t depth = 0) const;

    /// Get L3 price level by price
    /// Access the orders at this price via level->orders (IntrusiveList)
    /// @param side Buy or Sell
    /// @param price Price level
    /// @return Pair of pointer to L3 price level (or nullptr if not found) and level index
    [[nodiscard]] std::pair<const detail::PriceLevelL3*, uint16_t> getLevel(Side side, Price price) const noexcept;

    [[nodiscard]] std::pair<detail::PriceLevelL3*, uint16_t> getLevel(Side side, Price price) noexcept;

    /// Get number of price levels on a side
    /// @param side Buy or Sell
    /// @return Number of levels
    [[nodiscard]] std::size_t levelCount(Side side) const noexcept;

    /// Get total number of orders
    /// @return Total order count
    [[nodiscard]] std::size_t orderCount() const noexcept { return order_map_.size(); }

    /// Get number of orders on a side
    /// @param side Buy or Sell
    /// @return Number of orders on this side
    [[nodiscard]] std::size_t orderCount(Side side) const noexcept;

    /// Check if a side is empty
    /// @param side Buy or Sell
    /// @return true if side has no orders
    [[nodiscard]] bool isEmpty(Side side) const noexcept;

    /// Check if book is empty (both sides)
    /// @return true if both sides are empty
    [[nodiscard]] bool isEmpty() const noexcept;

    /// Clear all orders on one side
    /// @param side Buy or Sell
    void clearSide(Side side) noexcept;

    /// Clear all orders (both sides)
    void clear() noexcept;

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

    /// Emit complete orderbook snapshot to observers (L3 MBO)
    /// Useful for replaying full book state to a new observer
    /// Calls onSnapshotBegin(), followed by onOrderUpdate() for each order, then onSnapshotEnd()
    /// @param timestamp Snapshot timestamp
    void emitSnapshot(Timestamp timestamp);

private:
    /// Get or create price level, returns pointer, index and if new level created
    std::tuple<detail::PriceLevelL3*, uint16_t, bool> getOrCreateLevel(Side side, Price price);

    /// Remove price level if empty
    bool removeLevelIfEmpty(Side side, Price price) noexcept;

    /// Notify observers of order update with level index and change flags
    void notifyOrderUpdate(const detail::Order* order, Quantity old_quantity, Price old_price,
                          Timestamp timestamp, uint16_t level_index, uint8_t change_flags) const;

    /// Notify observers of order delete with level index
    void notifyOrderDelete(const detail::Order* order, Timestamp timestamp, uint16_t level_index, uint8_t change_flags) const;

    /// Calculate price level index for a given side and price
    [[nodiscard]] uint16_t calculateLevelIndex(Side side, Price price) const noexcept;

    /// Notify observers of trade
    void notifyTrade(OrderId passive_order_id, OrderId aggressive_order_id,
                     Side aggressor_side, Price price, Quantity quantity, Timestamp timestamp) const;

    /// Notify observers of price level update (L2 aggregated view)
    void notifyPriceLevelUpdate(Side side, Price price, Quantity total_quantity,  Timestamp timestamp, uint16_t level_index, uint8_t change_flags) const;

    /// Notify observers of top-of-book update if best changed
    /// Updates cached_tob_ after notification
    /// @param timestamp Update timestamp
    /// @param update_flags Change flags from the update (used to check LastInBatch)
    void notifyTopOfBookIfChanged(Timestamp timestamp, uint8_t update_flags);

    SymbolId symbol_;                                           // Symbol identifier
    std::array<PriceLevelMap, SideCount> levels_;              // Price levels per side (indexed by Side enum)
    detail::OrderMap order_map_;                                // OrderId -> Order* lookup
    detail::ObjectPool<detail::Order> order_pool_;             // Memory pool for Order objects
    ObserverManager observers_;                                 // Observer notifications
    TopOfBook cached_tob_;                                      // Cached top-of-book for efficient change detection
};

SLICK_NAMESPACE_END

// Include implementation for header-only mode
#ifdef SLICK_ORDERBOOK_HEADER_ONLY
#include <slick/orderbook/detail/impl/orderbook_l3_impl.hpp>
#endif

