// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <version>

// Check for C++23 flat_map support
#if defined(__cpp_lib_flat_map)
    #include <flat_map>
    #define SLICK_HAS_FLAT_MAP 1
#else
    #include <map>
    #define SLICK_HAS_FLAT_MAP 0
#endif

SLICK_DETAIL_NAMESPACE_BEGIN

/// Cache-friendly sorted map
///
/// Uses std::flat_map (C++23) if available, which provides:
/// - Excellent cache locality (contiguous storage)
/// - Fast iteration (sequential memory access)
/// - Binary search for lookups (O(log n))
///
/// Falls back to std::map on compilers without C++23 flat_map support.
///
/// @tparam Key Key type (must be comparable)
/// @tparam Value Value type
/// @tparam Compare Comparison function (default: std::less<Key>)
#if SLICK_HAS_FLAT_MAP
template<typename Key, typename Value, typename Compare = std::less<Key>>
using FlatMap = std::flat_map<Key, Value, Compare>;
#else
template<typename Key, typename Value, typename Compare = std::less<Key>>
using FlatMap = std::map<Key, Value, Compare>;
#endif

SLICK_DETAIL_NAMESPACE_END
