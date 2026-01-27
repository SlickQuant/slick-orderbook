// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

/// @file multi_symbol_orderbook.cpp
/// @brief Demonstrates multi-symbol orderbook management with concurrent access
///
/// This example shows how to:
/// 1. Use OrderBookManager to manage multiple symbols
/// 2. Handle concurrent access to different symbols (thread-safe)
/// 3. Implement a simple market data feed processor
/// 4. Query top-of-book across multiple symbols
/// 5. Manage symbol lifecycle (add/remove symbols)

#include <slick/orderbook/orderbook_manager.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>

using namespace slick::orderbook;

// Example symbols
enum Symbols : SymbolId {
    AAPL = 1,   // Apple
    MSFT = 2,   // Microsoft
    GOOGL = 3,  // Google
    AMZN = 4,   // Amazon
    TSLA = 5,   // Tesla
};

const char* getSymbolName(SymbolId symbol) {
    switch (symbol) {
        case AAPL: return "AAPL";
        case MSFT: return "MSFT";
        case GOOGL: return "GOOGL";
        case AMZN: return "AMZN";
        case TSLA: return "TSLA";
        default: return "UNKNOWN";
    }
}

// Simulate market data feed for a single symbol
void feedSimulator(OrderBookManager<OrderBookL2>& manager,
                   SymbolId symbol,
                   std::atomic<bool>& stop,
                   int update_count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<Price> price_dist(10000, 15000);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);

    auto* book = manager.getOrCreateOrderBook(symbol);

    for (int i = 0; i < update_count && !stop.load(); ++i) {
        Timestamp ts = std::chrono::system_clock::now().time_since_epoch().count();

        // Add some bid levels
        Price bid_base = price_dist(gen);
        book->updateLevel(Side::Buy, bid_base, qty_dist(gen), ts);
        book->updateLevel(Side::Buy, bid_base - 10, qty_dist(gen), ts);
        book->updateLevel(Side::Buy, bid_base - 20, qty_dist(gen), ts);

        // Add some ask levels
        Price ask_base = bid_base + 50;
        book->updateLevel(Side::Sell, ask_base, qty_dist(gen), ts);
        book->updateLevel(Side::Sell, ask_base + 10, qty_dist(gen), ts);
        book->updateLevel(Side::Sell, ask_base + 20, qty_dist(gen), ts);

        // Small delay to simulate realistic feed rate
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Print market snapshot for all symbols
void printMarketSnapshot(const OrderBookManager<OrderBookL2>& manager,
                         const std::vector<SymbolId>& symbols) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "MARKET SNAPSHOT\n";
    std::cout << std::string(80, '=') << "\n";
    std::cout << std::left << std::setw(10) << "Symbol"
              << std::right << std::setw(12) << "Bid Price"
              << std::setw(12) << "Bid Qty"
              << std::setw(12) << "Ask Price"
              << std::setw(12) << "Ask Qty"
              << std::setw(12) << "Spread\n";
    std::cout << std::string(80, '-') << "\n";

    for (SymbolId symbol : symbols) {
        const auto* book = manager.getOrderBook(symbol);
        if (!book) continue;

        auto tob = book->getTopOfBook();
        const char* name = getSymbolName(symbol);

        std::cout << std::left << std::setw(10) << name;

        if (tob.best_bid > 0) {
            std::cout << std::right << std::setw(12) << tob.best_bid
                      << std::setw(12) << tob.bid_quantity;
        } else {
            std::cout << std::right << std::setw(12) << "-"
                      << std::setw(12) << "-";
        }

        if (tob.best_ask > 0) {
            std::cout << std::setw(12) << tob.best_ask
                      << std::setw(12) << tob.ask_quantity;
        } else {
            std::cout << std::setw(12) << "-"
                      << std::setw(12) << "-";
        }

        if (tob.best_bid > 0 && tob.best_ask > 0) {
            Price spread = tob.best_ask - tob.best_bid;
            std::cout << std::setw(12) << spread;
        } else {
            std::cout << std::setw(12) << "-";
        }

        std::cout << "\n";
    }

    std::cout << std::string(80, '=') << "\n";
}

// Print market depth for a specific symbol
void printMarketDepth(const OrderBookManager<OrderBookL2>& manager,
                      SymbolId symbol,
                      std::size_t depth) {
    const auto* book = manager.getOrderBook(symbol);
    if (!book) {
        std::cout << "Symbol not found\n";
        return;
    }

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "MARKET DEPTH: " << getSymbolName(symbol) << "\n";
    std::cout << std::string(60, '=') << "\n";

    // Get bid and ask levels
    auto bid_levels = book->getLevels(Side::Buy, depth);
    auto ask_levels = book->getLevels(Side::Sell, depth);

    // Print header
    std::cout << std::right << std::setw(12) << "Bid Price"
              << std::setw(12) << "Bid Qty"
              << "  |  "
              << std::setw(12) << "Ask Price"
              << std::setw(12) << "Ask Qty" << "\n";
    std::cout << std::string(60, '-') << "\n";

    // Print levels side-by-side
    std::size_t max_levels = std::max(bid_levels.size(), ask_levels.size());
    for (std::size_t i = 0; i < max_levels; ++i) {
        // Bid side
        if (i < bid_levels.size()) {
            std::cout << std::right << std::setw(12) << bid_levels[i].price
                      << std::setw(12) << bid_levels[i].quantity;
        } else {
            std::cout << std::right << std::setw(12) << "-"
                      << std::setw(12) << "-";
        }

        std::cout << "  |  ";

        // Ask side
        if (i < ask_levels.size()) {
            std::cout << std::setw(12) << ask_levels[i].price
                      << std::setw(12) << ask_levels[i].quantity;
        } else {
            std::cout << std::setw(12) << "-"
                      << std::setw(12) << "-";
        }

        std::cout << "\n";
    }

    std::cout << std::string(60, '=') << "\n";
}

// Calculate total liquidity across all symbols
void printLiquidityStats(const OrderBookManager<OrderBookL2>& manager,
                         const std::vector<SymbolId>& symbols) {
    Quantity total_bid_qty = 0;
    Quantity total_ask_qty = 0;

    for (SymbolId symbol : symbols) {
        const auto* book = manager.getOrderBook(symbol);
        if (!book) continue;

        auto bid_levels = book->getLevels(Side::Buy, 10);
        auto ask_levels = book->getLevels(Side::Sell, 10);

        for (const auto& level : bid_levels) {
            total_bid_qty += level.quantity;
        }
        for (const auto& level : ask_levels) {
            total_ask_qty += level.quantity;
        }
    }

    std::cout << "\nLIQUIDITY STATS (Top 10 levels per symbol):\n";
    std::cout << "  Total Bid Quantity: " << total_bid_qty << "\n";
    std::cout << "  Total Ask Quantity: " << total_ask_qty << "\n";
}

int main() {
    std::cout << "=== Multi-Symbol OrderBook Example ===\n\n";

    // Create L2 manager
    OrderBookManager<OrderBookL2> manager;

    // List of symbols to track
    std::vector<SymbolId> symbols = {AAPL, MSFT, GOOGL, AMZN, TSLA};

    std::cout << "Starting market data feed simulation for "
              << symbols.size() << " symbols...\n";

    // Start feed simulators (one thread per symbol)
    std::atomic<bool> stop{false};
    std::vector<std::thread> feed_threads;
    constexpr int UPDATES_PER_SYMBOL = 50;

    for (SymbolId symbol : symbols) {
        feed_threads.emplace_back(
            feedSimulator,
            std::ref(manager),
            symbol,
            std::ref(stop),
            UPDATES_PER_SYMBOL
        );
    }

    // Wait for feeds to generate some data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Print initial snapshot
    printMarketSnapshot(manager, symbols);

    // Wait a bit more
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Print market depth for a specific symbol
    printMarketDepth(manager, AAPL, 5);

    // Wait for all feed threads to complete
    for (auto& thread : feed_threads) {
        thread.join();
    }

    std::cout << "\nFeed simulation complete.\n";

    // Final snapshot
    printMarketSnapshot(manager, symbols);

    // Liquidity stats
    printLiquidityStats(manager, symbols);

    // Manager stats
    std::cout << "\nMANAGER STATS:\n";
    std::cout << "  Active symbols: " << manager.symbolCount() << "\n";
    auto active_symbols = manager.getSymbols();
    std::cout << "  Symbol IDs: ";
    for (SymbolId s : active_symbols) {
        std::cout << getSymbolName(s) << " ";
    }
    std::cout << "\n";

    // Demonstrate symbol removal
    std::cout << "\nRemoving symbol: TSLA\n";
    if (manager.removeOrderBook(TSLA)) {
        std::cout << "  Successfully removed TSLA\n";
        std::cout << "  Active symbols: " << manager.symbolCount() << "\n";
    }

    // Final snapshot without TSLA
    symbols.erase(std::remove(symbols.begin(), symbols.end(), TSLA), symbols.end());
    printMarketSnapshot(manager, symbols);

    std::cout << "\n=== Example Complete ===\n";
    return 0;
}
