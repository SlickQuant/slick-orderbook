// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_l3.hpp>
#include <gtest/gtest.h>

using namespace slick::orderbook;

class OrderBookL3Test : public ::testing::Test {
protected:
    static constexpr SymbolId kSymbol = 12345;
    static constexpr OrderId kOrder1 = 1001;
    static constexpr OrderId kOrder2 = 1002;
    static constexpr OrderId kOrder3 = 1003;
    static constexpr OrderId kOrder4 = 1004;
    static constexpr OrderId kOrder5 = 1005;
    static constexpr Price kPrice100 = 10000;
    static constexpr Price kPrice101 = 10100;
    static constexpr Price kPrice102 = 10200;
    static constexpr Price kPrice99 = 9900;
    static constexpr Price kPrice98 = 9800;
    static constexpr Quantity kQty10 = 10;
    static constexpr Quantity kQty20 = 20;
    static constexpr Quantity kQty30 = 30;
    static constexpr Quantity kQty40 = 40;
    static constexpr Quantity kQty50 = 50;
    static constexpr Timestamp kTs1 = 1000;
    static constexpr Timestamp kTs2 = 2000;
    static constexpr Timestamp kTs3 = 3000;
    static constexpr Timestamp kTs4 = 4000;
    static constexpr uint64_t kPriority1 = 100;
    static constexpr uint64_t kPriority2 = 200;
    static constexpr uint64_t kPriority3 = 300;
};

// ============================================================================
// Initial State Tests
// ============================================================================

TEST_F(OrderBookL3Test, InitialState) {
    OrderBookL3 book(kSymbol);

    EXPECT_EQ(book.symbol(), kSymbol);
    EXPECT_TRUE(book.isEmpty());
    EXPECT_TRUE(book.isEmpty(Side::Buy));
    EXPECT_TRUE(book.isEmpty(Side::Sell));
    EXPECT_EQ(book.orderCount(), 0);
    EXPECT_EQ(book.orderCount(Side::Buy), 0);
    EXPECT_EQ(book.orderCount(Side::Sell), 0);
    EXPECT_EQ(book.levelCount(Side::Buy), 0);
    EXPECT_EQ(book.levelCount(Side::Sell), 0);
    EXPECT_EQ(book.getBestBid(), nullptr);
    EXPECT_EQ(book.getBestAsk(), nullptr);
}

// ============================================================================
// Add Order Tests
// ============================================================================

TEST_F(OrderBookL3Test, AddSingleBid) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));

    EXPECT_FALSE(book.isEmpty());
    EXPECT_FALSE(book.isEmpty(Side::Buy));
    EXPECT_TRUE(book.isEmpty(Side::Sell));
    EXPECT_EQ(book.orderCount(), 1);
    EXPECT_EQ(book.orderCount(Side::Buy), 1);
    EXPECT_EQ(book.levelCount(Side::Buy), 1);

    const auto* best_bid = book.getBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price, kPrice100);
    EXPECT_EQ(best_bid->getTotalQuantity(), kQty10);
    EXPECT_EQ(best_bid->orderCount(), 1);
}

TEST_F(OrderBookL3Test, AddSingleAsk) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Sell, kPrice100, kQty10, kTs1));

    EXPECT_FALSE(book.isEmpty());
    EXPECT_TRUE(book.isEmpty(Side::Buy));
    EXPECT_FALSE(book.isEmpty(Side::Sell));
    EXPECT_EQ(book.orderCount(), 1);
    EXPECT_EQ(book.orderCount(Side::Sell), 1);
    EXPECT_EQ(book.levelCount(Side::Sell), 1);

    const auto* best_ask = book.getBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->price, kPrice100);
    EXPECT_EQ(best_ask->getTotalQuantity(), kQty10);
    EXPECT_EQ(best_ask->orderCount(), 1);
}

TEST_F(OrderBookL3Test, AddDuplicateOrderId) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_FALSE(book.addOrder(kOrder1, Side::Buy, kPrice101, kQty20, kTs2));  // Same OrderId

    EXPECT_EQ(book.orderCount(), 1);

    // Verify order was NOT modified
    const auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->price, kPrice100);
    EXPECT_EQ(order->quantity, kQty10);
}

TEST_F(OrderBookL3Test, AddOrModifyOrderIdempotent) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrModifyOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, kTs1));  // priority = timestamp
    EXPECT_TRUE(book.addOrModifyOrder(kOrder1, Side::Buy, kPrice101, kQty20, kTs2, kTs2));  // Same OrderId - modifies

    EXPECT_EQ(book.orderCount(), 1);

    // Verify order WAS modified
    const auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->price, kPrice101);
    EXPECT_EQ(order->quantity, kQty20);
}

TEST_F(OrderBookL3Test, AddMultipleOrdersAtSamePrice) {
    OrderBookL3 book(kSymbol);

    // Add three orders at same price with different priorities
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, kPriority2));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice100, kQty20, kTs2, kPriority1));  // Higher priority
    EXPECT_TRUE(book.addOrder(kOrder3, Side::Buy, kPrice100, kQty30, kTs3, kPriority3));

    EXPECT_EQ(book.orderCount(), 3);
    EXPECT_EQ(book.levelCount(Side::Buy), 1);

    const auto* level = book.getBestBid();
    ASSERT_NE(level, nullptr);
    EXPECT_EQ(level->getTotalQuantity(), kQty10 + kQty20 + kQty30);
    EXPECT_EQ(level->orderCount(), 3);

    // Best order should be Order2 (lowest priority value = highest priority)
    const auto* best_order = level->getBestOrder();
    ASSERT_NE(best_order, nullptr);
    EXPECT_EQ(best_order->order_id, kOrder2);
    EXPECT_EQ(best_order->priority, kPriority1);
}

TEST_F(OrderBookL3Test, AddMultiplePriceLevels) {
    OrderBookL3 book(kSymbol);

    // Add orders at different price levels
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs2));
    EXPECT_TRUE(book.addOrder(kOrder3, Side::Buy, kPrice99, kQty30, kTs3));

    EXPECT_EQ(book.orderCount(), 3);
    EXPECT_EQ(book.levelCount(Side::Buy), 3);

    // Best bid should be highest price (101)
    const auto* best_bid = book.getBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price, kPrice101);
    EXPECT_EQ(best_bid->getTotalQuantity(), kQty20);
}

// ============================================================================
// Find Order Tests
// ============================================================================

TEST_F(OrderBookL3Test, FindOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));

    const auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->order_id, kOrder1);
    EXPECT_EQ(order->price, kPrice100);
    EXPECT_EQ(order->quantity, kQty10);
    EXPECT_EQ(order->side, Side::Buy);
    EXPECT_EQ(order->timestamp, kTs1);
}

TEST_F(OrderBookL3Test, FindNonExistentOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));

    const auto* order = book.findOrder(kOrder2);
    EXPECT_EQ(order, nullptr);
}

// ============================================================================
// Modify Order Tests
// ============================================================================

TEST_F(OrderBookL3Test, ModifyOrderQuantity) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice100, kQty20));  // Increase quantity

    const auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->quantity, kQty20);

    const auto* level = book.getBestBid();
    ASSERT_NE(level, nullptr);
    EXPECT_EQ(level->getTotalQuantity(), kQty20);
}

TEST_F(OrderBookL3Test, ModifyOrderPrice) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice101, kQty10));  // Change price

    const auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->price, kPrice101);

    // Old level should be gone
    const auto *old_level = book.getLevel(Side::Buy, kPrice100).first;
    EXPECT_EQ(old_level, nullptr);

    // New level should exist
    const auto *new_level = book.getLevel(Side::Buy, kPrice101).first;
    ASSERT_NE(new_level, nullptr);
    EXPECT_EQ(new_level->getTotalQuantity(), kQty10);
}

TEST_F(OrderBookL3Test, ModifyOrderPriceAndQuantity) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice101, kQty20));  // Change both

    const auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->price, kPrice101);
    EXPECT_EQ(order->quantity, kQty20);

    const auto *new_level = book.getLevel(Side::Buy, kPrice101).first;
    ASSERT_NE(new_level, nullptr);
    EXPECT_EQ(new_level->getTotalQuantity(), kQty20);
}

TEST_F(OrderBookL3Test, ModifyOrderToZeroQuantityDeletes) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice100, 0));  // Delete via quantity=0

    EXPECT_EQ(book.orderCount(), 0);
    EXPECT_TRUE(book.isEmpty());
    EXPECT_EQ(book.findOrder(kOrder1), nullptr);
}

TEST_F(OrderBookL3Test, AddOrModifyZeroQuantityDeletes) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrModifyOrder(kOrder1, Side::Buy, kPrice100, 0, kTs2, kTs2));  // priority = timestamp

    EXPECT_EQ(book.orderCount(), 0);
    EXPECT_TRUE(book.isEmpty());
    EXPECT_EQ(book.findOrder(kOrder1), nullptr);
}

TEST_F(OrderBookL3Test, AddOrModifyZeroQuantityNonExistentReturnsFalse) {
    OrderBookL3 book(kSymbol);

    EXPECT_FALSE(book.addOrModifyOrder(kOrder1, Side::Buy, kPrice100, 0, kTs1, kTs1));  // priority = timestamp
    EXPECT_EQ(book.orderCount(), 0);
    EXPECT_TRUE(book.isEmpty());
}

TEST_F(OrderBookL3Test, ModifyNonExistentOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_FALSE(book.modifyOrder(kOrder1, kPrice100, kQty10));
}

TEST_F(OrderBookL3Test, ModifyOrderNoChange) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice100, kQty10));  // No change

    const auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->price, kPrice100);
    EXPECT_EQ(order->quantity, kQty10);
}

// ============================================================================
// Delete Order Tests
// ============================================================================

TEST_F(OrderBookL3Test, DeleteOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.deleteOrder(kOrder1));

    EXPECT_EQ(book.orderCount(), 0);
    EXPECT_TRUE(book.isEmpty());
    EXPECT_EQ(book.findOrder(kOrder1), nullptr);
    EXPECT_EQ(book.getBestBid(), nullptr);
}

TEST_F(OrderBookL3Test, DeleteNonExistentOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_FALSE(book.deleteOrder(kOrder1));
}

TEST_F(OrderBookL3Test, DeleteOrderLeavesOthersIntact) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice100, kQty20, kTs2));
    EXPECT_TRUE(book.deleteOrder(kOrder1));

    EXPECT_EQ(book.orderCount(), 1);
    EXPECT_NE(book.findOrder(kOrder2), nullptr);

    const auto* level = book.getBestBid();
    ASSERT_NE(level, nullptr);
    EXPECT_EQ(level->getTotalQuantity(), kQty20);
}

TEST_F(OrderBookL3Test, DeleteLastOrderRemovesLevel) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.deleteOrder(kOrder1));

    EXPECT_EQ(book.levelCount(Side::Buy), 0);
    EXPECT_EQ(book.getLevel(Side::Buy, kPrice100).first, nullptr);
}

// ============================================================================
// Execute Order Tests
// ============================================================================

TEST_F(OrderBookL3Test, ExecuteOrderPartially) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty30, kTs1));
    EXPECT_TRUE(book.executeOrder(kOrder1, kQty10));  // Execute 10 out of 30

    const auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->quantity, kQty20);  // 30 - 10 = 20

    const auto* level = book.getBestBid();
    ASSERT_NE(level, nullptr);
    EXPECT_EQ(level->getTotalQuantity(), kQty20);
}

TEST_F(OrderBookL3Test, ExecuteOrderFully) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.executeOrder(kOrder1, kQty10));  // Execute fully

    EXPECT_EQ(book.orderCount(), 0);
    EXPECT_TRUE(book.isEmpty());
    EXPECT_EQ(book.findOrder(kOrder1), nullptr);
}

TEST_F(OrderBookL3Test, ExecuteNonExistentOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_FALSE(book.executeOrder(kOrder1, kQty10));
}

// ============================================================================
// Top of Book Tests
// ============================================================================

TEST_F(OrderBookL3Test, TopOfBookEmpty) {
    OrderBookL3 book(kSymbol);

    TopOfBook tob = book.getTopOfBook();
    EXPECT_EQ(tob.symbol, kSymbol);
    EXPECT_EQ(tob.best_bid, 0);
    EXPECT_EQ(tob.best_ask, 0);
    EXPECT_EQ(tob.bid_quantity, 0);
    EXPECT_EQ(tob.ask_quantity, 0);
}

TEST_F(OrderBookL3Test, TopOfBookBidOnly) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));

    TopOfBook tob = book.getTopOfBook();
    EXPECT_EQ(tob.best_bid, kPrice100);
    EXPECT_EQ(tob.bid_quantity, kQty10);
    EXPECT_EQ(tob.best_ask, 0);
    EXPECT_EQ(tob.ask_quantity, 0);
}

TEST_F(OrderBookL3Test, TopOfBookAskOnly) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Sell, kPrice100, kQty10, kTs1));

    TopOfBook tob = book.getTopOfBook();
    EXPECT_EQ(tob.best_bid, 0);
    EXPECT_EQ(tob.bid_quantity, 0);
    EXPECT_EQ(tob.best_ask, kPrice100);
    EXPECT_EQ(tob.ask_quantity, kQty10);
}

TEST_F(OrderBookL3Test, TopOfBookBidAndAsk) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice99, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Sell, kPrice101, kQty20, kTs2));

    TopOfBook tob = book.getTopOfBook();
    EXPECT_EQ(tob.best_bid, kPrice99);
    EXPECT_EQ(tob.bid_quantity, kQty10);
    EXPECT_EQ(tob.best_ask, kPrice101);
    EXPECT_EQ(tob.ask_quantity, kQty20);
}

// ============================================================================
// L2 Aggregation Tests
// ============================================================================

TEST_F(OrderBookL3Test, GetLevelsL2Empty) {
    OrderBookL3 book(kSymbol);

    auto levels = book.getLevelsL2(Side::Buy, 0);
    EXPECT_TRUE(levels.empty());
}

TEST_F(OrderBookL3Test, GetLevelsL2SingleLevel) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice100, kQty20, kTs2));

    auto levels = book.getLevelsL2(Side::Buy, 0);
    ASSERT_EQ(levels.size(), 1);
    EXPECT_EQ(levels[0].price, kPrice100);
    EXPECT_EQ(levels[0].quantity, kQty30);  // Aggregated: 10 + 20
}

TEST_F(OrderBookL3Test, GetLevelsL2MultipleLevels) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice101, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice100, kQty20, kTs2));
    EXPECT_TRUE(book.addOrder(kOrder3, Side::Buy, kPrice99, kQty30, kTs3));

    auto levels = book.getLevelsL2(Side::Buy, 0);
    ASSERT_EQ(levels.size(), 3);

    // Should be sorted descending for bids (highest price first)
    EXPECT_EQ(levels[0].price, kPrice101);
    EXPECT_EQ(levels[0].quantity, kQty10);
    EXPECT_EQ(levels[1].price, kPrice100);
    EXPECT_EQ(levels[1].quantity, kQty20);
    EXPECT_EQ(levels[2].price, kPrice99);
    EXPECT_EQ(levels[2].quantity, kQty30);
}

TEST_F(OrderBookL3Test, GetLevelsL2WithDepth) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice101, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice100, kQty20, kTs2));
    EXPECT_TRUE(book.addOrder(kOrder3, Side::Buy, kPrice99, kQty30, kTs3));

    auto levels = book.getLevelsL2(Side::Buy, 2);  // Top 2 levels only
    ASSERT_EQ(levels.size(), 2);
    EXPECT_EQ(levels[0].price, kPrice101);
    EXPECT_EQ(levels[1].price, kPrice100);
}

TEST_F(OrderBookL3Test, GetLevelsL2AsksAscending) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Sell, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Sell, kPrice101, kQty20, kTs2));
    EXPECT_TRUE(book.addOrder(kOrder3, Side::Sell, kPrice99, kQty30, kTs3));

    auto levels = book.getLevelsL2(Side::Sell, 0);
    ASSERT_EQ(levels.size(), 3);

    // Should be sorted ascending for asks (lowest price first)
    EXPECT_EQ(levels[0].price, kPrice99);
    EXPECT_EQ(levels[0].quantity, kQty30);
    EXPECT_EQ(levels[1].price, kPrice100);
    EXPECT_EQ(levels[1].quantity, kQty10);
    EXPECT_EQ(levels[2].price, kPrice101);
    EXPECT_EQ(levels[2].quantity, kQty20);
}

// ============================================================================
// L3 Level Access Tests
// ============================================================================

TEST_F(OrderBookL3Test, GetLevelsL3ZeroCopy) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs2));

    const auto& levels = book.getLevelsL3(Side::Buy);
    EXPECT_EQ(levels.size(), 2);

    // Iterate through levels
    std::size_t count = 0;
    for (const auto& [price, level] : levels) {
        EXPECT_GT(level.getTotalQuantity(), 0);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(OrderBookL3Test, GetLevelByPrice) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));

    const auto* level = book.getLevel(Side::Buy, kPrice100).first;
    ASSERT_NE(level, nullptr);
    EXPECT_EQ(level->price, kPrice100);
    EXPECT_EQ(level->getTotalQuantity(), kQty10);
}

TEST_F(OrderBookL3Test, GetNonExistentLevel) {
    OrderBookL3 book(kSymbol);

    const auto* level = book.getLevel(Side::Buy, kPrice100).first;
    EXPECT_EQ(level, nullptr);
}

TEST_F(OrderBookL3Test, IterateOrdersAtLevel) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, kPriority2));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice100, kQty20, kTs2, kPriority1));
    EXPECT_TRUE(book.addOrder(kOrder3, Side::Buy, kPrice100, kQty30, kTs3, kPriority3));

    const auto* level = book.getLevel(Side::Buy, kPrice100).first;
    ASSERT_NE(level, nullptr);

    // Iterate through orders (should be in priority order)
    std::vector<OrderId> order_ids;
    for (const auto& order : level->orders) {
        order_ids.push_back(order.order_id);
    }

    ASSERT_EQ(order_ids.size(), 3);
    EXPECT_EQ(order_ids[0], kOrder2);  // Priority 1 (highest)
    EXPECT_EQ(order_ids[1], kOrder1);  // Priority 2
    EXPECT_EQ(order_ids[2], kOrder3);  // Priority 3 (lowest)
}

// ============================================================================
// Clear Tests
// ============================================================================

TEST_F(OrderBookL3Test, ClearSide) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Sell, kPrice101, kQty20, kTs2));

    book.clearSide(Side::Buy);

    EXPECT_TRUE(book.isEmpty(Side::Buy));
    EXPECT_FALSE(book.isEmpty(Side::Sell));
    EXPECT_EQ(book.orderCount(Side::Buy), 0);
    EXPECT_EQ(book.orderCount(Side::Sell), 1);
}

TEST_F(OrderBookL3Test, Clear) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Sell, kPrice101, kQty20, kTs2));

    book.clear();

    EXPECT_TRUE(book.isEmpty());
    EXPECT_EQ(book.orderCount(), 0);
    EXPECT_EQ(book.levelCount(Side::Buy), 0);
    EXPECT_EQ(book.levelCount(Side::Sell), 0);
}

// ============================================================================
// Observer Tests
// ============================================================================

class TestObserver : public IOrderBookObserver {
public:
    int order_update_count = 0;
    int price_level_update_count = 0;
    int tob_update_count = 0;
    int trade_count = 0;

    OrderUpdate last_order_update;
    PriceLevelUpdate last_level_update;
    TopOfBook last_tob;
    Trade last_trade;

    void onOrderUpdate(const OrderUpdate& update) override {
        ++order_update_count;
        last_order_update = update;
    }

    void onPriceLevelUpdate(const PriceLevelUpdate& update) override {
        ++price_level_update_count;
        last_level_update = update;
    }

    void onTopOfBookUpdate(const TopOfBook& tob) override {
        ++tob_update_count;
        last_tob = tob;
    }

    void onTrade(const Trade& trade) override {
        ++trade_count;
        last_trade = trade;
    }
};

class SnapshotObserver : public IOrderBookObserver {
public:
    int snapshot_begin_count = 0;
    int snapshot_end_count = 0;
    int order_update_count = 0;

    void onSnapshotBegin([[maybe_unused]] SymbolId symbol, [[maybe_unused]] uint64_t seq_num, [[maybe_unused]] Timestamp timestamp) override {
        ++snapshot_begin_count;
    }

    void onSnapshotEnd([[maybe_unused]] SymbolId symbol, [[maybe_unused]] uint64_t seq_num, [[maybe_unused]] Timestamp timestamp) override {
        ++snapshot_end_count;
    }

    void onOrderUpdate(const OrderUpdate& update) override {
        (void)update;
        ++order_update_count;
    }
};

TEST_F(OrderBookL3Test, ObserverAddOrder) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<TestObserver>();
    book.addObserver(observer);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));

    EXPECT_EQ(observer->order_update_count, 1);
    EXPECT_EQ(observer->price_level_update_count, 1);
    EXPECT_EQ(observer->tob_update_count, 1);

    EXPECT_EQ(observer->last_order_update.order_id, kOrder1);
    EXPECT_EQ(observer->last_order_update.price, kPrice100);
    EXPECT_EQ(observer->last_order_update.quantity, kQty10);

    EXPECT_EQ(observer->last_level_update.price, kPrice100);
    EXPECT_EQ(observer->last_level_update.quantity, kQty10);

    EXPECT_EQ(observer->last_tob.best_bid, kPrice100);
}

TEST_F(OrderBookL3Test, ObserverModifyOrder) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<TestObserver>();
    book.addObserver(observer);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    observer->order_update_count = 0;  // Reset

    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice100, kQty20));

    EXPECT_EQ(observer->order_update_count, 1);
    EXPECT_EQ(observer->last_order_update.quantity, kQty20);
}

TEST_F(OrderBookL3Test, ObserverDeleteOrder) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<TestObserver>();
    book.addObserver(observer);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    observer->order_update_count = 0;  // Reset

    EXPECT_TRUE(book.deleteOrder(kOrder1));

    EXPECT_EQ(observer->order_update_count, 1);
    EXPECT_EQ(observer->last_order_update.quantity, 0);  // Quantity=0 means delete
}

TEST_F(OrderBookL3Test, EmitSnapshotEmitsAllOrders) {
    OrderBookL3 book(kSymbol);
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Sell, kPrice101, kQty20, kTs2));
    EXPECT_TRUE(book.addOrder(kOrder3, Side::Buy, kPrice99, kQty30, kTs3));

    auto observer = std::make_shared<SnapshotObserver>();
    book.addObserver(observer);

    book.emitSnapshot(kTs4);

    EXPECT_EQ(observer->snapshot_begin_count, 1);
    EXPECT_EQ(observer->snapshot_end_count, 1);
    EXPECT_EQ(observer->order_update_count, 3);
}

TEST_F(OrderBookL3Test, OrderUpdateLevelIndexBestBidIsZero) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<TestObserver>();
    book.addObserver(observer);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs2));

    EXPECT_EQ(observer->last_order_update.order_id, kOrder2);
    EXPECT_EQ(observer->last_order_update.price_level_index, 0);
}

// Batch flag tests
class BatchObserverL3 : public IOrderBookObserver {
public:
    std::vector<OrderUpdate> order_updates;
    std::vector<PriceLevelUpdate> level_updates;
    std::vector<TopOfBook> tob_updates;

    void onOrderUpdate(const OrderUpdate& update) override {
        order_updates.push_back(update);
    }

    void onPriceLevelUpdate(const PriceLevelUpdate& update) override {
        level_updates.push_back(update);
    }

    void onTopOfBookUpdate(const TopOfBook& tob) override {
        tob_updates.push_back(tob);
    }

    void reset() {
        order_updates.clear();
        level_updates.clear();
        tob_updates.clear();
    }
};

TEST_F(OrderBookL3Test, BatchFlagSingleAddOrder) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    // Single operation with default is_last_in_batch = true
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));

    // Should receive 1 order update, 1 level update, and 1 ToB update
    ASSERT_EQ(observer->order_updates.size(), 1);
    ASSERT_EQ(observer->level_updates.size(), 1);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Check flags
    EXPECT_TRUE(observer->order_updates[0].isLastInBatch());
    EXPECT_TRUE(observer->order_updates[0].priceChanged());
    EXPECT_TRUE(observer->order_updates[0].quantityChanged());

    EXPECT_TRUE(observer->level_updates[0].isLastInBatch());
    EXPECT_TRUE(observer->level_updates[0].priceChanged());
    EXPECT_TRUE(observer->level_updates[0].quantityChanged());
}

TEST_F(OrderBookL3Test, BatchFlagMultipleAddOrders) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    // Batch of 3 orders - only last one should trigger ToB
    // Note: Must explicitly pass priority=0 and seq_num=0 to reach is_last_in_batch parameter
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 0, false));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs1, 0, 0, false));
    EXPECT_TRUE(book.addOrder(kOrder3, Side::Buy, kPrice102, kQty30, kTs1, 0, 0, true));

    // Should receive 3 order updates, 3 level updates, but only 1 ToB update
    ASSERT_EQ(observer->order_updates.size(), 3);
    ASSERT_EQ(observer->level_updates.size(), 3);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Check flags
    EXPECT_FALSE(observer->order_updates[0].isLastInBatch());
    EXPECT_FALSE(observer->order_updates[1].isLastInBatch());
    EXPECT_TRUE(observer->order_updates[2].isLastInBatch());

    EXPECT_FALSE(observer->level_updates[0].isLastInBatch());
    EXPECT_FALSE(observer->level_updates[1].isLastInBatch());
    EXPECT_TRUE(observer->level_updates[2].isLastInBatch());

    // ToB should reflect final state (best bid = 10200)
    EXPECT_EQ(observer->tob_updates[0].best_bid, kPrice102);
}

TEST_F(OrderBookL3Test, BatchFlagModifyOrder) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    // Add initial order
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    observer->reset();

    // Modify in batch
    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice100, kQty20, 0, false));  // seq_num=0, qty change, not last
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty30, kTs2, 0, 0, true));  // priority=0, seq_num=0, last

    // Should receive 2 order updates, 2 level updates, 1 ToB
    ASSERT_EQ(observer->order_updates.size(), 2);
    ASSERT_EQ(observer->level_updates.size(), 2);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // First modify should not have LastInBatch
    EXPECT_FALSE(observer->order_updates[0].priceChanged());
    EXPECT_TRUE(observer->order_updates[0].quantityChanged());
    EXPECT_FALSE(observer->order_updates[0].isLastInBatch());

    // Second operation should have LastInBatch
    EXPECT_TRUE(observer->order_updates[1].isLastInBatch());
}

TEST_F(OrderBookL3Test, BatchFlagDeleteOrder) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    // Add orders
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs1));
    observer->reset();

    // Delete in batch
    EXPECT_TRUE(book.deleteOrder(kOrder1, 0, false));  // seq_num=0, not last
    EXPECT_TRUE(book.deleteOrder(kOrder2, 0, true));   // seq_num=0, last

    // Should receive 2 order updates, 2 level updates, 1 ToB
    ASSERT_EQ(observer->order_updates.size(), 2);
    ASSERT_EQ(observer->level_updates.size(), 2);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Both deletes should have proper flags
    EXPECT_TRUE(observer->order_updates[0].isDelete());
    EXPECT_FALSE(observer->order_updates[0].isLastInBatch());

    EXPECT_TRUE(observer->order_updates[1].isDelete());
    EXPECT_TRUE(observer->order_updates[1].isLastInBatch());

    // ToB should show empty book
    EXPECT_EQ(observer->tob_updates[0].best_bid, 0);
}

TEST_F(OrderBookL3Test, BatchFlagExecuteOrder) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    // Add order
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty30, kTs1));
    observer->reset();

    // Execute in batch (partial fills)
    EXPECT_TRUE(book.executeOrder(kOrder1, kQty10, 0, false));  // seq_num=0, partial, not last
    EXPECT_TRUE(book.executeOrder(kOrder1, kQty10, 0, true));   // seq_num=0, partial, last

    // Should receive 2 order updates (modify qty), 2 level updates, 1 ToB
    ASSERT_EQ(observer->order_updates.size(), 2);
    ASSERT_EQ(observer->level_updates.size(), 2);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    EXPECT_FALSE(observer->order_updates[0].isLastInBatch());
    EXPECT_TRUE(observer->order_updates[1].isLastInBatch());

    // Remaining quantity should be 10
    EXPECT_EQ(observer->order_updates[1].quantity, kQty10);
}

TEST_F(OrderBookL3Test, BatchFlagAddOrModifyOrder) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    // Batch using addOrModifyOrder
    EXPECT_TRUE(book.addOrModifyOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, kTs1, 0, false));  // Add, priority=ts, seq=0, not last
    EXPECT_TRUE(book.addOrModifyOrder(kOrder1, Side::Buy, kPrice101, kQty20, kTs2, kTs2, 0, false));  // Modify price, not last
    EXPECT_TRUE(book.addOrModifyOrder(kOrder2, Side::Buy, kPrice102, kQty30, kTs2, kTs2, 0, true));   // Add, last

    // Should get multiple updates but only 1 ToB at end
    ASSERT_GT(observer->order_updates.size(), 0);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Final ToB should show best bid at 10200
    EXPECT_EQ(observer->tob_updates[0].best_bid, kPrice102);
}

TEST_F(OrderBookL3Test, BatchFlagMixedOperations) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    // Complex batch: add, modify, delete, execute
    // Note: Must explicitly pass priority=0 and seq_num=0 to reach is_last_in_batch parameter
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty40, kTs1, 0, 0, false));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty30, kTs1, 0, 0, false));
    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice100, kQty50, 0, false));  // seq_num=0, increase qty
    EXPECT_TRUE(book.executeOrder(kOrder2, kQty10, 0, false));             // seq_num=0, partial fill
    EXPECT_TRUE(book.deleteOrder(kOrder1, 0, true));                       // seq_num=0, delete, last

    // Should receive multiple order/level updates but only 1 ToB
    ASSERT_GT(observer->order_updates.size(), 0);
    ASSERT_GT(observer->level_updates.size(), 0);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Last order update should have LastInBatch
    bool found_last_in_batch = false;
    for (const auto& update : observer->order_updates) {
        if (update.isLastInBatch()) {
            found_last_in_batch = true;
        }
    }
    EXPECT_TRUE(found_last_in_batch);

    // Final ToB should show best bid at 10100 (only order2 remains)
    EXPECT_EQ(observer->tob_updates[0].best_bid, kPrice101);
}

TEST_F(OrderBookL3Test, BatchFlagPriceChangeInBatch) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    // Add order then modify price in batch
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    observer->reset();

    // Modify price in batch (creates level updates for old and new price)
    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice101, kQty10, 0, true));  // seq_num=0, last

    // Should receive order update, 2 level updates (old price deletion + new price), 1 ToB
    ASSERT_EQ(observer->order_updates.size(), 1);
    ASSERT_EQ(observer->level_updates.size(), 2);
    ASSERT_EQ(observer->tob_updates.size(), 1);

    // Order update should have both price and quantity changed flags
    EXPECT_TRUE(observer->order_updates[0].priceChanged());
    EXPECT_TRUE(observer->order_updates[0].isLastInBatch());

    // First level update (old price) should be deletion
    EXPECT_TRUE(observer->level_updates[0].priceChanged());
    EXPECT_TRUE(observer->level_updates[0].quantityChanged());
    EXPECT_EQ(observer->level_updates[0].price, kPrice100);
    EXPECT_EQ(observer->level_updates[0].quantity, 0);

    // Second level update (new price) should have LastInBatch
    EXPECT_TRUE(observer->level_updates[1].isLastInBatch());
    EXPECT_EQ(observer->level_updates[1].price, kPrice101);
}

// ============================================================================
// Sequence Number Tracking Tests
// ============================================================================

TEST_F(OrderBookL3Test, SequenceNumberInitialState) {
    OrderBookL3 book(kSymbol);
    EXPECT_EQ(book.getLastSeqNum(), 0);  // Initial state - no tracking
}

TEST_F(OrderBookL3Test, SequenceNumberTrackingAddOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs2, 0, 101));
    EXPECT_EQ(book.getLastSeqNum(), 101);
}

TEST_F(OrderBookL3Test, SequenceNumberTrackingModifyOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice100, kQty20, 101));
    EXPECT_EQ(book.getLastSeqNum(), 101);
}

TEST_F(OrderBookL3Test, SequenceNumberTrackingDeleteOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 100));
    EXPECT_TRUE(book.deleteOrder(kOrder1, 101));
    EXPECT_EQ(book.getLastSeqNum(), 101);
}

TEST_F(OrderBookL3Test, SequenceNumberTrackingExecuteOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty20, kTs1, 0, 100));
    EXPECT_TRUE(book.executeOrder(kOrder1, kQty10, 101));
    EXPECT_EQ(book.getLastSeqNum(), 101);

    // Verify partial execution
    auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->quantity, kQty10);
}

TEST_F(OrderBookL3Test, SequenceNumberRejectOutOfOrderAddOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Try out-of-order (should fail)
    EXPECT_FALSE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs1, 0, 99));
    EXPECT_EQ(book.getLastSeqNum(), 100);  // Should not update

    // Verify second order was NOT added
    EXPECT_EQ(book.orderCount(Side::Buy), 1);
    EXPECT_EQ(book.findOrder(kOrder2), nullptr);
}

TEST_F(OrderBookL3Test, SequenceNumberRejectOutOfOrderModifyOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Try out-of-order modify (should fail)
    EXPECT_FALSE(book.modifyOrder(kOrder1, kPrice100, kQty20, 99));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Verify order was NOT modified
    auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->quantity, kQty10);  // Original quantity
}

TEST_F(OrderBookL3Test, SequenceNumberRejectOutOfOrderDeleteOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Try out-of-order delete (should fail)
    EXPECT_FALSE(book.deleteOrder(kOrder1, 99));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Verify order was NOT deleted
    EXPECT_EQ(book.orderCount(Side::Buy), 1);
    EXPECT_NE(book.findOrder(kOrder1), nullptr);
}

TEST_F(OrderBookL3Test, SequenceNumberRejectOutOfOrderExecuteOrder) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty20, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Try out-of-order execute (should fail)
    EXPECT_FALSE(book.executeOrder(kOrder1, kQty10, 99));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Verify order was NOT executed
    auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->quantity, kQty20);  // Original quantity
}

TEST_F(OrderBookL3Test, SequenceNumberAcceptGap) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Gap from 100 to 200 (should be accepted)
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs1, 0, 200));
    EXPECT_EQ(book.getLastSeqNum(), 200);

    // Verify both orders exist
    EXPECT_EQ(book.orderCount(Side::Buy), 2);
}

TEST_F(OrderBookL3Test, SequenceNumberAcceptDuplicate) {
    OrderBookL3 book(kSymbol);

    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Duplicate seq_num (should be accepted - seq_num == last_seq_num is allowed)
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Verify both orders exist
    EXPECT_EQ(book.orderCount(Side::Buy), 2);
}

TEST_F(OrderBookL3Test, SequenceNumberNoTracking) {
    OrderBookL3 book(kSymbol);

    // All updates without seq_num (default = 0)
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs2));
    EXPECT_TRUE(book.modifyOrder(kOrder1, kPrice100, kQty30));
    EXPECT_TRUE(book.deleteOrder(kOrder2));

    // Sequence number should remain 0
    EXPECT_EQ(book.getLastSeqNum(), 0);

    // Operations should work normally
    EXPECT_EQ(book.orderCount(Side::Buy), 1);
    auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->quantity, kQty30);
}

TEST_F(OrderBookL3Test, SequenceNumberInOrderUpdateEvents) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 12345);

    ASSERT_EQ(observer->order_updates.size(), 1);
    EXPECT_EQ(observer->order_updates[0].seq_num, 12345);
}

TEST_F(OrderBookL3Test, SequenceNumberInLevelUpdateEvents) {
    OrderBookL3 book(kSymbol);
    auto observer = std::make_shared<BatchObserverL3>();
    book.addObserver(observer);

    book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 54321);

    ASSERT_EQ(observer->level_updates.size(), 1);
    EXPECT_EQ(observer->level_updates[0].seq_num, 54321);
}

TEST_F(OrderBookL3Test, SequenceNumberAddOrModifyOrder) {
    OrderBookL3 book(kSymbol);

    // Add with seq_num
    EXPECT_TRUE(book.addOrModifyOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, kTs1, 100));
    EXPECT_EQ(book.getLastSeqNum(), 100);

    // Modify with seq_num
    EXPECT_TRUE(book.addOrModifyOrder(kOrder1, Side::Buy, kPrice101, kQty20, kTs2, kTs2, 101));
    EXPECT_EQ(book.getLastSeqNum(), 101);

    // Verify modification
    auto* order = book.findOrder(kOrder1);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->price, kPrice101);
    EXPECT_EQ(order->quantity, kQty20);
}

TEST_F(OrderBookL3Test, SequenceNumberMultipleSides) {
    OrderBookL3 book(kSymbol);

    // Sequence numbers apply to entire book, not per side
    EXPECT_TRUE(book.addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1, 0, 100));
    EXPECT_TRUE(book.addOrder(kOrder2, Side::Sell, kPrice101, kQty20, kTs1, 0, 101));

    EXPECT_EQ(book.getLastSeqNum(), 101);

    // Out-of-order on different side should still be rejected
    EXPECT_FALSE(book.addOrder(kOrder3, Side::Buy, kPrice99, kQty30, kTs1, 0, 100));
    EXPECT_EQ(book.getLastSeqNum(), 101);  // Should not update

    // Verify rejected order was not added
    EXPECT_EQ(book.orderCount(Side::Buy), 1);  // Only first bid exists
    EXPECT_EQ(book.findOrder(kOrder3), nullptr);
}
