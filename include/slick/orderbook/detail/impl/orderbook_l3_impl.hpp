// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

#ifndef SLICK_OB_INLINE
#define SLICK_OB_INLINE inline
#endif

#include <limits>

SLICK_NAMESPACE_BEGIN

SLICK_OB_INLINE OrderBookL3::OrderBookL3(SymbolId symbol,
                         std::size_t initial_order_capacity,
                         std::size_t initial_level_capacity)
    : symbol_(symbol),
      order_map_(initial_order_capacity),
      order_pool_(initial_order_capacity),
      last_seq_num_(0) {
    // Initialize level maps with comparators
    // Note: Clang-17 with GCC-14 libstdc++ has issues with aggregate initialization in member init list
    levels_[Side::Buy] = PriceLevelMap{PriceComparator{Side::Buy}};
    levels_[Side::Sell] = PriceLevelMap{PriceComparator{Side::Sell}};

    // Reserve capacity for price levels (flat_map doesn't have reserve in C++23)
    // This will be used during insertions to pre-allocate space
    (void)initial_level_capacity;  // Mark as used

    // Initialize cached top-of-book
    cached_tob_.symbol = symbol_;
}

SLICK_OB_INLINE OrderBookL3::~OrderBookL3() {
    // Clean up all orders
    clear();
}

SLICK_OB_INLINE bool OrderBookL3::addOrModifyOrder(OrderId order_id, Side side, Price price, Quantity quantity,
                                   Timestamp timestamp, uint64_t priority, uint64_t seq_num, bool is_last_in_batch) {
    SLICK_ASSERT(side < SideCount);

    // Validate sequence number - reject out-of-order (seq_num < last_seq_num)
    if (seq_num > 0) {
        if (seq_num < last_seq_num_) {
            // Out of order - reject
            return false;
        }
        last_seq_num_ = seq_num;
    }

    // Check if order already exists
    detail::Order* order = order_map_.find(order_id);
    if (order) [[unlikely]] {
        if (order->side != side) {
            // Side mismatch implies OrderId reuse - reject.
            SLICK_ASSERT(false);
            return false;
        }

        if (SLICK_UNLIKELY(quantity <= 0)) {
            return (quantity == 0) ? deleteOrder(order_id, seq_num, is_last_in_batch) : false;
        }

        // Idempotent update - nothing to do.
        if (order->price == price && order->quantity == quantity) {
            return true;
        }

        // Use the provided timestamp for update notifications.
        order->timestamp = timestamp;
        return modifyOrder(order_id, price, quantity, seq_num, is_last_in_batch);
    }

    if (SLICK_UNLIKELY(quantity <= 0)) {
        return false;
    }

    // Allocate order from pool
    order = order_pool_.construct(order_id, price, quantity, side, timestamp, priority);
    if (SLICK_UNLIKELY(order == nullptr)) {
        return false;
    }

    // Get or create price level
    auto [level, level_idx, is_new] = getOrCreateLevel(side, price);

    // Insert order into level (maintains priority order)
    level->insertOrder(order);

    // Add to order map
    order_map_.insert(order);

    // New order: both price and quantity changed
    uint8_t order_flags = PriceChanged | QuantityChanged;
    uint8_t level_change_flag = QuantityChanged;
    if (is_new) {
        level_change_flag |= PriceChanged;
    }
    // Add LastInBatch flag if this is the last update
    if (is_last_in_batch) {
        order_flags |= LastInBatch;
        level_change_flag |= LastInBatch;
    }

    // Notify observers
    notifyOrderUpdate(order, 0, 0, timestamp, level_idx, order_flags, seq_num);
    notifyPriceLevelUpdate(side, price, level->getTotalQuantity(), timestamp, level_idx, level_change_flag, seq_num);
    notifyTopOfBookIfChanged(timestamp, order_flags);

    return true;
}

SLICK_OB_INLINE bool OrderBookL3::addOrder(OrderId order_id, Side side, Price price, Quantity quantity,
                          Timestamp timestamp, uint64_t priority, uint64_t seq_num, bool is_last_in_batch) {
    // Validate sequence number - reject out-of-order (seq_num < last_seq_num)
    if (seq_num > 0) {
        if (seq_num < last_seq_num_) {
            // Out of order - reject
            return false;
        }
        last_seq_num_ = seq_num;
    }

    // Strict add - fail if order already exists
    if (order_map_.contains(order_id)) {
        return false;
    }
    // Use timestamp as priority if not specified
    uint64_t actual_priority = (priority == 0) ? timestamp : priority;
    return addOrModifyOrder(order_id, side, price, quantity, timestamp, actual_priority, seq_num, is_last_in_batch);
}

SLICK_OB_INLINE bool OrderBookL3::modifyOrder(OrderId order_id, Price new_price, Quantity new_quantity,
                                               uint64_t seq_num, bool is_last_in_batch) {
    // Validate sequence number - reject out-of-order (seq_num < last_seq_num)
    if (seq_num > 0) {
        if (seq_num < last_seq_num_) {
            // Out of order - reject
            return false;
        }
        last_seq_num_ = seq_num;
    }

    // Find order
    detail::Order* order = order_map_.find(order_id);
    if (!order) {
        return false;
    }

    if (SLICK_UNLIKELY(new_quantity < 0)) {
        return false;
    }

    // Check if this is a delete (quantity = 0)
    if (new_quantity == 0) {
        return deleteOrder(order_id, seq_num, is_last_in_batch);
    }

    const Timestamp timestamp = order->timestamp;  // Use original timestamp for modifications
    const Price old_price = order->price;
    const Quantity old_quantity = order->quantity;
    const Side side = order->side;

    // Check what changed
    const bool price_changed = (new_price != old_price);
    const bool quantity_changed = (new_quantity != old_quantity);

    if (!price_changed && !quantity_changed) {
        // No change
        return true;
    }

    if (price_changed) {
        // Price changed - remove from old level (if exists) and add to new level
        auto [old_level, old_level_idx] = getLevel(side, old_price);
        Quantity old_level_total = 0;

        if (old_level) {
            // Remove from old level
            old_level->removeOrder(order);
            old_level_total = old_level->getTotalQuantity();

            uint8_t old_level_change_flags = QuantityChanged;
            if (removeLevelIfEmpty(side, old_price)) {
                old_level_change_flags |= PriceChanged;
            }
            // Don't add LastInBatch to intermediate old level update
            notifyPriceLevelUpdate(side, old_price, old_level_total, timestamp, old_level_idx, old_level_change_flags, seq_num);
        }

        // Update order fields
        order->price = new_price;
        order->quantity = new_quantity;

        // Get or create new level
        auto [new_level, new_level_idx, is_new] = getOrCreateLevel(side, new_price);

        // Insert into new level
        new_level->insertOrder(order);

        // Notify observers (price changed: both price and quantity flags)
        uint8_t order_flags = PriceChanged;
        if (quantity_changed) {
            order_flags |= QuantityChanged;
        }
        uint8_t new_level_change_flags = QuantityChanged;
        if (is_new) {
            new_level_change_flags |= PriceChanged;
        }
        // Add LastInBatch flag if this is the last update
        if (is_last_in_batch) {
            order_flags |= LastInBatch;
            new_level_change_flags |= LastInBatch;
        }

        notifyOrderUpdate(order, old_quantity, old_price, timestamp, new_level_idx, order_flags, seq_num);
        notifyPriceLevelUpdate(side, new_price, new_level->getTotalQuantity(), timestamp, new_level_idx, new_level_change_flags, seq_num);
        notifyTopOfBookIfChanged(timestamp, order_flags);

    } else {
        // Only quantity changed
        auto [level, level_index, is_new] = getOrCreateLevel(side, old_price);

        // Update quantity
        level->updateOrderQuantity(old_quantity, new_quantity);
        order->quantity = new_quantity;

        // Notify observers (only quantity changed)
        uint8_t order_flags = QuantityChanged;
        uint8_t level_change_flags = QuantityChanged;
        if (is_new) [[unlikely]] {
            level_change_flags |= PriceChanged;
        }
        // Add LastInBatch flag if this is the last update
        if (is_last_in_batch) {
            order_flags |= LastInBatch;
            level_change_flags |= LastInBatch;
        }

        notifyOrderUpdate(order, old_quantity, old_price, timestamp, level_index, order_flags, seq_num);
        notifyPriceLevelUpdate(side, old_price, level->getTotalQuantity(), timestamp, level_index, level_change_flags, seq_num);
        notifyTopOfBookIfChanged(timestamp, order_flags);
    }

    return true;
}

SLICK_OB_INLINE bool OrderBookL3::deleteOrder(OrderId order_id, uint64_t seq_num, bool is_last_in_batch) noexcept {
    // Validate sequence number - reject out-of-order (seq_num < last_seq_num)
    if (seq_num > 0) {
        if (seq_num < last_seq_num_) {
            // Out of order - reject
            return false;
        }
        last_seq_num_ = seq_num;
    }

    // Find order
    detail::Order* order = order_map_.find(order_id);
    if (!order) {
        return false;
    }

    const Timestamp timestamp = order->timestamp;
    const Price price = order->price;
    const Side side = order->side;

    // Get level
    auto [level, level_idx] = getLevel(side, price);
    if (!level) {
        // Level doesn't exist - data structure inconsistency
        // Deletion: both price and quantity changed
        uint8_t order_flags = PriceChanged | QuantityChanged;
        if (is_last_in_batch) {
            order_flags |= LastInBatch;
        }
        // Notify deletion, then clean up (use max index since level not found)
        notifyOrderDelete(order, timestamp, std::numeric_limits<uint16_t>::max(), order_flags, seq_num);
        order_map_.erase(order_id);
        order_pool_.destroy(order);
        return false;
    }

    // Remove from level
    level->removeOrder(order);
    const Quantity level_total = level->getTotalQuantity();

    // Remove from order map
    order_map_.erase(order_id);

    // Deletion: both price and quantity changed
    uint8_t order_flags = PriceChanged | QuantityChanged;
    uint8_t level_change_flags = QuantityChanged;
    if (removeLevelIfEmpty(side, price)) {
        level_change_flags |= PriceChanged;
    }
    // Add LastInBatch flag if this is the last update
    if (is_last_in_batch) {
        order_flags |= LastInBatch;
        level_change_flags |= LastInBatch;
    }

    // Notify observers (before destroying order)
    notifyOrderDelete(order, timestamp, level_idx, order_flags, seq_num);
    notifyPriceLevelUpdate(side, price, level_total, timestamp, level_idx, level_change_flags, seq_num);

    // Destroy order
    order_pool_.destroy(order);

    // Notify ToB if changed
    notifyTopOfBookIfChanged(timestamp, order_flags);

    return true;
}

SLICK_OB_INLINE bool OrderBookL3::executeOrder(OrderId order_id, Quantity executed_quantity, uint64_t seq_num, bool is_last_in_batch) {
    // Validate sequence number - reject out-of-order (seq_num < last_seq_num)
    if (seq_num > 0) {
        if (seq_num < last_seq_num_) {
            // Out of order - reject
            return false;
        }
        last_seq_num_ = seq_num;
    }

    detail::Order* order = order_map_.find(order_id);
    if (!order) {
        return false;
    }

    if (SLICK_UNLIKELY(executed_quantity <= 0 || executed_quantity > order->quantity)) {
        SLICK_ASSERT(executed_quantity > 0 && executed_quantity <= order->quantity);
        return false;
    }

    const Quantity remaining = order->quantity - executed_quantity;

    if (remaining == 0) {
        // Fully executed - delete order (pass through seq_num and is_last_in_batch)
        return deleteOrder(order_id, seq_num, is_last_in_batch);
    } else {
        // Partial execution - reduce quantity (pass through seq_num and is_last_in_batch)
        return modifyOrder(order_id, order->price, remaining, seq_num, is_last_in_batch);
    }
}

SLICK_OB_INLINE const detail::Order* OrderBookL3::findOrder(OrderId order_id) const noexcept {
    return order_map_.find(order_id);
}

SLICK_OB_INLINE const detail::PriceLevelL3* OrderBookL3::getBestBid() const noexcept {
    const auto& bids = levels_[Side::Buy];
    if (bids.empty()) {
        return nullptr;
    }
    // FlatMap is sorted by key (Price) in ascending order by default
    // For Buy side, we want highest price first, so use reverse iterator
    return &bids.begin()->second;
}

SLICK_OB_INLINE const detail::PriceLevelL3* OrderBookL3::getBestAsk() const noexcept {
    const auto& asks = levels_[Side::Sell];
    if (asks.empty()) {
        return nullptr;
    }
    // For Sell side, we want lowest price first (first element)
    return &asks.begin()->second;
}

SLICK_OB_INLINE TopOfBook OrderBookL3::getTopOfBook() const noexcept {
    const auto* bid = getBestBid();
    const auto* ask = getBestAsk();

    TopOfBook tob;
    tob.symbol = symbol_;

    if (bid) {
        tob.best_bid = bid->price;
        tob.bid_quantity = bid->getTotalQuantity();
        if (!bid->orders.empty()) {
            tob.timestamp = bid->orders.front()->timestamp;
        }
    }

    if (ask) {
        tob.best_ask = ask->price;
        tob.ask_quantity = ask->getTotalQuantity();
        if (!ask->orders.empty()) {
            const Timestamp ask_time = ask->orders.front()->timestamp;
            if (ask_time > tob.timestamp) {
                tob.timestamp = ask_time;
            }
        }
    }

    return tob;
}

SLICK_OB_INLINE std::vector<detail::PriceLevelL2> OrderBookL3::getLevelsL2(Side side, std::size_t depth) const {
    SLICK_ASSERT(side < SideCount);

    const auto& level_map = levels_[side];
    std::vector<detail::PriceLevelL2> result;

    if (level_map.empty()) {
        return result;
    }

    const std::size_t count = (depth == 0) ? level_map.size() : std::min(depth, level_map.size());
    result.reserve(count);

    if (side == Side::Buy) {
        // For bids, iterate in reverse (highest price first)
        auto it = level_map.begin();
        for (std::size_t i = 0; i < count && it != level_map.end(); ++i, ++it) {
            const auto& [price, level] = *it;
            Timestamp latest_timestamp = level.orders.empty() ? 0 : level.orders.front()->timestamp;
            result.emplace_back(price, level.getTotalQuantity(), latest_timestamp);
        }
    } else {
        // For asks, iterate forward (lowest price first)
        auto it = level_map.begin();
        for (std::size_t i = 0; i < count && it != level_map.end(); ++i, ++it) {
            const auto& [price, level] = *it;
            Timestamp latest_timestamp = level.orders.empty() ? 0 : level.orders.front()->timestamp;
            result.emplace_back(price, level.getTotalQuantity(), latest_timestamp);
        }
    }

    return result;
}

SLICK_OB_INLINE std::pair<const detail::PriceLevelL3*, uint16_t> OrderBookL3::getLevel(Side side, Price price) const noexcept {
    SLICK_ASSERT(side < SideCount);
    const auto& level_map = levels_[side];
    auto it = level_map.find(price);
    if (it != level_map.end()) {
        uint16_t index = static_cast<uint16_t>(std::distance(level_map.begin(), it));
        return {&it->second, index};
    }
    return {nullptr, std::numeric_limits<uint16_t>::max()};
}

SLICK_OB_INLINE std::pair<detail::PriceLevelL3*, uint16_t> OrderBookL3::getLevel(Side side, Price price) noexcept {
    SLICK_ASSERT(side < SideCount);
    auto& level_map = levels_[side];
    auto it = level_map.find(price);
    if (it != level_map.end()) {
        uint16_t index = static_cast<uint16_t>(std::distance(level_map.begin(), it));
        return {&it->second, index};
    }
    return {nullptr, std::numeric_limits<uint16_t>::max()};
}

SLICK_OB_INLINE const detail::PriceLevelL3* OrderBookL3::getLevelByIndex(Side side, uint16_t index) const noexcept {
    SLICK_ASSERT(side < SideCount);
    const auto& level_map = levels_[side];
    if (index >= level_map.size()) {
        return nullptr;
    }
    auto it = level_map.begin();
    std::advance(it, index);
    return &it->second;
}

SLICK_OB_INLINE std::size_t OrderBookL3::levelCount(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    return levels_[side].size();
}

SLICK_OB_INLINE std::size_t OrderBookL3::orderCount(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    std::size_t count = 0;
    for (const auto& [price, level] : levels_[side]) {
        count += level.orderCount();
    }
    return count;
}

SLICK_OB_INLINE bool OrderBookL3::isEmpty(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    return levels_[side].empty();
}

SLICK_OB_INLINE bool OrderBookL3::isEmpty() const noexcept {
    return levels_[Side::Buy].empty() && levels_[Side::Sell].empty();
}

SLICK_OB_INLINE void OrderBookL3::clearSide(Side side) noexcept {
    SLICK_ASSERT(side < SideCount);

    // Delete all orders on this side
    auto& level_map = levels_[side];
    for (auto& [price, level] : level_map) {
        for (auto it = level.orders.begin(); it != level.orders.end(); ) {
            auto* order = &(*it);
            ++it;  // Advance before unlinking/destroying.
            order_map_.erase(order->order_id);
            level.orders.erase(order);
            order_pool_.destroy(order);
        }
    }

    // Clear level map
    level_map.clear();
}

SLICK_OB_INLINE void OrderBookL3::clear() noexcept {
    clearSide(Side::Buy);
    clearSide(Side::Sell);
}

SLICK_OB_INLINE std::tuple<detail::PriceLevelL3*, uint16_t, bool> OrderBookL3::getOrCreateLevel(Side side, Price price) {
    SLICK_ASSERT(side < SideCount);

    auto& level_map = levels_[side];
    auto it = level_map.find(price);

    if (it != level_map.end()) {
        uint16_t index = static_cast<uint16_t>(std::distance(level_map.begin(), it));
        return {&it->second, index, false};
    }

    // Create new level
    auto [new_it, inserted] = level_map.emplace(price, detail::PriceLevelL3{price});
    SLICK_ASSERT(inserted);
    uint16_t index = static_cast<uint16_t>(std::distance(level_map.begin(), new_it));
    return {&new_it->second, index, true};
}

SLICK_OB_INLINE bool OrderBookL3::removeLevelIfEmpty(Side side, Price price) noexcept {
    SLICK_ASSERT(side < SideCount);

    auto& level_map = levels_[side];
    auto it = level_map.find(price);

    if (it != level_map.end() && it->second.isEmpty()) {
        level_map.erase(it);
        return true;
    }
    return false;
}

SLICK_OB_INLINE uint16_t OrderBookL3::calculateLevelIndex(Side side, Price price) const noexcept {
    SLICK_ASSERT(side < SideCount);
    const auto& level_map = levels_[side];

    // Find the price level using lower_bound
    auto it = level_map.lower_bound(price);

    // If exact match, calculate its index
    if (it != level_map.end() && it->first == price) {
        return static_cast<uint16_t>(std::distance(level_map.begin(), it));
    }

    // Price not found - return the index where it would be inserted
    return static_cast<uint16_t>(std::distance(level_map.begin(), it));
}

SLICK_OB_INLINE void OrderBookL3::notifyOrderUpdate(const detail::Order* order, [[maybe_unused]] Quantity old_quantity,
                                    [[maybe_unused]] Price old_price, Timestamp timestamp,
                                    uint16_t level_index, uint8_t change_flags, uint64_t seq_num) const {
    OrderUpdate update{
        symbol_,
        order->order_id,
        order->side,
        order->price,
        order->quantity,
        timestamp,
        level_index,
        order->priority,
        change_flags,
        seq_num
    };
    observers_.notifyOrderUpdate(update);
}

SLICK_OB_INLINE void OrderBookL3::notifyOrderDelete(const detail::Order* order, Timestamp timestamp, uint16_t level_index, uint8_t change_flags, uint64_t seq_num) const {
    OrderUpdate update{
        symbol_,
        order->order_id,
        order->side,
        order->price,
        0,  // quantity = 0 means delete
        timestamp,
        level_index,
        order->priority,
        change_flags,
        seq_num
    };
    observers_.notifyOrderUpdate(update);
}

SLICK_OB_INLINE void OrderBookL3::notifyTrade(OrderId passive_order_id, OrderId aggressive_order_id,
                              Side aggressor_side, Price price, Quantity quantity,
                              Timestamp timestamp) const {
    Trade trade{
        symbol_,
        price,
        quantity,
        timestamp,
        aggressor_side,
        passive_order_id,
        aggressive_order_id,
    };
    observers_.notifyTrade(trade);
}

SLICK_OB_INLINE void OrderBookL3::notifyPriceLevelUpdate(Side side, Price price, Quantity total_quantity,
                                         Timestamp timestamp, uint16_t level_index,
                                         uint8_t change_flags, uint64_t seq_num) const {
    PriceLevelUpdate update{
        symbol_,
        side,
        price,
        total_quantity,
        timestamp,
        level_index,
        change_flags,
        seq_num
    };
    observers_.notifyPriceLevelUpdate(update);
}

SLICK_OB_INLINE void OrderBookL3::notifyTopOfBookIfChanged(Timestamp timestamp, uint8_t update_flags) {
    // Only emit ToB if LastInBatch flag is set
    // (defaults to true for single operations, false for intermediate batch updates)
    if ((update_flags & LastInBatch) == 0) {
        return;  // Skip ToB emission for intermediate batch updates
    }

    // Compute current top-of-book
    const auto* bid = getBestBid();
    const auto* ask = getBestAsk();

    Price new_best_bid = bid ? bid->price : 0;
    Quantity new_bid_qty = bid ? bid->getTotalQuantity() : 0;
    Price new_best_ask = ask ? ask->price : 0;
    Quantity new_ask_qty = ask ? ask->getTotalQuantity() : 0;

    // Check if best bid or ask changed
    const bool bid_changed = (cached_tob_.best_bid != new_best_bid) ||
                            (cached_tob_.bid_quantity != new_bid_qty);
    const bool ask_changed = (cached_tob_.best_ask != new_best_ask) ||
                            (cached_tob_.ask_quantity != new_ask_qty);

    if (bid_changed || ask_changed) {
        // Update cached values
        cached_tob_.best_bid = new_best_bid;
        cached_tob_.bid_quantity = new_bid_qty;
        cached_tob_.best_ask = new_best_ask;
        cached_tob_.ask_quantity = new_ask_qty;
        cached_tob_.timestamp = timestamp;

        // Notify observers
        observers_.notifyTopOfBookUpdate(cached_tob_);
    }
}

SLICK_OB_INLINE void OrderBookL3::emitSnapshot(Timestamp timestamp) {
    observers_.notifySnapshotBegin(symbol_, last_seq_num_, timestamp);

    // Bids: highest price first (best = index 0)
    uint16_t level_idx = 0;
    for (auto it = levels_[Side::Buy].begin(); it != levels_[Side::Buy].end(); ++it, ++level_idx) {
        const auto& level = it->second;
        for (const auto& order : level.orders) {
            OrderUpdate update{
                symbol_,
                order.order_id,
                order.side,
                order.price,
                order.quantity,
                timestamp,
                level_idx,
                order.priority,
                static_cast<uint8_t>(PriceChanged | QuantityChanged)
            };
            observers_.notifyOrderUpdate(update);
        }
    }

    // Asks: lowest price first (best = index 0)
    level_idx = 0;
    for (auto it = levels_[Side::Sell].begin(); it != levels_[Side::Sell].end(); ++it, ++level_idx) {
        const auto& level = it->second;
        for (const auto& order : level.orders) {
            OrderUpdate update{
                symbol_,
                order.order_id,
                order.side,
                order.price,
                order.quantity,
                timestamp,
                level_idx,
                order.priority,
                static_cast<uint8_t>(PriceChanged | QuantityChanged)
            };
            observers_.notifyOrderUpdate(update);
        }
    }

    observers_.notifySnapshotEnd(symbol_, last_seq_num_, timestamp);
}

SLICK_NAMESPACE_END
