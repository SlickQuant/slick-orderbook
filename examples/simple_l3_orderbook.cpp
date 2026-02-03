// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_l3.hpp>
#include <iostream>
#include <iomanip>
#include <memory>

using namespace slick::orderbook;

// Simple observer that prints updates
class PrintObserver : public IOrderBookObserver {
public:
    void onOrderUpdate(const OrderUpdate& update) override {
        std::cout << "[ORDER] Symbol=" << update.symbol
                  << " OrderId=" << update.order_id
                  << " Side=" << (update.side == Side::Buy ? "Buy" : "Sell")
                  << " Price=" << update.price
                  << " Qty=" << update.quantity
                  << " Timestamp=" << update.timestamp << "\n";
    }

    void onPriceLevelUpdate(const PriceLevelUpdate& update) override {
        std::cout << "[LEVEL] Symbol=" << update.symbol
                  << " Side=" << (update.side == Side::Buy ? "Bid" : "Ask")
                  << " Price=" << update.price
                  << " TotalQty=" << update.quantity
                  << " Timestamp=" << update.timestamp << "\n";
    }

    void onTopOfBookUpdate(const TopOfBook& tob) override {
        std::cout << "[TOB] Symbol=" << tob.symbol
                  << " BestBid=" << tob.best_bid << " (" << tob.bid_quantity << ")"
                  << " BestAsk=" << tob.best_ask << " (" << tob.ask_quantity << ")"
                  << " Timestamp=" << tob.timestamp << "\n";
    }

    void onTrade(const Trade& trade) override {
        std::cout << "[TRADE] Symbol=" << trade.symbol
                  << " Price=" << trade.price
                  << " Qty=" << trade.quantity
                  << " AggressorSide=" << (trade.aggressor_side == Side::Buy ? "Buy" : "Sell")
                  << " Timestamp=" << trade.timestamp << "\n";
    }
};

void printOrderBook(const OrderBookL3& book) {
    std::cout << "\n=== Order Book L3 (Symbol " << book.symbol() << ") ===\n\n";

    // Print asks (descending price order for display)
    auto ask_levels = book.getLevelsL2(Side::Sell, 5);
    std::cout << "Asks:\n";
    for (auto it = ask_levels.rbegin(); it != ask_levels.rend(); ++it) {
        std::cout << "  " << std::setw(10) << it->price
                  << " | " << std::setw(8) << it->quantity << "\n";
    }

    std::cout << "  " << std::string(21, '-') << "\n";

    // Print bids (descending price order)
    auto bid_levels = book.getLevelsL2(Side::Buy, 5);
    std::cout << "Bids:\n";
    for (const auto& level : bid_levels) {
        std::cout << "  " << std::setw(10) << level.price
                  << " | " << std::setw(8) << level.quantity << "\n";
    }

    std::cout << "\nTotal Orders: " << book.orderCount()
              << " (Bids: " << book.orderCount(Side::Buy)
              << ", Asks: " << book.orderCount(Side::Sell) << ")\n";
    std::cout << "Total Levels: " << book.levelCount(Side::Buy) + book.levelCount(Side::Sell)
              << " (Bids: " << book.levelCount(Side::Buy)
              << ", Asks: " << book.levelCount(Side::Sell) << ")\n\n";
}

void printOrdersAtPrice(const OrderBookL3& book, Side side, Price price) {
    const auto [level, level_index] = book.getLevel(side, price);
    if (!level) {
        std::cout << "No orders at " << (side == Side::Buy ? "bid" : "ask")
                  << " price " << price << "\n\n";
        return;
    }

    std::cout << "Orders at " << (side == Side::Buy ? "bid" : "ask")
              << " price " << price << ":\n";
    std::size_t order_num = 1;
    for (const auto& order : level->orders) {
        std::cout << "  #" << order_num++
                  << " OrderId=" << order.order_id
                  << " Qty=" << order.quantity
                  << " Priority=" << order.priority
                  << " Timestamp=" << order.timestamp << "\n";
    }
    std::cout << "Total: " << level->getTotalQuantity()
              << " (" << level->orderCount() << " orders)\n\n";
}

int main() {
    constexpr SymbolId SYMBOL_AAPL = 1;
    constexpr Price PRICE_15000 = 15000;  // $150.00
    constexpr Price PRICE_15010 = 15010;  // $150.10
    constexpr Price PRICE_15020 = 15020;  // $150.20
    constexpr Price PRICE_14990 = 14990;  // $149.90
    constexpr Price PRICE_14980 = 14980;  // $149.80

    std::cout << "=== Slick OrderBook L3 Example ===\n\n";

    // Create L3 orderbook for AAPL
    OrderBookL3 book(SYMBOL_AAPL);

    // Add observer to see all updates
    auto observer = std::make_shared<PrintObserver>();
    book.addObserver(observer);
    std::cout << "Observer registered. Starting order flow...\n\n";

    // Add some bid orders
    std::cout << "--- Adding bid orders ---\n";
    book.addOrder(1001, Side::Buy, PRICE_15000, 100, 1000, 1000);  // Priority = timestamp
    book.addOrder(1002, Side::Buy, PRICE_15000, 200, 2000, 2000);  // Same price, lower priority
    book.addOrder(1003, Side::Buy, PRICE_14990, 150, 3000, 3000);
    book.addOrder(1004, Side::Buy, PRICE_14980, 300, 4000, 4000);

    // Add some ask orders
    std::cout << "\n--- Adding ask orders ---\n";
    book.addOrder(2001, Side::Sell, PRICE_15010, 120, 5000, 5000);
    book.addOrder(2002, Side::Sell, PRICE_15010, 80, 6000, 6000);   // Same price
    book.addOrder(2003, Side::Sell, PRICE_15020, 250, 7000, 7000);

    // Print current state
    printOrderBook(book);

    // Show individual orders at a price level
    std::cout << "=== Order Details at Price Level ===\n\n";
    printOrdersAtPrice(book, Side::Buy, PRICE_15000);
    printOrdersAtPrice(book, Side::Sell, PRICE_15010);

    // Modify an order (quantity)
    std::cout << "--- Modifying order 1001 quantity: 100 -> 150 ---\n";
    book.modifyOrder(1001, PRICE_15000, 150);
    printOrderBook(book);

    // Modify an order (price)
    std::cout << "--- Moving order 1002 from 15000 to 14990 ---\n";
    book.modifyOrder(1002, PRICE_14990, 200);
    printOrderBook(book);

    // Execute an order partially
    std::cout << "--- Partially executing order 2001: 120 -> 70 (executed 50) ---\n";
    book.executeOrder(2001, 50);
    printOrderBook(book);

    // Delete an order
    std::cout << "--- Deleting order 1003 ---\n";
    book.deleteOrder(1003);
    printOrderBook(book);

    // Get top of book
    std::cout << "=== Current Top of Book ===\n";
    TopOfBook tob = book.getTopOfBook();
    std::cout << "Best Bid: " << tob.best_bid << " (" << tob.bid_quantity << ")\n";
    std::cout << "Best Ask: " << tob.best_ask << " (" << tob.ask_quantity << ")\n";
    std::cout << "Spread: " << (tob.best_ask - tob.best_bid) << "\n\n";

    // Get L2 aggregated view
    std::cout << "=== L2 Aggregated View (Top 3 Levels) ===\n\n";
    std::cout << "Bids:\n";
    auto bid_levels = book.getLevelsL2(Side::Buy, 3);
    for (const auto& level : bid_levels) {
        std::cout << "  Price=" << level.price
                  << " Qty=" << level.quantity
                  << " Timestamp=" << level.timestamp << "\n";
    }

    std::cout << "\nAsks:\n";
    auto ask_levels = book.getLevelsL2(Side::Sell, 3);
    for (const auto& level : ask_levels) {
        std::cout << "  Price=" << level.price
                  << " Qty=" << level.quantity
                  << " Timestamp=" << level.timestamp << "\n";
    }

    // Iterate L3 levels directly (zero-copy)
    std::cout << "\n=== L3 Iteration (Zero-Copy Access) ===\n\n";
    std::cout << "All bid levels:\n";
    const auto& bid_levels_l3 = book.getLevelsL3(Side::Buy);
    for (const auto& [price, level] : bid_levels_l3) {
        std::cout << "  Price=" << price
                  << " Orders=" << level.orderCount()
                  << " TotalQty=" << level.getTotalQuantity() << "\n";

        // Iterate orders at this level
        for (const auto& order : level.orders) {
            std::cout << "    OrderId=" << order.order_id
                      << " Qty=" << order.quantity
                      << " Priority=" << order.priority << "\n";
        }
    }

    // Clear the book
    std::cout << "\n--- Clearing order book ---\n";
    book.clear();
    printOrderBook(book);

    std::cout << "=== Example Complete ===\n";
    return 0;
}
