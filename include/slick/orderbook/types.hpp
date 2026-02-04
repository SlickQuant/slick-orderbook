// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <cstdint>
#include <string_view>
#include <limits>

SLICK_NAMESPACE_BEGIN

// Core numeric types with strong typing for performance and safety

/// Price type - fixed-point representation
/// Example: For 4 decimal places, price = actual_price * 10000
/// This allows integer arithmetic while maintaining decimal precision
using Price = int64_t;

/// Quantity type - volume or size
using Quantity = int64_t;

/// Unique order identifier
using OrderId = uint64_t;

/// Symbol identifier - can be hash or sequential ID
using SymbolId = uint16_t;

/// Timestamp in nanoseconds since epoch
using Timestamp = uint64_t;

constexpr uint16_t INVALID_INDEX = std::numeric_limits<uint16_t>::max();

/// Order side - using enum for easy array indexing
enum Side : uint8_t {
    Buy = 0,
    Sell = 1,
    SideCount = 2  // Helper for array sizing
};

/// Order type
enum class OrderType : uint8_t {
    Limit,      // Limit order
    Market,     // Market order
    Stop,       // Stop order
    StopLimit   // Stop-limit order
};

/// Symbol information
struct Symbol {
    SymbolId id;               // Unique identifier
    std::string_view name;     // Symbol name (non-owning view for performance)

    constexpr Symbol() noexcept : id(0), name() {}
    constexpr Symbol(SymbolId id_, std::string_view name_) noexcept
        : id(id_), name(name_) {}

    [[nodiscard]] constexpr bool operator==(const Symbol& other) const noexcept {
        return id == other.id;
    }

    [[nodiscard]] constexpr bool operator!=(const Symbol& other) const noexcept {
        return !(*this == other);
    }
};

/// Helper functions for Side enum

[[nodiscard]] constexpr const char* toString(Side side) noexcept {
    switch (side) {
        case Buy:  return "Buy";
        case Sell: return "Sell";
        default:   return "Unknown";
    }
}

[[nodiscard]] constexpr Side opposite(Side side) noexcept {
    return side == Buy ? Sell : Buy;
}

/// Helper functions for OrderType enum

[[nodiscard]] constexpr const char* toString(OrderType type) noexcept {
    switch (type) {
        case OrderType::Limit:     return "Limit";
        case OrderType::Market:    return "Market";
        case OrderType::Stop:      return "Stop";
        case OrderType::StopLimit: return "StopLimit";
        default:                   return "Unknown";
    }
}

SLICK_NAMESPACE_END
