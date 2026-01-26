// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <flat_map>

SLICK_DETAIL_NAMESPACE_BEGIN

/// Alias for std::flat_map - cache-friendly associative container
///
/// std::flat_map (C++23) provides:
/// - Excellent cache locality (contiguous storage)
/// - Fast iteration (sequential memory access)
/// - Binary search for lookups (O(log n))
/// - Better performance than std::map for typical orderbook sizes (< 100 elements)
///
/// This is a thin alias to allow future customization if needed while
/// keeping the codebase clean.
///
/// @tparam Key Key type (must be comparable)
/// @tparam Value Value type
/// @tparam Compare Comparison function (default: std::less<Key>)
template<typename Key, typename Value, typename Compare = std::less<Key>>
using FlatMap = std::flat_map<Key, Value, Compare>;

SLICK_DETAIL_NAMESPACE_END
