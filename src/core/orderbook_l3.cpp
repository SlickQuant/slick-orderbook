// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_l3.hpp>

SLICK_NAMESPACE_BEGIN

OrderBookL3::OrderBookL3(SymbolId symbol,
                         std::size_t initial_order_capacity,
                         std::size_t initial_level_capacity)
    : symbol_(symbol),
      levels_(),
      order_map_(initial_order_capacity),
      order_pool_(initial_order_capacity),
      observers_(),
      cached_tob_() {
    // Reserve capacity for price levels (flat_map doesn't have reserve in C++23)
    // This will be used during insertions to pre-allocate space
    (void)initial_level_capacity;  // Mark as used

    // Initialize cached top-of-book
    cached_tob_.symbol = symbol_;
}

OrderBookL3::~OrderBookL3() {
    // Clean up all orders
    clear();
}

bool OrderBookL3::addOrModifyOrder(OrderId order_id, Side side, Price price, Quantity quantity,
                                   Timestamp timestamp, uint64_t priority) {
    SLICK_ASSERT(side < SideCount);
    SLICK_ASSERT(quantity > 0);

    // Check if order already exists
    detail::Order* order = order_map_.find(order_id);
    if (order) [[unlikely]] {
        if (order->side == side) {
            return modifyOrder(order_id, price, quantity);
        }
        // side is different, the order_id reused?
        SLICK_ASSERT(false);
        return false;
    }

    // Allocate order from pool
    order = order_pool_.construct(order_id, price, quantity, side, timestamp, priority);

    // Get or create price level
    detail::PriceLevelL3* level = getOrCreateLevel(side, price);

    // Insert order into level (maintains priority order)
    level->insertOrder(order);

    // Add to order map
    order_map_.insert(order);

    // Notify observers
    notifyOrderUpdate(order, 0, 0, timestamp);
    notifyPriceLevelUpdate(side, price, level->getTotalQuantity(), timestamp);
    notifyTopOfBookIfChanged(timestamp);

    return true;
}

bool OrderBookL3::addOrder(OrderId order_id, Side side, Price price, Quantity quantity,
                          Timestamp timestamp, uint64_t priority) {
    // Strict add - fail if order already exists
    if (order_map_.contains(order_id)) {
        return false;
    }
    return addOrModifyOrder(order_id, side, price, quantity, timestamp, priority);
}

bool OrderBookL3::modifyOrder(OrderId order_id, Price new_price, Quantity new_quantity) {
    // Find order
    detail::Order* order = order_map_.find(order_id);
    if (!order) {
        return false;
    }

    // Check if this is a delete (quantity = 0)
    if (new_quantity == 0) {
        return deleteOrder(order_id);
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
        detail::PriceLevelL3* old_level = getLevel(side, old_price);
        Quantity old_level_total = 0;

        if (old_level) {
            // Remove from old level
            old_level->removeOrder(order);
            old_level_total = old_level->getTotalQuantity();
        }

        // Update order fields
        order->price = new_price;
        order->quantity = new_quantity;

        // Get or create new level
        detail::PriceLevelL3* new_level = getOrCreateLevel(side, new_price);

        // Insert into new level
        new_level->insertOrder(order);

        // Remove old level if empty
        if (old_level) {
            removeLevelIfEmpty(side, old_price);
        }

        // Notify observers
        notifyOrderUpdate(order, old_quantity, old_price, timestamp);
        if (old_level) {
            notifyPriceLevelUpdate(side, old_price, old_level_total, timestamp);
        }
        notifyPriceLevelUpdate(side, new_price, new_level->getTotalQuantity(), timestamp);
        notifyTopOfBookIfChanged(timestamp);

    } else {
        // Only quantity changed
        detail::PriceLevelL3* level = getOrCreateLevel(side, old_price);

        // Update quantity
        level->updateOrderQuantity(old_quantity, new_quantity);
        order->quantity = new_quantity;

        // Notify observers
        notifyOrderUpdate(order, old_quantity, old_price, timestamp);
        notifyPriceLevelUpdate(side, old_price, level->getTotalQuantity(), timestamp);
        notifyTopOfBookIfChanged(timestamp);
    }

    return true;
}

bool OrderBookL3::deleteOrder(OrderId order_id) noexcept {
    // Find order
    detail::Order* order = order_map_.find(order_id);
    if (!order) {
        return false;
    }

    const Timestamp timestamp = order->timestamp;
    const Price price = order->price;
    const Side side = order->side;

    // Get level
    detail::PriceLevelL3* level = getLevel(side, price);
    if (!level) {
        // Level doesn't exist - data structure inconsistency
        // Notify deletion, then clean up
        notifyOrderDelete(order, timestamp);
        order_map_.erase(order_id);
        order_pool_.destroy(order);
        return false;
    }

    // Remove from level
    level->removeOrder(order);
    const Quantity level_total = level->getTotalQuantity();

    // Remove from order map
    order_map_.erase(order_id);

    // Notify observers (before destroying order)
    notifyOrderDelete(order, timestamp);
    notifyPriceLevelUpdate(side, price, level_total, timestamp);

    // Remove level if empty
    removeLevelIfEmpty(side, price);

    // Destroy order
    order_pool_.destroy(order);

    // Notify ToB if changed
    notifyTopOfBookIfChanged(timestamp);

    return true;
}

bool OrderBookL3::executeOrder(OrderId order_id, Quantity executed_quantity) {
    detail::Order* order = order_map_.find(order_id);
    if (!order) {
        return false;
    }

    SLICK_ASSERT(executed_quantity > 0 && executed_quantity <= order->quantity);

    const Quantity remaining = order->quantity - executed_quantity;

    if (remaining == 0) {
        // Fully executed - delete order
        return deleteOrder(order_id);
    } else {
        // Partial execution - reduce quantity
        return modifyOrder(order_id, order->price, remaining);
    }
}

const detail::Order* OrderBookL3::findOrder(OrderId order_id) const noexcept {
    return order_map_.find(order_id);
}

const detail::PriceLevelL3* OrderBookL3::getBestBid() const noexcept {
    const auto& bids = levels_[Side::Buy];
    if (bids.empty()) {
        return nullptr;
    }
    // FlatMap is sorted by key (Price) in ascending order by default
    // For Buy side, we want highest price first, so use reverse iterator
    return &bids.rbegin()->second;
}

const detail::PriceLevelL3* OrderBookL3::getBestAsk() const noexcept {
    const auto& asks = levels_[Side::Sell];
    if (asks.empty()) {
        return nullptr;
    }
    // For Sell side, we want lowest price first (first element)
    return &asks.begin()->second;
}

TopOfBook OrderBookL3::getTopOfBook() const noexcept {
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

std::vector<detail::PriceLevelL2> OrderBookL3::getLevelsL2(Side side, std::size_t depth) const {
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
        auto it = level_map.rbegin();
        for (std::size_t i = 0; i < count && it != level_map.rend(); ++i, ++it) {
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

const detail::PriceLevelL3* OrderBookL3::getLevel(Side side, Price price) const noexcept {
    SLICK_ASSERT(side < SideCount);
    auto it = levels_[side].find(price);
    return (it != levels_[side].end()) ? &it->second : nullptr;
}

detail::PriceLevelL3* OrderBookL3::getLevel(Side side, Price price) noexcept {
    SLICK_ASSERT(side < SideCount);
    auto it = levels_[side].find(price);
    return (it != levels_[side].end()) ? &it->second : nullptr;
}

std::size_t OrderBookL3::levelCount(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    return levels_[side].size();
}

std::size_t OrderBookL3::orderCount(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    std::size_t count = 0;
    for (const auto& [price, level] : levels_[side]) {
        count += level.orderCount();
    }
    return count;
}

bool OrderBookL3::isEmpty(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    return levels_[side].empty();
}

bool OrderBookL3::isEmpty() const noexcept {
    return levels_[Side::Buy].empty() && levels_[Side::Sell].empty();
}

void OrderBookL3::clearSide(Side side) noexcept {
    SLICK_ASSERT(side < SideCount);

    // Delete all orders on this side
    auto& level_map = levels_[side];
    for (auto& [price, level] : level_map) {
        for (auto& order : level.orders) {
            order_map_.erase(order.order_id);
            order_pool_.destroy(&order);
        }
    }

    // Clear level map
    level_map.clear();
}

void OrderBookL3::clear() noexcept {
    clearSide(Side::Buy);
    clearSide(Side::Sell);
}

detail::PriceLevelL3* OrderBookL3::getOrCreateLevel(Side side, Price price) {
    SLICK_ASSERT(side < SideCount);

    auto& level_map = levels_[side];
    auto it = level_map.find(price);

    if (it != level_map.end()) {
        return &it->second;
    }

    // Create new level
    auto [new_it, inserted] = level_map.emplace(price, detail::PriceLevelL3{price});
    SLICK_ASSERT(inserted);
    return &new_it->second;
}

void OrderBookL3::removeLevelIfEmpty(Side side, Price price) noexcept {
    SLICK_ASSERT(side < SideCount);

    auto& level_map = levels_[side];
    auto it = level_map.find(price);

    if (it != level_map.end() && it->second.isEmpty()) {
        level_map.erase(it);
    }
}

void OrderBookL3::notifyOrderUpdate(const detail::Order* order, [[maybe_unused]] Quantity old_quantity,
                                    [[maybe_unused]] Price old_price, Timestamp timestamp) const {
    OrderUpdate update{
        symbol_,
        order->order_id,
        order->side,
        order->price,
        order->quantity,
        timestamp
    };
    observers_.notifyOrderUpdate(update);
}

void OrderBookL3::notifyOrderDelete(const detail::Order* order, Timestamp timestamp) const {
    OrderUpdate update{
        symbol_,
        order->order_id,
        order->side,
        order->price,
        0,  // quantity = 0 means delete
        timestamp
    };
    observers_.notifyOrderUpdate(update);
}

void OrderBookL3::notifyTrade(OrderId passive_order_id, OrderId aggressive_order_id,
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

void OrderBookL3::notifyPriceLevelUpdate(Side side, Price price, Quantity total_quantity,
                                         Timestamp timestamp) const {
    PriceLevelUpdate update{
        symbol_,
        side,
        price,
        total_quantity,
        timestamp
    };
    observers_.notifyPriceLevelUpdate(update);
}

void OrderBookL3::notifyTopOfBookIfChanged(Timestamp timestamp) {
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

SLICK_NAMESPACE_END
