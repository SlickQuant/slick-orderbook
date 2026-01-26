// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#pragma once

#include <slick/orderbook/config.hpp>
#include <slick/orderbook/events.hpp>
#include <slick/orderbook/concepts.hpp>
#include <vector>
#include <memory>

SLICK_NAMESPACE_BEGIN

/// Base interface for orderbook observers
/// Observers receive callbacks for orderbook events
///
/// Users should inherit from this class and implement the callback methods
/// they're interested in. Default implementations do nothing.
class IOrderBookObserver {
public:
    virtual ~IOrderBookObserver() = default;

    /// Called when a price level is updated (L2 event)
    virtual void onPriceLevelUpdate([[maybe_unused]] const PriceLevelUpdate& update) {
    }

    /// Called when an individual order is updated (L3 event)
    virtual void onOrderUpdate([[maybe_unused]] const OrderUpdate& update) {
    }

    /// Called when a trade occurs
    virtual void onTrade([[maybe_unused]] const Trade& trade) {
    }

    /// Called when top-of-book changes
    virtual void onTopOfBookUpdate([[maybe_unused]] const TopOfBook& tob) {
    }
};

/// Observer manager for notifying multiple observers
/// Uses type erasure to support both concept-based and inheritance-based observers
class ObserverManager {
public:
    ObserverManager() = default;
    ~ObserverManager() = default;

    // Non-copyable, movable
    ObserverManager(const ObserverManager&) = delete;
    ObserverManager& operator=(const ObserverManager&) = delete;
    ObserverManager(ObserverManager&&) noexcept = default;
    ObserverManager& operator=(ObserverManager&&) noexcept = default;

    /// Add an observer
    /// @param observer Shared pointer to observer (can be IOrderBookObserver or concept-based)
    void addObserver(std::shared_ptr<IOrderBookObserver> observer) {
        if (observer) {
            observers_.push_back(std::move(observer));
        }
    }

    /// Remove an observer
    /// @param observer Observer to remove
    /// @return true if observer was found and removed
    bool removeObserver(const std::shared_ptr<IOrderBookObserver>& observer) {
        auto it = std::find(observers_.begin(), observers_.end(), observer);
        if (it != observers_.end()) {
            observers_.erase(it);
            return true;
        }
        return false;
    }

    /// Clear all observers
    void clearObservers() noexcept {
        observers_.clear();
    }

    /// Get number of observers
    [[nodiscard]] std::size_t observerCount() const noexcept {
        return observers_.size();
    }

    /// Notify all observers of price level update
    void notifyPriceLevelUpdate(const PriceLevelUpdate& update) const {
        for (const auto& observer : observers_) {
            observer->onPriceLevelUpdate(update);
        }
    }

    /// Notify all observers of order update
    void notifyOrderUpdate(const OrderUpdate& update) const {
        for (const auto& observer : observers_) {
            observer->onOrderUpdate(update);
        }
    }

    /// Notify all observers of trade
    void notifyTrade(const Trade& trade) const {
        for (const auto& observer : observers_) {
            observer->onTrade(trade);
        }
    }

    /// Notify all observers of top-of-book update
    void notifyTopOfBookUpdate(const TopOfBook& tob) const {
        for (const auto& observer : observers_) {
            observer->onTopOfBookUpdate(tob);
        }
    }

private:
    std::vector<std::shared_ptr<IOrderBookObserver>> observers_;
};

SLICK_NAMESPACE_END
