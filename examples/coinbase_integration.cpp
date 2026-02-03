// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_l2.hpp>
#include <coinbase/websocket.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <csignal>
#include <atomic>

using WebSocketClient = coinbase::WebSocketClient;

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

void signalHandler([[maybe_unused]] int signal) {
    std::cout << "\nShutting down gracefully..." << std::endl;
    g_running.store(false);
}

/// Observer to track and display top N level changes
class CoinbaseOrderBookObserver : public slick::orderbook::IOrderBookObserver {
private:
    slick::orderbook::OrderBookL2* orderbook_;
    std::string symbol_;
    bool in_snapshot_;
    static constexpr int64_t SCALE_FACTOR = 100'000'000;  // 10^8 for 8 decimals
    static constexpr size_t TOP_N_LEVELS = 10;

public:
    explicit CoinbaseOrderBookObserver(slick::orderbook::OrderBookL2* orderbook, const std::string& symbol)
        : orderbook_(orderbook), symbol_(symbol), in_snapshot_(false) {}

    void onSnapshotBegin(slick::orderbook::SymbolId symbol, uint64_t, [[maybe_unused]] slick::orderbook::Timestamp timestamp) override {
        in_snapshot_ = true;
        std::cout << symbol << " receiving orderbook snapshot..." << std::endl;
    }

    void onSnapshotEnd(slick::orderbook::SymbolId symbol, uint64_t seq_num, [[maybe_unused]] slick::orderbook::Timestamp timestamp) override {
        in_snapshot_ = false;
        std::cout << symbol << " snapshot complete. Displaying initial orderbook:\n" << std::endl;
        printTopLevels(seq_num);
    }

    void onPriceLevelUpdate(const slick::orderbook::PriceLevelUpdate& update) override {
        // Only print if:
        // 1. Not in snapshot (avoid printing during snapshot load)
        // 2. Update affects top 10 levels
        if (!in_snapshot_ && update.isTopN(TOP_N_LEVELS)) {
            if (update.change_flags == 0) {
                std::cout << "!!!! UNCHANGED UPDATE" << std::endl;
            }
            if (update.isLastInBatch()) {
                printTopLevels(update.seq_num);
            }
        }
    }

    void printTopLevels(uint64_t seq_num) {
        auto bids = orderbook_->getLevels(slick::orderbook::Side::Buy, TOP_N_LEVELS);
        auto asks = orderbook_->getLevels(slick::orderbook::Side::Sell, TOP_N_LEVELS);

        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = *std::localtime(&now_time_t);
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%F %T");
        std::string str_local_now = oss.str();

        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << symbol_ << " Order Book - "
                  << str_local_now << " seq_num: " << seq_num << "\n";
        std::cout << std::string(70, '=') << "\n";

        // Print asks in reverse order (highest to lowest)
        std::cout << std::right << std::setw(35) << "ASKS" << "\n";
        std::cout << std::string(70, '-') << "\n";
        std::cout << std::right << std::setw(20) << "Price"
                  << "  " << std::setw(15) << "Quantity" << "\n";
        std::cout << std::string(70, '-') << "\n";

        for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
            double price_dbl = static_cast<double>(it->price) / SCALE_FACTOR;
            double qty_dbl = static_cast<double>(it->quantity) / SCALE_FACTOR;
            std::cout << std::right << std::setw(20) << std::fixed << std::setprecision(2) << price_dbl
                      << "  " << std::setw(15) << std::setprecision(8) << qty_dbl << "\n";
        }

        // Print spread
        if (!bids.empty() && !asks.empty()) {
            double bid_price = static_cast<double>(bids[0].price) / SCALE_FACTOR;
            double ask_price = static_cast<double>(asks[0].price) / SCALE_FACTOR;
            double spread = ask_price - bid_price;
            double spread_pct = (spread / ask_price) * 100.0;
            std::cout << std::string(70, '-') << "\n";
            std::cout << "Spread: " << std::fixed << std::setprecision(2) << spread
                      << " (" << std::setprecision(4) << spread_pct << "%)\n";
            std::cout << std::string(70, '-') << "\n";
        }

        // Print bids (highest to lowest)
        std::cout << std::right << std::setw(35) << "BIDS" << "\n";
        std::cout << std::string(70, '-') << "\n";
        std::cout << std::right << std::setw(20) << "Price"
                  << "  " << std::setw(15) << "Quantity" << "\n";
        std::cout << std::string(70, '-') << "\n";

        for (const auto& level : bids) {
            double price_dbl = static_cast<double>(level.price) / SCALE_FACTOR;
            double qty_dbl = static_cast<double>(level.quantity) / SCALE_FACTOR;
            std::cout << std::right << std::setw(20) << std::fixed << std::setprecision(2) << price_dbl
                      << "  " << std::setw(15) << std::setprecision(8) << qty_dbl << "\n";
        }

        std::cout << std::string(70, '=') << "\n" << std::endl;

    }
};

/// Main adapter class bridging Coinbase WebSocket and OrderBookL2
class CoinbaseOrderBookAdapter : public coinbase::WebsocketCallbacks {
private:
    slick::orderbook::OrderBookL2 orderbook_;
    std::shared_ptr<CoinbaseOrderBookObserver> observer_;
    std::string symbol_;

    // Constants
    static constexpr int64_t SCALE_FACTOR = 100'000'000;  // 10^8 for 8 decimals
    static constexpr slick::orderbook::SymbolId SYMBOL_ID = 1;

    /// Convert Coinbase price (double) to fixed-point Price (int64_t)
    int64_t toPrice(double price) const {
        return static_cast<int64_t>(price * SCALE_FACTOR);
    }

    /// Convert Coinbase quantity (double) to fixed-point Quantity (int64_t)
    int64_t toQuantity(double qty) const {
        return static_cast<int64_t>(qty * SCALE_FACTOR);
    }

    /// Convert Coinbase Side enum to slick::orderbook Side enum
    slick::orderbook::Side toSide(coinbase::Side side) const {
        return (side == coinbase::Side::BUY) ?
            slick::orderbook::Side::Buy : slick::orderbook::Side::Sell;
    }

    /// Convert Coinbase timestamp (milliseconds) to nanoseconds
    slick::orderbook::Timestamp toTimestamp(uint64_t event_time_ms) const {
        return event_time_ms * 1'000'000;  // ms to ns
    }

public:
    explicit CoinbaseOrderBookAdapter(const std::string& symbol)
        : orderbook_(SYMBOL_ID), symbol_(symbol)
    {
        observer_ = std::make_shared<CoinbaseOrderBookObserver>(&orderbook_, symbol_);
        orderbook_.addObserver(observer_);
    }

    void onMarketDataConnected(WebSocketClient*) override {
        std::cout << "MarketData WebSocket Conntected" << std::endl;
    }

    void onMarketDataDisconnected(WebSocketClient*) override {
        std::cout << "MarketData WebSocket Disconnected" << std::endl;
    }

    void onUserDataConnected(WebSocketClient*) override {
        std::cout << "UserData WebSocket Conntected" << std::endl;
    }

    void onUserDataDisconnected(WebSocketClient*) override {
        std::cout << "UserData WebSocket Disconntected" << std::endl;
    }

    /// Handle initial orderbook snapshot from Coinbase
    void onLevel2Snapshot(WebSocketClient*, uint64_t seq_num, const coinbase::Level2UpdateBatch& snapshot) override {
        std::cout << "Received L2 snapshot for " << snapshot.product_id
                  << " with " << snapshot.updates.size() << " levels" << std::endl;

        // Clear existing orderbook (in case of reconnection)
        orderbook_.clear();

        // Notify snapshot begin
        if (!snapshot.updates.empty()) {
            observer_->onSnapshotBegin(SYMBOL_ID, seq_num, toTimestamp(snapshot.updates[0].event_time));
        }

        // Process all levels in snapshot
        // Each updateLevel() call will trigger onPriceLevelUpdate() in the observer
        // Observer is in snapshot mode (in_snapshot_=true), so it won't print yet
        // Mark only the last update in the batch with is_last_in_batch=true
        const size_t num_updates = snapshot.updates.size();
        for (size_t i = 0, last = num_updates - 1; i < num_updates; ++i) {
            const auto& update = snapshot.updates[i];
            if (update.new_quantity > 0.0) {  // Only add levels with positive quantity
                bool is_last = (i == last);
                orderbook_.updateLevel(
                    toSide(update.side),
                    toPrice(update.price_level),
                    toQuantity(update.new_quantity),
                    toTimestamp(update.event_time),
                    seq_num,
                    is_last
                );
            }
        }

        // Notify snapshot end
        // Observer's onSnapshotEnd() will print the full orderbook
        if (!snapshot.updates.empty()) {
            observer_->onSnapshotEnd(SYMBOL_ID, seq_num, toTimestamp(snapshot.updates.back().event_time));
        }
    }

    /// Handle incremental orderbook updates from Coinbase
    void onLevel2Updates(WebSocketClient*, uint64_t seq_num, const coinbase::Level2UpdateBatch& updates) override {
        // Apply all updates as a batch - only the last one should trigger ToB update
        // Pass seq_num to orderbook for sequence tracking and gap detection
        const size_t num_updates = updates.updates.size();
        for (size_t i = 0, last = num_updates - 1; i < num_updates; ++i) {
            const auto& update = updates.updates[i];
            auto side = toSide(update.side);
            auto price = toPrice(update.price_level);
            auto qty = toQuantity(update.new_quantity);
            auto ts = toTimestamp(update.event_time);
            bool is_last = (i == last);

            // Quantity of 0 means delete the level
            // Observer will be notified with level_index and change_flags
            // Only the last update in the batch will trigger ToB emission
            // Pass seq_num for out-of-order detection (orderbook will reject if seq_num < last_seq_num)
            orderbook_.updateLevel(side, price, qty, ts, seq_num, is_last);
        }
    }

    /// Handle market data sequence gap (messages lost)
    void onMarketDataGap(WebSocketClient*) override {
        std::cerr << "\n*** WARNING: Market data sequence gap detected! ***" << std::endl;
        std::cerr << "*** Orderbook may be out of sync. ***" << std::endl;
        std::cerr << "*** Consider reconnecting to receive a fresh snapshot. ***\n" << std::endl;
    }

    /// Handle market data errors
    void onMarketDataError(WebSocketClient*, std::string &&err) override {
        std::cerr << "\n*** ERROR: " << err << " ***\n" << std::endl;
    }

    // Empty implementations for unused callbacks (required by WebsocketCallbacks interface)
    void onMarketTradesSnapshot(WebSocketClient*, uint64_t, const std::vector<coinbase::MarketTrade>&) override {}
    void onMarketTrades(WebSocketClient*, uint64_t, const std::vector<coinbase::MarketTrade>&) override {}
    void onTickerSnapshot(WebSocketClient*, uint64_t, uint64_t, const std::vector<coinbase::Ticker>&) override {}
    void onTickers(WebSocketClient*, uint64_t, uint64_t, const std::vector<coinbase::Ticker>&) override {}
    void onCandlesSnapshot(WebSocketClient*, uint64_t, uint64_t, const std::vector<coinbase::Candle>&) override {}
    void onCandles(WebSocketClient*, uint64_t, uint64_t, const std::vector<coinbase::Candle>&) override {}
    void onStatusSnapshot(WebSocketClient*, uint64_t, uint64_t, const std::vector<coinbase::Status>&) override {}
    void onStatus(WebSocketClient*, uint64_t, uint64_t, const std::vector<coinbase::Status>&) override {}
    void onUserDataGap(WebSocketClient*) override {}
    void onUserDataSnapshot(WebSocketClient*, uint64_t, const std::vector<coinbase::Order>&,
                           const std::vector<coinbase::PerpetualFuturePosition>&,
                           const std::vector<coinbase::ExpiringFuturePosition>&) override {}
    void onOrderUpdates(WebSocketClient*, uint64_t, const std::vector<coinbase::Order>&) override {}
    void onUserDataError(WebSocketClient*, std::string&&) override {}
};

int main(int argc, char* argv[]) {
    // Set up signal handler for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Parse command line arguments
    std::string symbol = "BTC-USD";
    if (argc > 1) {
        symbol = argv[1];
    }

    std::cout << "=================================================================\n";
    std::cout << "           Coinbase OrderBook Integration Example\n";
    std::cout << "=================================================================\n";
    std::cout << "Symbol: " << symbol << "\n";
    std::cout << "Connecting to Coinbase WebSocket feed...\n";
    std::cout << "Press Ctrl+C to exit.\n";
    std::cout << "=================================================================\n" << std::endl;

    try {
        // Create adapter
        CoinbaseOrderBookAdapter adapter(symbol);

        // Create WebSocket client
        coinbase::WebSocketClient client(&adapter);

        // Subscribe to Level2 channel for the symbol
        std::vector<std::string> symbols = {symbol};
        std::vector<coinbase::WebSocketChannel> channels = {
            coinbase::WebSocketChannel::LEVEL2
        };

        client.subscribe(symbols, channels);

        std::cout << "Subscribed successfully. Waiting for market data...\n" << std::endl;

        // Keep running until interrupted
        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "\nDisconnecting..." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Shutdown complete." << std::endl;
    return 0;
}
