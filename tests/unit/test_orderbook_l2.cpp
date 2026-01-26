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
