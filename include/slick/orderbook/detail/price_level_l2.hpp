// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>

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

    [[nodiscard]] constexpr bool operator()(Price a, Price b) const noexcept {
        return a < b;
    }
};

SLICK_DETAIL_NAMESPACE_END
