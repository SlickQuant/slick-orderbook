// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_l2.hpp>

SLICK_NAMESPACE_BEGIN

OrderBookL2::OrderBookL2(SymbolId symbol, std::size_t initial_capacity)
    : symbol_(symbol),
      sides_{detail::LevelContainer{Side::Buy, initial_capacity},
             detail::LevelContainer{Side::Sell, initial_capacity}},
      observers_(),
      cached_tob_() {
    // Initialize cached top-of-book
    cached_tob_.symbol = symbol_;
}

void OrderBookL2::updateLevel(Side side, Price price, Quantity quantity, Timestamp timestamp) {
    SLICK_ASSERT(side < SideCount);

    if (quantity == 0) {
        // Delete level - only notify if it existed
        if (deleteLevel(side, price)) {
            notifyPriceLevelUpdate(side, price, 0, timestamp);
            notifyTopOfBookIfChanged(timestamp);
        }
    } else {
        // Insert or update level
        sides_[side].insertOrUpdate(price, quantity, timestamp);

        // Notify observers
        notifyPriceLevelUpdate(side, price, quantity, timestamp);
        notifyTopOfBookIfChanged(timestamp);
    }
}

bool OrderBookL2::deleteLevel(Side side, Price price) noexcept {
    SLICK_ASSERT(side < SideCount);
    return sides_[side].erase(price);
}

void OrderBookL2::clearSide(Side side) noexcept {
    SLICK_ASSERT(side < SideCount);
    sides_[side].clear();
}

void OrderBookL2::clear() noexcept {
    sides_[Side::Buy].clear();
    sides_[Side::Sell].clear();
}

const detail::PriceLevelL2* OrderBookL2::getBestBid() const noexcept {
    return sides_[Side::Buy].best();
}

const detail::PriceLevelL2* OrderBookL2::getBestAsk() const noexcept {
    return sides_[Side::Sell].best();
}

TopOfBook OrderBookL2::getTopOfBook() const noexcept {
    const auto* bid = getBestBid();
    const auto* ask = getBestAsk();

    TopOfBook tob;
    tob.symbol = symbol_;

    if (bid) {
        tob.best_bid = bid->price;
        tob.bid_quantity = bid->quantity;
        tob.timestamp = bid->timestamp;
    }

    if (ask) {
        tob.best_ask = ask->price;
        tob.ask_quantity = ask->quantity;
        if (ask->timestamp > tob.timestamp) {
            tob.timestamp = ask->timestamp;
        }
    }

    return tob;
}

std::vector<detail::PriceLevelL2> OrderBookL2::getLevels(Side side, std::size_t depth) const {
    SLICK_ASSERT(side < SideCount);
    return sides_[side].getLevels(depth);
}

std::size_t OrderBookL2::levelCount(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    return sides_[side].size();
}

bool OrderBookL2::isEmpty(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    return sides_[side].empty();
}

bool OrderBookL2::isEmpty() const noexcept {
    return sides_[Side::Buy].empty() && sides_[Side::Sell].empty();
}

void OrderBookL2::notifyPriceLevelUpdate(Side side, Price price, Quantity quantity, Timestamp timestamp) const {
    PriceLevelUpdate update{symbol_, side, price, quantity, timestamp};
    observers_.notifyPriceLevelUpdate(update);
}

void OrderBookL2::notifyTopOfBookIfChanged(Timestamp timestamp) {
    // Compute current top-of-book
    const auto* bid = getBestBid();
    const auto* ask = getBestAsk();

    Price new_best_bid = bid ? bid->price : 0;
    Quantity new_bid_qty = bid ? bid->quantity : 0;
    Price new_best_ask = ask ? ask->price : 0;
    Quantity new_ask_qty = ask ? ask->quantity : 0;

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
