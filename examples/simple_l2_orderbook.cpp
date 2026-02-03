// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_l2.hpp>
#include <iostream>
#include <iomanip>
#include <memory>

using namespace slick::orderbook;

// Simple observer that prints updates
class PrintObserver : public IOrderBookObserver {
public:
    void onOrderUpdate(const OrderUpdate& update) override {
        // L2 orderbook doesn't generate order updates
        (void)update;
    }

    void onPriceLevelUpdate(const PriceLevelUpdate& update) override {
        std::cout << "[LEVEL] Symbol=" << update.symbol
                  << " Side=" << (update.side == Side::Buy ? "Bid" : "Ask")
                  << " Price=" << update.price
                  << " Qty=" << update.quantity
                  << " Timestamp=" << update.timestamp << "\n";
    }

    void onTopOfBookUpdate(const TopOfBook& tob) override {
        std::cout << "[TOB] Symbol=" << tob.symbol
                  << " BestBid=" << tob.best_bid << " (" << tob.bid_quantity << ")"
                  << " BestAsk=" << tob.best_ask << " (" << tob.ask_quantity << ")"
                  << " Timestamp=" << tob.timestamp << "\n";
    }

    void onTrade(const Trade& trade) override {
        // L2 orderbook doesn't generate trades
        (void)trade;
    }
};

void printOrderBook(const OrderBookL2& book) {
    std::cout << "\n=== Order Book L2 (Symbol " << book.symbol() << ") ===\n\n";

    // Print asks (descending price order for display)
    auto ask_levels = book.getLevels(Side::Sell, 5);
    std::cout << "Asks:\n";
    for (auto it = ask_levels.rbegin(); it != ask_levels.rend(); ++it) {
        std::cout << "  " << std::setw(10) << it->price
                  << " | " << std::setw(8) << it->quantity << "\n";
    }

    std::cout << "  " << std::string(21, '-') << "\n";

    // Print bids (descending price order)
    auto bid_levels = book.getLevels(Side::Buy, 5);
    std::cout << "Bids:\n";
    for (const auto& level : bid_levels) {
        std::cout << "  " << std::setw(10) << level.price
                  << " | " << std::setw(8) << level.quantity << "\n";
    }

    std::cout << "\nTotal Levels: " << book.levelCount(Side::Buy) + book.levelCount(Side::Sell)
              << " (Bids: " << book.levelCount(Side::Buy)
              << ", Asks: " << book.levelCount(Side::Sell) << ")\n\n";
}

int main() {
    constexpr SymbolId SYMBOL_AAPL = 1;
    constexpr Price PRICE_15000 = 15000;  // $150.00
    constexpr Price PRICE_15010 = 15010;  // $150.10
    constexpr Price PRICE_15020 = 15020;  // $150.20
    constexpr Price PRICE_15030 = 15030;  // $150.30
    constexpr Price PRICE_14990 = 14990;  // $149.90
    constexpr Price PRICE_14980 = 14980;  // $149.80
    constexpr Price PRICE_14970 = 14970;  // $149.70

    std::cout << "=== Slick OrderBook L2 Example ===\n\n";

    // Create L2 orderbook for AAPL
    OrderBookL2 book(SYMBOL_AAPL);

    // Add observer to see all updates
    auto observer = std::make_shared<PrintObserver>();
    book.addObserver(observer);
    std::cout << "Observer registered. Starting market data feed...\n\n";

    // Add bid levels (price, quantity, timestamp)
    std::cout << "--- Adding bid levels ---\n";
    book.updateLevel(Side::Buy, PRICE_15000, 500, 1000);
    book.updateLevel(Side::Buy, PRICE_14990, 750, 1100);
    book.updateLevel(Side::Buy, PRICE_14980, 1000, 1200);
    book.updateLevel(Side::Buy, PRICE_14970, 1500, 1300);

    // Add ask levels
    std::cout << "\n--- Adding ask levels ---\n";
    book.updateLevel(Side::Sell, PRICE_15010, 600, 1400);
    book.updateLevel(Side::Sell, PRICE_15020, 800, 1500);
    book.updateLevel(Side::Sell, PRICE_15030, 1200, 1600);

    // Print current state
    printOrderBook(book);

    // Update existing level (increase quantity)
    std::cout << "--- Updating bid at 15000: 500 -> 800 ---\n";
    book.updateLevel(Side::Buy, PRICE_15000, 800, 2000);
    printOrderBook(book);

    // Update existing level (decrease quantity)
    std::cout << "--- Updating ask at 15010: 600 -> 300 ---\n";
    book.updateLevel(Side::Sell, PRICE_15010, 300, 2100);
    printOrderBook(book);

    // Delete a level (quantity = 0)
    std::cout << "--- Deleting bid at 14970 (quantity = 0) ---\n";
    book.updateLevel(Side::Buy, PRICE_14970, 0, 2200);
    printOrderBook(book);

    // Add a new best bid (improves the market)
    std::cout << "--- Adding new best bid at 15005 ---\n";
    book.updateLevel(Side::Buy, 15005, 400, 2300);
    printOrderBook(book);

    // Add a new best ask (improves the market)
    std::cout << "--- Adding new best ask at 15005 (crossing) ---\n";
    book.updateLevel(Side::Sell, 15005, 250, 2400);
    printOrderBook(book);

    // Get top of book
    std::cout << "=== Current Top of Book ===\n";
    TopOfBook tob = book.getTopOfBook();
    std::cout << "Best Bid: " << tob.best_bid << " (" << tob.bid_quantity << ")\n";
    std::cout << "Best Ask: " << tob.best_ask << " (" << tob.ask_quantity << ")\n";
    std::cout << "Spread: " << (tob.best_ask - tob.best_bid) << "\n";
    std::cout << "Mid Price: " << (tob.best_bid + tob.best_ask) / 2.0 << "\n\n";

    // Get best bid and ask
    std::cout << "=== Best Bid/Ask Details ===\n";
    const auto* bid = book.getBestBid();
    if (bid) {
        std::cout << "Best Bid: Price=" << bid->price
                  << " Qty=" << bid->quantity
                  << " Timestamp=" << bid->timestamp << "\n";
    }

    const auto* best_ask = book.getBestAsk();
    if (best_ask) {
        std::cout << "Best Ask: Price=" << best_ask->price
                  << " Qty=" << best_ask->quantity
                  << " Timestamp=" << best_ask->timestamp << "\n\n";
    }

    // Get market depth (top N levels)
    std::cout << "=== Market Depth (Top 3 Levels) ===\n\n";
    std::cout << "Bids:\n";
    auto bid_levels = book.getLevels(Side::Buy, 3);
    for (const auto& level : bid_levels) {
        std::cout << "  Price=" << level.price
                  << " Qty=" << level.quantity
                  << " Timestamp=" << level.timestamp << "\n";
    }

    std::cout << "\nAsks:\n";
    auto ask_levels = book.getLevels(Side::Sell, 3);
    for (const auto& level : ask_levels) {
        std::cout << "  Price=" << level.price
                  << " Qty=" << level.quantity
                  << " Timestamp=" << level.timestamp << "\n";
    }

    // Calculate volume-weighted average price (VWAP) for top 3 bid levels
    std::cout << "\n=== Volume Metrics ===\n";
    Quantity total_bid_qty = 0;
    int64_t weighted_sum = 0;
    for (const auto& level : bid_levels) {
        total_bid_qty += level.quantity;
        weighted_sum += level.price * level.quantity;
    }
    if (total_bid_qty > 0) {
        std::cout << "Bid VWAP (top 3): " << (weighted_sum / total_bid_qty) << "\n";
        std::cout << "Total Bid Volume (top 3): " << total_bid_qty << "\n";
    }

    Quantity total_ask_qty = 0;
    weighted_sum = 0;
    for (const auto& level : ask_levels) {
        total_ask_qty += level.quantity;
        weighted_sum += level.price * level.quantity;
    }
    if (total_ask_qty > 0) {
        std::cout << "Ask VWAP (top 3): " << (weighted_sum / total_ask_qty) << "\n";
        std::cout << "Total Ask Volume (top 3): " << total_ask_qty << "\n";
    }

    // Clear one side
    std::cout << "\n--- Clearing all bids ---\n";
    book.clearSide(Side::Buy);
    printOrderBook(book);

    // Check if sides are empty
    std::cout << "=== Empty Check ===\n";
    std::cout << "Bid side empty: " << (book.isEmpty(Side::Buy) ? "Yes" : "No") << "\n";
    std::cout << "Ask side empty: " << (book.isEmpty(Side::Sell) ? "Yes" : "No") << "\n";
    std::cout << "Book empty: " << (book.isEmpty() ? "Yes" : "No") << "\n\n";

    // Clear entire book
    std::cout << "--- Clearing entire book ---\n";
    book.clear();
    printOrderBook(book);

    std::cout << "=== Example Complete ===\n";
    return 0;
}
