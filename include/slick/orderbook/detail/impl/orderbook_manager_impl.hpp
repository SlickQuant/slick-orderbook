// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

SLICK_NAMESPACE_BEGIN

template<typename OrderBookT>
OrderBookManager<OrderBookT>::OrderBookManager(std::size_t initial_symbol_capacity)
    : symbol_map_() {
    // Reserve capacity to avoid rehashing during initial symbol registration
    // Note: reserve() is not available in std::map, only in vector-based containers
    // This is a no-op if FlatMap is std::map, otherwise it pre-allocates
    if constexpr (requires { symbol_map_.reserve(initial_symbol_capacity); }) {
        symbol_map_.reserve(initial_symbol_capacity);
    }
}

template<typename OrderBookT>
OrderBookT* OrderBookManager<OrderBookT>::getOrCreateOrderBook(SymbolId symbol) {
    // First, try read-only access (shared lock) - common case
    {
        std::shared_lock lock(mutex_);
        auto it = symbol_map_.find(symbol);
        if (it != symbol_map_.end()) [[likely]] {
            return it->second.get();
        }
    }

    // Symbol doesn't exist - need exclusive lock to insert
    std::unique_lock lock(mutex_);

    // Double-check pattern: another thread might have inserted while we waited
    auto it = symbol_map_.find(symbol);
    if (it != symbol_map_.end()) {
        return it->second.get();
    }

    // Create new orderbook
    auto orderbook = std::make_unique<OrderBookT>(symbol);
    OrderBookT* ptr = orderbook.get();
    symbol_map_.emplace(symbol, std::move(orderbook));

    return ptr;
}

template<typename OrderBookT>
const OrderBookT* OrderBookManager<OrderBookT>::getOrderBook(SymbolId symbol) const {
    std::shared_lock lock(mutex_);
    auto it = symbol_map_.find(symbol);
    return (it != symbol_map_.end()) ? it->second.get() : nullptr;
}

template<typename OrderBookT>
OrderBookT* OrderBookManager<OrderBookT>::getOrderBook(SymbolId symbol) {
    std::shared_lock lock(mutex_);
    auto it = symbol_map_.find(symbol);
    return (it != symbol_map_.end()) ? it->second.get() : nullptr;
}

template<typename OrderBookT>
bool OrderBookManager<OrderBookT>::hasSymbol(SymbolId symbol) const {
    std::shared_lock lock(mutex_);
    return symbol_map_.contains(symbol);
}

template<typename OrderBookT>
bool OrderBookManager<OrderBookT>::removeOrderBook(SymbolId symbol) {
    std::unique_lock lock(mutex_);
    return symbol_map_.erase(symbol) > 0;
}

template<typename OrderBookT>
std::vector<SymbolId> OrderBookManager<OrderBookT>::getSymbols() const {
    std::shared_lock lock(mutex_);
    std::vector<SymbolId> symbols;
    symbols.reserve(symbol_map_.size());

    for (const auto& [symbol_id, _] : symbol_map_) {
        symbols.push_back(symbol_id);
    }

    return symbols;
}

template<typename OrderBookT>
std::size_t OrderBookManager<OrderBookT>::symbolCount() const {
    std::shared_lock lock(mutex_);
    return symbol_map_.size();
}

template<typename OrderBookT>
void OrderBookManager<OrderBookT>::clear() {
    std::unique_lock lock(mutex_);
    symbol_map_.clear();
}

template<typename OrderBookT>
void OrderBookManager<OrderBookT>::reserve(std::size_t capacity) {
    // No lock needed - should be called before concurrent access
    if constexpr (requires { symbol_map_.reserve(capacity); }) {
        symbol_map_.reserve(capacity);
    }
}


SLICK_NAMESPACE_END
