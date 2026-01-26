// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>
#include <concepts>
#include <type_traits>

SLICK_NAMESPACE_BEGIN

// Forward declarations
struct PriceLevelUpdate;
struct OrderUpdate;
struct Trade;
struct TopOfBook;

/// Concept for orderbook observers
/// Observers must implement all four callback methods
template<typename T>
concept OrderBookObserver = requires(T observer,
                                     const PriceLevelUpdate& level_update,
                                     const OrderUpdate& order_update,
                                     const Trade& trade,
                                     const TopOfBook& tob) {
    { observer.onPriceLevelUpdate(level_update) } -> std::same_as<void>;
    { observer.onOrderUpdate(order_update) } -> std::same_as<void>;
    { observer.onTrade(trade) } -> std::same_as<void>;
    { observer.onTopOfBookUpdate(tob) } -> std::same_as<void>;
};

/// Concept for price-like types
template<typename T>
concept PriceLike = std::is_arithmetic_v<T> && std::totally_ordered<T>;

/// Concept for quantity-like types
template<typename T>
concept QuantityLike = std::is_arithmetic_v<T> && std::totally_ordered<T>;

/// Concept for timestamp-like types
template<typename T>
concept TimestampLike = std::is_arithmetic_v<T> && std::totally_ordered<T>;

/// Concept for container that supports reserve()
template<typename T>
concept Reservable = requires(T container, std::size_t n) {
    { container.reserve(n) } -> std::same_as<void>;
    { container.capacity() } -> std::convertible_to<std::size_t>;
};

/// Concept for allocator-like types
template<typename A>
concept AllocatorLike = requires(A alloc, std::size_t n) {
    typename A::value_type;
    { alloc.allocate(n) } -> std::same_as<typename A::value_type*>;
    { alloc.deallocate(std::declval<typename A::value_type*>(), n) } -> std::same_as<void>;
};

SLICK_NAMESPACE_END
