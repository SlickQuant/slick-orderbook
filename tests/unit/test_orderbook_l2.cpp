// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_l2.hpp>
#include <gtest/gtest.h>
#include <memory>

using namespace slick::orderbook;

class OrderBookL2Test : public ::testing::Test {
protected:
    static constexpr SymbolId kSymbol = 1;
    static constexpr Price kPrice100 = 10000;
    static constexpr Price kPrice101 = 10100;
    static constexpr Price kPrice102 = 10200;
    static constexpr Price kPrice99 = 9900;
    static constexpr Price kPrice98 = 9800;
    static constexpr Quantity kQty10 = 10;
    static constexpr Quantity kQty20 = 20;
    static constexpr Quantity kQty30 = 30;
    static constexpr Quantity kQty40 = 40;
    static constexpr Timestamp kTs1 = 1000;
    static constexpr Timestamp kTs2 = 2000;
};

TEST_F(OrderBookL2Test, InitialState) {
    OrderBookL2 book(kSymbol);

    EXPECT_EQ(book.symbol(), kSymbol);
    EXPECT_TRUE(book.isEmpty());
    EXPECT_TRUE(book.isEmpty(Side::Buy));
    EXPECT_TRUE(book.isEmpty(Side::Sell));
    EXPECT_EQ(book.levelCount(Side::Buy), 0);
    EXPECT_EQ(book.levelCount(Side::Sell), 0);
    EXPECT_EQ(book.getBestBid(), nullptr);
    EXPECT_EQ(book.getBestAsk(), nullptr);
}

TEST_F(OrderBookL2Test, AddSingleBidLevel) {
    OrderBookL2 book(kSymbol);

    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);

    EXPECT_FALSE(book.isEmpty(Side::Buy));
    EXPECT_EQ(book.levelCount(Side::Buy), 1);

    const auto* best_bid = book.getBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price, kPrice100);
    EXPECT_EQ(best_bid->quantity, kQty10);
    EXPECT_EQ(best_bid->timestamp, kTs1);
}

TEST_F(OrderBookL2Test, AddSingleAskLevel) {
    OrderBookL2 book(kSymbol);

    book.updateLevel(Side::Sell, kPrice100, kQty10, kTs1);

    EXPECT_FALSE(book.isEmpty(Side::Sell));
    EXPECT_EQ(book.levelCount(Side::Sell), 1);

    const auto* best_ask = book.getBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->price, kPrice100);
    EXPECT_EQ(best_ask->quantity, kQty10);
}

TEST_F(OrderBookL2Test, BidLevelsSortedDescending) {
    OrderBookL2 book(kSymbol);

    // Add in random order
    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Buy, kPrice102, kQty20, kTs1);
    book.updateLevel(Side::Buy, kPrice99, kQty30, kTs1);

    EXPECT_EQ(book.levelCount(Side::Buy), 3);

    // Best bid should be highest price
    const auto* best_bid = book.getBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price, kPrice102);

    // Get all levels - should be sorted descending
    auto levels = book.getLevels(Side::Buy);
    ASSERT_EQ(levels.size(), 3);
    EXPECT_EQ(levels[0].price, kPrice102);
    EXPECT_EQ(levels[1].price, kPrice100);
    EXPECT_EQ(levels[2].price, kPrice99);
}

TEST_F(OrderBookL2Test, AskLevelsSortedAscending) {
    OrderBookL2 book(kSymbol);

    // Add in random order
    book.updateLevel(Side::Sell, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Sell, kPrice102, kQty20, kTs1);
    book.updateLevel(Side::Sell, kPrice99, kQty30, kTs1);

    EXPECT_EQ(book.levelCount(Side::Sell), 3);

    // Best ask should be lowest price
    const auto* best_ask = book.getBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->price, kPrice99);

    // Get all levels - should be sorted ascending
    auto levels = book.getLevels(Side::Sell);
    ASSERT_EQ(levels.size(), 3);
    EXPECT_EQ(levels[0].price, kPrice99);
    EXPECT_EQ(levels[1].price, kPrice100);
    EXPECT_EQ(levels[2].price, kPrice102);
}

TEST_F(OrderBookL2Test, UpdateExistingLevel) {
    OrderBookL2 book(kSymbol);

    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Buy, kPrice100, kQty20, kTs2);

    EXPECT_EQ(book.levelCount(Side::Buy), 1);

    const auto* best_bid = book.getBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price, kPrice100);
    EXPECT_EQ(best_bid->quantity, kQty20);
    EXPECT_EQ(best_bid->timestamp, kTs2);
}

TEST_F(OrderBookL2Test, DeleteLevelWithZeroQuantity) {
    OrderBookL2 book(kSymbol);

    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Buy, kPrice101, kQty20, kTs1);

    EXPECT_EQ(book.levelCount(Side::Buy), 2);

    // Delete with quantity=0
    book.updateLevel(Side::Buy, kPrice100, 0, kTs2);

    EXPECT_EQ(book.levelCount(Side::Buy), 1);

    const auto* best_bid = book.getBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price, kPrice101);
}

TEST_F(OrderBookL2Test, DeleteLevelExplicit) {
    OrderBookL2 book(kSymbol);

    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Buy, kPrice101, kQty20, kTs1);

    bool deleted = book.deleteLevel(Side::Buy, kPrice100);
    EXPECT_TRUE(deleted);
    EXPECT_EQ(book.levelCount(Side::Buy), 1);

    // Try deleting non-existent level
    deleted = book.deleteLevel(Side::Buy, kPrice100);
    EXPECT_FALSE(deleted);
}

TEST_F(OrderBookL2Test, GetTopOfBook) {
    OrderBookL2 book(kSymbol);

    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Buy, kPrice99, kQty20, kTs1);
    book.updateLevel(Side::Sell, kPrice101, kQty30, kTs2);
    book.updateLevel(Side::Sell, kPrice102, kQty40, kTs2);

    auto tob = book.getTopOfBook();

    EXPECT_EQ(tob.symbol, kSymbol);
    EXPECT_EQ(tob.best_bid, kPrice100);
    EXPECT_EQ(tob.bid_quantity, kQty10);
    EXPECT_EQ(tob.best_ask, kPrice101);
    EXPECT_EQ(tob.ask_quantity, kQty30);
    EXPECT_EQ(tob.spread(), kPrice101 - kPrice100);
    EXPECT_TRUE(tob.isValid());
    EXPECT_FALSE(tob.isCrossed());
}

TEST_F(OrderBookL2Test, GetLevelsWithDepth) {
    OrderBookL2 book(kSymbol);

    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Buy, kPrice99, kQty20, kTs1);
    book.updateLevel(Side::Buy, kPrice98, kQty30, kTs1);

    // Get top 2 levels
    auto levels = book.getLevels(Side::Buy, 2);
    ASSERT_EQ(levels.size(), 2);
    EXPECT_EQ(levels[0].price, kPrice100);
    EXPECT_EQ(levels[1].price, kPrice99);

    // Get all levels (depth=0)
    levels = book.getLevels(Side::Buy, 0);
    EXPECT_EQ(levels.size(), 3);
}

TEST_F(OrderBookL2Test, ClearSide) {
    OrderBookL2 book(kSymbol);

    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Buy, kPrice99, kQty20, kTs1);
    book.updateLevel(Side::Sell, kPrice101, kQty30, kTs1);

    book.clearSide(Side::Buy);

    EXPECT_TRUE(book.isEmpty(Side::Buy));
    EXPECT_FALSE(book.isEmpty(Side::Sell));
    EXPECT_EQ(book.levelCount(Side::Buy), 0);
    EXPECT_EQ(book.levelCount(Side::Sell), 1);
}

TEST_F(OrderBookL2Test, ClearAll) {
    OrderBookL2 book(kSymbol);

    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Sell, kPrice101, kQty30, kTs1);

    book.clear();

    EXPECT_TRUE(book.isEmpty());
    EXPECT_EQ(book.levelCount(Side::Buy), 0);
    EXPECT_EQ(book.levelCount(Side::Sell), 0);
}

TEST_F(OrderBookL2Test, ObserverNotifications) {
    class TestObserver : public IOrderBookObserver {
    public:
        int price_level_updates = 0;
        int tob_updates = 0;

        void onPriceLevelUpdate([[maybe_unused]] const PriceLevelUpdate& update) override {
            ++price_level_updates;
        }

        void onTopOfBookUpdate([[maybe_unused]] const TopOfBook& tob) override {
            ++tob_updates;
        }
    };

    OrderBookL2 book(kSymbol);
    auto observer = std::make_shared<TestObserver>();
    book.addObserver(observer);

    EXPECT_EQ(book.observerCount(), 1);

    // Add level should trigger notifications
    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    EXPECT_EQ(observer->price_level_updates, 1);
    EXPECT_EQ(observer->tob_updates, 1);  // ToB changed

    // Update existing level should trigger notifications
    book.updateLevel(Side::Buy, kPrice100, kQty20, kTs2);
    EXPECT_EQ(observer->price_level_updates, 2);
    EXPECT_EQ(observer->tob_updates, 2);  // ToB quantity changed

    // Add level that doesn't change ToB
    book.updateLevel(Side::Buy, kPrice99, kQty30, kTs2);
    EXPECT_EQ(observer->price_level_updates, 3);
    EXPECT_EQ(observer->tob_updates, 2);  // ToB unchanged (best bid still 100)

    // Delete non-existent level should not trigger notifications
    book.updateLevel(Side::Buy, 5000, 0, kTs2);
    EXPECT_EQ(observer->price_level_updates, 3);  // No notification

    // Remove observer
    bool removed = book.removeObserver(observer);
    EXPECT_TRUE(removed);
    EXPECT_EQ(book.observerCount(), 0);
}

TEST_F(OrderBookL2Test, Move) {
    OrderBookL2 book1(kSymbol);
    book1.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);

    // Move constructor
    OrderBookL2 book2(std::move(book1));
    EXPECT_EQ(book2.symbol(), kSymbol);
    EXPECT_EQ(book2.levelCount(Side::Buy), 1);

    // Move assignment
    OrderBookL2 book3(2);
    book3 = std::move(book2);
    EXPECT_EQ(book3.symbol(), kSymbol);
    EXPECT_EQ(book3.levelCount(Side::Buy), 1);
}

// Batch flag tests
class BatchObserverL2 : public IOrderBookObserver {
public:
    std::vector<PriceLevelUpdate> level_updates;
    std::vector<TopOfBook> tob_updates;

    void onPriceLevelUpdate(const PriceLevelUpdate& update) override {
        level_updates.push_back(update);
    }

    void onTopOfBookUpdate(const TopOfBook& tob) override {
        tob_updates.push_back(tob);
    }

    void reset() {
        level_updates.clear();
        tob_updates.clear();
    }
};

TEST_F(OrderBookL2Test, BatchFlagSingleOperation) {
    OrderBookL2 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL2>();
    book.addObserver(observer);

    // Single operation with default is_last_in_batch = true
    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);

    // Should receive 1 level update and 1 ToB update
    ASSERT_EQ(observer->level_updates.size(), 1);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Check flags
    const auto& update = observer->level_updates[0];
    EXPECT_TRUE(update.isLastInBatch());
    EXPECT_TRUE(update.priceChanged());
    EXPECT_TRUE(update.quantityChanged());
}

TEST_F(OrderBookL2Test, BatchFlagMultipleUpdates) {
    OrderBookL2 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL2>();
    book.addObserver(observer);

    // Batch of 3 updates - only last one should trigger ToB
    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1, false);  // Not last
    book.updateLevel(Side::Buy, kPrice101, kQty20, kTs1, false);  // Not last
    book.updateLevel(Side::Buy, kPrice102, kQty30, kTs1, true);   // Last

    // Should receive 3 level updates but only 1 ToB update (at the end)
    ASSERT_EQ(observer->level_updates.size(), 3);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Check flags on each update
    EXPECT_FALSE(observer->level_updates[0].isLastInBatch());
    EXPECT_FALSE(observer->level_updates[1].isLastInBatch());
    EXPECT_TRUE(observer->level_updates[2].isLastInBatch());

    // ToB should reflect the final state (best bid = 10200)
    EXPECT_EQ(observer->tob_updates[0].best_bid, kPrice102);
    EXPECT_EQ(observer->tob_updates[0].bid_quantity, kQty30);
}

TEST_F(OrderBookL2Test, BatchFlagIntermediateUpdates) {
    OrderBookL2 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL2>();
    book.addObserver(observer);

    // Add initial levels
    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Sell, kPrice101, kQty10, kTs1);
    observer->reset();

    // Batch update: modify quantities without triggering ToB until end
    book.updateLevel(Side::Buy, kPrice100, kQty20, kTs2, false);   // Qty change only, not last
    book.updateLevel(Side::Sell, kPrice101, kQty20, kTs2, true);   // Qty change only, last

    // Should receive 2 level updates and 1 ToB update
    ASSERT_EQ(observer->level_updates.size(), 2);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // First update should only have QuantityChanged flag (no PriceChanged)
    EXPECT_FALSE(observer->level_updates[0].priceChanged());
    EXPECT_TRUE(observer->level_updates[0].quantityChanged());
    EXPECT_FALSE(observer->level_updates[0].isLastInBatch());

    // Second update should have both flags
    EXPECT_FALSE(observer->level_updates[1].priceChanged());
    EXPECT_TRUE(observer->level_updates[1].quantityChanged());
    EXPECT_TRUE(observer->level_updates[1].isLastInBatch());
}

TEST_F(OrderBookL2Test, BatchFlagDeletion) {
    OrderBookL2 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL2>();
    book.addObserver(observer);

    // Add levels
    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    book.updateLevel(Side::Buy, kPrice99, kQty20, kTs1);
    observer->reset();

    // Delete in batch
    book.updateLevel(Side::Buy, kPrice100, 0, kTs2, false);  // Delete, not last
    book.updateLevel(Side::Buy, kPrice99, 0, kTs2, true);    // Delete, last

    // Should receive 2 deletions and 1 ToB update
    ASSERT_EQ(observer->level_updates.size(), 2);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Both deletions should have PriceChanged | QuantityChanged
    EXPECT_TRUE(observer->level_updates[0].isDelete());
    EXPECT_TRUE(observer->level_updates[0].priceChanged());
    EXPECT_TRUE(observer->level_updates[0].quantityChanged());
    EXPECT_FALSE(observer->level_updates[0].isLastInBatch());

    EXPECT_TRUE(observer->level_updates[1].isDelete());
    EXPECT_TRUE(observer->level_updates[1].priceChanged());
    EXPECT_TRUE(observer->level_updates[1].quantityChanged());
    EXPECT_TRUE(observer->level_updates[1].isLastInBatch());

    // ToB should reflect empty book
    EXPECT_EQ(observer->tob_updates[0].best_bid, 0);
    EXPECT_EQ(observer->tob_updates[0].bid_quantity, 0);
}

TEST_F(OrderBookL2Test, BatchFlagMixedOperations) {
    OrderBookL2 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL2>();
    book.addObserver(observer);

    // Mixed operations in a batch: add, modify, delete
    book.updateLevel(Side::Buy, kPrice100, kQty10, kTs1, false);   // Add new level
    book.updateLevel(Side::Buy, kPrice100, kQty20, kTs1, false);   // Modify (qty only)
    book.updateLevel(Side::Buy, kPrice101, kQty30, kTs1, false);   // Add new level
    book.updateLevel(Side::Buy, kPrice100, 0, kTs1, true);         // Delete, last

    // Should receive 4 level updates and 1 ToB update
    ASSERT_EQ(observer->level_updates.size(), 4);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Check flags
    EXPECT_TRUE(observer->level_updates[0].priceChanged());   // New level
    EXPECT_TRUE(observer->level_updates[0].quantityChanged());
    EXPECT_FALSE(observer->level_updates[0].isLastInBatch());

    EXPECT_FALSE(observer->level_updates[1].priceChanged());  // Qty change only
    EXPECT_TRUE(observer->level_updates[1].quantityChanged());
    EXPECT_FALSE(observer->level_updates[1].isLastInBatch());

    EXPECT_TRUE(observer->level_updates[2].priceChanged());   // New level
    EXPECT_TRUE(observer->level_updates[2].quantityChanged());
    EXPECT_FALSE(observer->level_updates[2].isLastInBatch());

    EXPECT_TRUE(observer->level_updates[3].priceChanged());   // Deletion
    EXPECT_TRUE(observer->level_updates[3].quantityChanged());
    EXPECT_TRUE(observer->level_updates[3].isLastInBatch());

    // Final ToB should show best bid at 10100
    EXPECT_EQ(observer->tob_updates[0].best_bid, kPrice101);
}
