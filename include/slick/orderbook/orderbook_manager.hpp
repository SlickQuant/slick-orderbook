// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>
#include <slick/orderbook/orderbook_l2.hpp>
#include <slick/orderbook/orderbook_l3.hpp>
#include <slick/orderbook/detail/flat_map.hpp>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

SLICK_NAMESPACE_BEGIN

/// Multi-symbol OrderBook Manager
///
/// Manages multiple orderbook instances (L2 or L3) across different symbols.
/// Thread-safe for concurrent access to different symbols.
///
/// Features:
/// - Thread-safe symbol registry using shared_mutex
/// - Per-symbol orderbook isolation (no cross-symbol locking)
/// - Automatic orderbook creation on first access
/// - Efficient symbol lookup via flat_map
/// - Support for both L2 and L3 orderbooks via template
///
/// Thread Safety:
/// - Multiple threads can simultaneously access different symbols (lock-free)
/// - Symbol map protected by shared_mutex (read-write lock)
/// - Each orderbook follows single-writer-per-symbol model
///
/// Template Parameter:
/// @tparam OrderBookT Either OrderBookL2 or OrderBookL3
///
/// Usage:
/// @code
/// // L2 Manager
/// OrderBookManager<OrderBookL2> l2_manager;
/// auto* book = l2_manager.getOrCreateOrderBook(symbol_id);
/// book->updateLevel(Side::Buy, 10000, 100, timestamp);
///
/// // L3 Manager
/// OrderBookManager<OrderBookL3> l3_manager;
/// auto* book = l3_manager.getOrCreateOrderBook(symbol_id);
/// book->addOrder(order_id, Side::Buy, 10000, 100, timestamp);
/// @endcode
template<typename OrderBookT>
class OrderBookManager {
public:
    using OrderBookPtr = std::unique_ptr<OrderBookT>;
    using SymbolMap = detail::FlatMap<SymbolId, OrderBookPtr>;

    /// Constructor
    /// @param initial_symbol_capacity Initial capacity for symbol map
    explicit OrderBookManager(std::size_t initial_symbol_capacity = 64);

    /// Destructor
    ~OrderBookManager() = default;

    // Non-copyable, movable
    OrderBookManager(const OrderBookManager&) = delete;
    OrderBookManager& operator=(const OrderBookManager&) = delete;
    OrderBookManager(OrderBookManager&&) noexcept = default;
    OrderBookManager& operator=(OrderBookManager&&) noexcept = default;

    /// Get existing orderbook or create new one if it doesn't exist
    /// Thread-safe: Uses shared_mutex for symbol map access
    /// @param symbol Symbol identifier
    /// @return Pointer to orderbook (never null)
    [[nodiscard]] OrderBookT* getOrCreateOrderBook(SymbolId symbol);

    /// Get existing orderbook (read-only access)
    /// Thread-safe: Uses shared lock for read-only access
    /// @param symbol Symbol identifier
    /// @return Pointer to orderbook, or nullptr if symbol doesn't exist
    [[nodiscard]] const OrderBookT* getOrderBook(SymbolId symbol) const;

    /// Get existing orderbook (mutable access)
    /// Thread-safe: Uses shared lock for read-only access to map
    /// @param symbol Symbol identifier
    /// @return Pointer to orderbook, or nullptr if symbol doesn't exist
    [[nodiscard]] OrderBookT* getOrderBook(SymbolId symbol);

    /// Check if symbol exists in manager
    /// Thread-safe: Uses shared lock
    /// @param symbol Symbol identifier
    /// @return true if orderbook exists for this symbol
    [[nodiscard]] bool hasSymbol(SymbolId symbol) const;

    /// Remove orderbook for a symbol
    /// Thread-safe: Uses exclusive lock
    /// @param symbol Symbol identifier
    /// @return true if symbol was found and removed
    bool removeOrderBook(SymbolId symbol);

    /// Get all active symbol IDs
    /// Thread-safe: Uses shared lock
    /// @return Vector of symbol IDs (copy for thread safety)
    [[nodiscard]] std::vector<SymbolId> getSymbols() const;

    /// Get number of active symbols
    /// Thread-safe: Uses shared lock
    /// @return Number of orderbooks being managed
    [[nodiscard]] std::size_t symbolCount() const;

    /// Clear all orderbooks
    /// Thread-safe: Uses exclusive lock
    void clear();

    /// Reserve capacity for expected number of symbols
    /// Not thread-safe: Should be called before concurrent access begins
    /// @param capacity Expected number of symbols
    void reserve(std::size_t capacity);

private:
    mutable std::shared_mutex mutex_;  ///< Protects symbol_map_
    SymbolMap symbol_map_;             ///< Map of SymbolId -> OrderBook
};

SLICK_NAMESPACE_END

// Include implementation for header-only mode
#ifdef SLICK_ORDERBOOK_HEADER_ONLY
#include <slick/orderbook/detail/impl/orderbook_manager_impl.hpp>
#endif
