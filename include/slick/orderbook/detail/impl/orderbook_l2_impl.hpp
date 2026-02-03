// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

#ifndef SLICK_OB_INLINE
#define SLICK_OB_INLINE inline
#endif

SLICK_NAMESPACE_BEGIN

SLICK_OB_INLINE OrderBookL2::OrderBookL2(SymbolId symbol, std::size_t initial_capacity)
    : symbol_(symbol),
      sides_(std::array<detail::LevelContainer, 2>{
          detail::LevelContainer{Side::Buy, initial_capacity},
          detail::LevelContainer{Side::Sell, initial_capacity}
      }),
      observers_(),
      cached_tob_(),
      tob_seq_(0),
      last_seq_num_(0) {
    // Initialize cached top-of-book
    cached_tob_.symbol = symbol_;
}

// Move constructor - manually implement due to std::atomic member
SLICK_OB_INLINE OrderBookL2::OrderBookL2(OrderBookL2&& other) noexcept
    : symbol_(other.symbol_),
      sides_(std::move(other.sides_)),
      observers_(std::move(other.observers_)),
      cached_tob_(other.cached_tob_),
      tob_seq_(other.tob_seq_.load(std::memory_order_relaxed)),
      last_seq_num_(other.last_seq_num_) {
}

// Move assignment - manually implement due to std::atomic member
SLICK_OB_INLINE OrderBookL2& OrderBookL2::operator=(OrderBookL2&& other) noexcept {
    if (this != &other) {
        symbol_ = other.symbol_;
        sides_ = std::move(other.sides_);
        observers_ = std::move(other.observers_);
        cached_tob_ = other.cached_tob_;
        tob_seq_.store(other.tob_seq_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        last_seq_num_ = other.last_seq_num_;
    }
    return *this;
}

SLICK_OB_INLINE void OrderBookL2::updateLevel(Side side, Price price, Quantity quantity, Timestamp timestamp,
                                               uint64_t seq_num, bool is_last_in_batch) {
    SLICK_ASSERT(side < SideCount);

    // Validate sequence number - reject out-of-order (seq_num < last_seq_num)
    if (seq_num > 0) {
        if (seq_num < last_seq_num_) {
            // Out of order - reject silently
            return;
        }
        last_seq_num_ = seq_num;
    }

    if (quantity == 0) {
        // Find level index before deletion
        auto it = sides_[side].find(price);
        if (it != sides_[side].end()) {
            uint16_t level_idx = static_cast<uint16_t>(std::distance(sides_[side].begin(), it));

            // Delete the level
            sides_[side].erase(it);

            // Deletion: both price and quantity changed
            uint8_t change_flags = PriceChanged | QuantityChanged;
            if (is_last_in_batch) {
                change_flags |= LastInBatch;
            }

            // Notify with level_index, flags, and seq_num
            PriceLevelUpdate update{symbol_, side, price, 0, timestamp, level_idx, change_flags, seq_num};
            observers_.notifyPriceLevelUpdate(update);
            notifyTopOfBookIfChanged(timestamp, change_flags);
        }
    } else {
        // Insert or update level
        auto [it, inserted] = sides_[side].insertOrUpdate(price, quantity, timestamp);

        // Calculate level index
        uint16_t level_idx = static_cast<uint16_t>(std::distance(sides_[side].begin(), it));

        // Determine change flags based on what actually changed
        uint8_t change_flags = 0;
        if (inserted) {
            // New level: both price and quantity changed
            change_flags = PriceChanged | QuantityChanged;
        } else {
            // Existing level: only quantity changed
            change_flags = QuantityChanged;
        }

        // Add LastInBatch flag if this is the last update
        if (is_last_in_batch) {
            change_flags |= LastInBatch;
        }

        // Notify observers
        PriceLevelUpdate update{symbol_, side, price, quantity, timestamp, level_idx, change_flags, seq_num};
        observers_.notifyPriceLevelUpdate(update);
        notifyTopOfBookIfChanged(timestamp, change_flags);
    }
}

SLICK_OB_INLINE bool OrderBookL2::deleteLevel(Side side, Price price) noexcept {
    SLICK_ASSERT(side < SideCount);
    return sides_[side].erase(price);
}

SLICK_OB_INLINE void OrderBookL2::clearSide(Side side) noexcept {
    SLICK_ASSERT(side < SideCount);
    sides_[side].clear();
}

SLICK_OB_INLINE void OrderBookL2::clear() noexcept {
    sides_[Side::Buy].clear();
    sides_[Side::Sell].clear();
}

SLICK_OB_INLINE const detail::PriceLevelL2* OrderBookL2::getBestBid() const noexcept {
    return sides_[Side::Buy].best();
}

SLICK_OB_INLINE const detail::PriceLevelL2* OrderBookL2::getBestAsk() const noexcept {
    return sides_[Side::Sell].best();
}

SLICK_OB_INLINE TopOfBook OrderBookL2::getTopOfBook() const noexcept {
    // Use sequence lock to read cached top-of-book atomically
    TopOfBook tob;
    uint64_t seq1, seq2;
    do {
        seq1 = tob_seq_.load(std::memory_order_acquire);
        // If seq1 is odd, writer is in progress, spin
        while (seq1 & 1) {
            seq1 = tob_seq_.load(std::memory_order_acquire);
        }
        // Read the cached value
        tob = cached_tob_;
        // Check if a write occurred during read
        seq2 = tob_seq_.load(std::memory_order_acquire);
    } while (seq1 != seq2);
    return tob;
}

SLICK_OB_INLINE std::vector<detail::PriceLevelL2> OrderBookL2::getLevels(Side side, std::size_t depth) const {
    SLICK_ASSERT(side < SideCount);
    return sides_[side].getLevels(depth);
}

SLICK_OB_INLINE const detail::PriceLevelL2* OrderBookL2::getLevel(Side side, Price price) const noexcept {
    SLICK_ASSERT(side < SideCount);
    return sides_[side].getLevel(price);
}

SLICK_OB_INLINE const detail::PriceLevelL2* OrderBookL2::getLevelByIndex(Side side, uint16_t index) const noexcept {
    SLICK_ASSERT(side < SideCount);
    const auto& container = sides_[side];
    if (index >= container.size()) {
        return nullptr;
    }
    return &container[index];
}

SLICK_OB_INLINE std::size_t OrderBookL2::levelCount(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    return sides_[side].size();
}

SLICK_OB_INLINE bool OrderBookL2::isEmpty(Side side) const noexcept {
    SLICK_ASSERT(side < SideCount);
    return sides_[side].empty();
}

SLICK_OB_INLINE bool OrderBookL2::isEmpty() const noexcept {
    return sides_[Side::Buy].empty() && sides_[Side::Sell].empty();
}

SLICK_OB_INLINE void OrderBookL2::notifyTopOfBookIfChanged(Timestamp timestamp, uint8_t update_flags) {
    // Only emit ToB if LastInBatch flag is set
    // (defaults to true for single operations, false for intermediate batch updates)
    if ((update_flags & LastInBatch) == 0) {
        return;  // Skip ToB emission for intermediate batch updates
    }

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
        // Update cached values using sequence lock (odd = writing, even = readable)
        uint64_t seq = tob_seq_.load(std::memory_order_relaxed);
        tob_seq_.store(seq + 1, std::memory_order_release);  // Mark as writing (odd)

        cached_tob_.best_bid = new_best_bid;
        cached_tob_.bid_quantity = new_bid_qty;
        cached_tob_.best_ask = new_best_ask;
        cached_tob_.ask_quantity = new_ask_qty;
        cached_tob_.timestamp = timestamp;

        tob_seq_.store(seq + 2, std::memory_order_release);  // Mark as readable (even)

        // Notify observers
        observers_.notifyTopOfBookUpdate(cached_tob_);
    }
}

SLICK_OB_INLINE void OrderBookL2::emitSnapshot(Timestamp timestamp) {
    // Notify snapshot begin
    observers_.notifySnapshotBegin(symbol_, last_seq_num_, timestamp);

    // Emit all bid levels
    uint16_t level_idx = 0;
    for (const auto& level : sides_[Side::Buy]) {
        PriceLevelUpdate update{
            symbol_,
            Side::Buy,
            level.price,
            level.quantity,
            timestamp,
            level_idx++,
            static_cast<uint8_t>(PriceChanged | QuantityChanged)
        };
        observers_.notifyPriceLevelUpdate(update);
    }

    // Emit all ask levels
    level_idx = 0;
    for (const auto& level : sides_[Side::Sell]) {
        PriceLevelUpdate update{
            symbol_,
            Side::Sell,
            level.price,
            level.quantity,
            timestamp,
            level_idx++,
            static_cast<uint8_t>(PriceChanged | QuantityChanged)
        };
        observers_.notifyPriceLevelUpdate(update);
    }

    // Notify snapshot end
    observers_.notifySnapshotEnd(symbol_, last_seq_num_, timestamp);
}

SLICK_NAMESPACE_END
