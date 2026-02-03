// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/types.hpp>
#include <slick/orderbook/detail/price_level.hpp>
#include <vector>
#include <algorithm>
#include <functional>

SLICK_DETAIL_NAMESPACE_BEGIN

/// Container for storing sorted price levels
/// Uses a sorted vector for cache-friendly storage (optimal for < 100 levels)
///
/// The container is configured at construction time to sort either as bids (descending)
/// or asks (ascending), allowing use in std::array for O(1) side indexing.
class LevelContainer {
public:
    using value_type = PriceLevelL2;
    using iterator = typename std::vector<PriceLevelL2>::iterator;
    using const_iterator = typename std::vector<PriceLevelL2>::const_iterator;
    using Comparator = std::function<bool(const PriceLevelL2&, const PriceLevelL2&)>;

    /// Constructor with side and initial capacity
    /// @param side Side (Buy = bids/descending, Sell = asks/ascending)
    /// @param initial_capacity Initial capacity for levels
    explicit LevelContainer(Side side, std::size_t initial_capacity = 32)
        : side_(side),
          compare_(side == Side::Buy ?
                   Comparator{BidComparator{}} :    // Bids: descending
                   Comparator{AskComparator{}})     // Asks: ascending
    {
        levels_.reserve(initial_capacity);
    }

    /// Get Side
    [[nodiscard]] Side side() const noexcept {
        return side_;
    }

    /// Get number of levels
    [[nodiscard]] std::size_t size() const noexcept {
        return levels_.size();
    }

    /// Check if empty
    [[nodiscard]] bool empty() const noexcept {
        return levels_.empty();
    }

    /// Get level at index (no bounds checking)
    [[nodiscard]] const PriceLevelL2& operator[](std::size_t index) const noexcept {
        return levels_[index];
    }

    /// Get level at index with bounds checking
    [[nodiscard]] const PriceLevelL2& at(std::size_t index) const {
        return levels_.at(index);
    }

    /// Get best level (first element)
    [[nodiscard]] const PriceLevelL2* best() const noexcept {
        return levels_.empty() ? nullptr : &levels_.front();
    }

    /// Find level by price (binary search)
    /// Returns iterator to level with matching price, or end() if not found
    [[nodiscard]] iterator find(Price price) noexcept {
        PriceLevelL2 key{price, 0, 0};
        auto it = std::lower_bound(levels_.begin(), levels_.end(), key, compare_);
        if (it != levels_.end() && it->price == price) {
            return it;
        }
        return levels_.end();
    }

    /// Find level by price (binary search, const version)
    [[nodiscard]] const_iterator find(Price price) const noexcept {
        PriceLevelL2 key{price, 0, 0};
        auto it = std::lower_bound(levels_.begin(), levels_.end(), key, compare_);
        if (it != levels_.end() && it->price == price) {
            return it;
        }
        return levels_.end();
    }

    /// Insert or update a level
    /// If price exists, updates quantity; otherwise inserts new level
    /// Returns iterator to the level and whether insertion occurred
    std::pair<iterator, bool> insertOrUpdate(Price price, Quantity quantity, Timestamp timestamp) {
        // Binary search for insertion position
        PriceLevelL2 key{price, 0, 0};
        auto it = std::lower_bound(levels_.begin(), levels_.end(), key, compare_);

        // Check if price already exists
        if (it != levels_.end() && it->price == price) {
            // Update existing level
            it->quantity = quantity;
            it->timestamp = timestamp;
            return {it, false};
        }

        // Insert new level
        it = levels_.insert(it, PriceLevelL2{price, quantity, timestamp});
        return {it, true};
    }

    /// Remove level by price
    /// Returns true if level was found and removed
    bool erase(Price price) noexcept {
        auto it = find(price);
        if (it != levels_.end()) {
            levels_.erase(it);
            return true;
        }
        return false;
    }

    /// Remove level by iterator
    iterator erase(iterator it) noexcept {
        return levels_.erase(it);
    }

    /// Clear all levels
    void clear() noexcept {
        levels_.clear();
    }

    /// Reserve capacity
    void reserve(std::size_t capacity) {
        levels_.reserve(capacity);
    }

    /// Get capacity
    [[nodiscard]] std::size_t capacity() const noexcept {
        return levels_.capacity();
    }

    /// Get all levels up to depth
    /// @param depth Maximum number of levels to return (0 = all)
    [[nodiscard]] std::vector<PriceLevelL2> getLevels(std::size_t depth = 0) const {
        if (depth == 0 || depth >= levels_.size()) {
            return levels_;
        }
        return std::vector<PriceLevelL2>(levels_.begin(), levels_.begin() + depth);
    }

    /// Iterators
    [[nodiscard]] iterator begin() noexcept { return levels_.begin(); }
    [[nodiscard]] iterator end() noexcept { return levels_.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return levels_.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return levels_.end(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return levels_.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return levels_.cend(); }

private:
    Side side_;                          // Side (Buy or Sell)
    Comparator compare_;                 // Comparison function for sorting
    std::vector<PriceLevelL2> levels_;  // Sorted vector of price levels
};

SLICK_DETAIL_NAMESPACE_END
