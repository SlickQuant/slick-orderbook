// Copyright 2026 Slick Quant
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
#include <mutex>
#include <utility>

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
/// Orderbook Lifetime:
/// - Raw pointers returned by getOrCreateOrderBook()/getOrderBook() are NOT retained by the
///   manager lock. They are invalidated by removeOrderBook()/clear() for that symbol.
///   Only use them when no thread can remove the symbol while the pointer is in use
///   (e.g. symbols are removed only after all feed threads have stopped).
/// - If symbols may be removed concurrently, use getOrCreateSharedOrderBook()/getSharedOrderBook():
///   the returned shared_ptr keeps the orderbook alive after removal from the manager.
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
    using OrderBookPtr = std::shared_ptr<OrderBookT>;
    using SymbolMap = detail::FlatMap<SymbolId, OrderBookPtr>;

    /// Constructor
    /// @param initial_symbol_capacity Initial capacity for symbol map
    explicit OrderBookManager(std::size_t initial_symbol_capacity = 64);

    /// Destructor
    ~OrderBookManager() = default;

    // Non-copyable, movable (manual implementation: std::shared_mutex is neither copyable nor movable)
    // Moving transfers the orderbooks under the source's lock; the moved-from manager is left empty.
    // Lifetime on move:
    // - Orderbooks of the source are transferred, not copied: pointers/handles obtained from the
    //   source stay valid and now refer to orderbooks owned by the destination.
    // - Move assignment releases the destination's previous orderbooks, as clear() does: raw pointers
    //   obtained from the destination before the assignment are invalidated, unless a shared handle
    //   (getSharedOrderBook()/getOrCreateSharedOrderBook()) still retains that orderbook.
    OrderBookManager(const OrderBookManager&) = delete;
    OrderBookManager& operator=(const OrderBookManager&) = delete;
    OrderBookManager(OrderBookManager&& other) noexcept;
    OrderBookManager& operator=(OrderBookManager&& other) noexcept;

    /// Get existing orderbook or create new one if it doesn't exist
    /// Thread-safe: Uses shared_mutex for symbol map access
    /// Lifetime: pointer is invalidated by removeOrderBook(symbol)/clear() (see class docs)
    /// @param symbol Symbol identifier
    /// @return Pointer to orderbook (never null)
    [[nodiscard]] OrderBookT* getOrCreateOrderBook(SymbolId symbol);

    /// Get existing orderbook (read-only access)
    /// Thread-safe: Uses shared lock for read-only access
    /// Lifetime: pointer is invalidated by removeOrderBook(symbol)/clear() (see class docs)
    /// @param symbol Symbol identifier
    /// @return Pointer to orderbook, or nullptr if symbol doesn't exist
    [[nodiscard]] const OrderBookT* getOrderBook(SymbolId symbol) const;

    /// Get existing orderbook (mutable access)
    /// Thread-safe: Uses shared lock for read-only access to map
    /// Lifetime: pointer is invalidated by removeOrderBook(symbol)/clear() (see class docs)
    /// @param symbol Symbol identifier
    /// @return Pointer to orderbook, or nullptr if symbol doesn't exist
    [[nodiscard]] OrderBookT* getOrderBook(SymbolId symbol);

    /// Get existing orderbook or create new one, as a retaining handle
    /// Thread-safe; the orderbook stays alive while the handle is held,
    /// even if removeOrderBook()/clear() runs concurrently
    /// @param symbol Symbol identifier
    /// @return Shared pointer to orderbook (never null)
    [[nodiscard]] std::shared_ptr<OrderBookT> getOrCreateSharedOrderBook(SymbolId symbol);

    /// Get existing orderbook as a retaining handle
    /// Thread-safe; the orderbook stays alive while the handle is held,
    /// even if removeOrderBook()/clear() runs concurrently
    /// @param symbol Symbol identifier
    /// @return Shared pointer to orderbook, or nullptr if symbol doesn't exist
    [[nodiscard]] std::shared_ptr<OrderBookT> getSharedOrderBook(SymbolId symbol) const;

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
    /// Look up symbol under shared lock and project the entry while the lock is held
    template<typename Projection>
    auto find(SymbolId symbol, Projection&& proj) const;

    /// Look up or create symbol and project the entry while the lock is held
    template<typename Projection>
    auto findOrCreate(SymbolId symbol, Projection&& proj);

    mutable std::shared_mutex mutex_;  ///< Protects symbol_map_
    SymbolMap symbol_map_;             ///< Map of SymbolId -> OrderBook
};

SLICK_NAMESPACE_END

// Include implementation for header-only mode
#ifdef SLICK_ORDERBOOK_HEADER_ONLY
#include <slick/orderbook/detail/impl/orderbook_manager_impl.hpp>
#elif SLICK_EXTERN_TEMPLATE_DECLS
// Explicitly instantiated in the compiled library (src/core/orderbook_manager.cpp)
SLICK_NAMESPACE_BEGIN
SLICK_DLL_INTERFACE_WARNINGS_PUSH
extern template class SLICK_API OrderBookManager<OrderBookL2>;
extern template class SLICK_API OrderBookManager<OrderBookL3>;
SLICK_DLL_INTERFACE_WARNINGS_POP
SLICK_NAMESPACE_END
#endif
