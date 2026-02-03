/**
 * @file bench_market_replay.cpp
 * @brief Realistic market data replay benchmarks
 *
 * Simulates real-world market data patterns:
 * - Typical exchange update sequences
 * - Realistic price/quantity distributions
 * - Trade execution patterns
 * - High-frequency update bursts
 * - Multi-symbol scenarios
 *
 * Provides end-to-end performance measurements
 */

#include <slick/orderbook/orderbook.hpp>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <chrono>

using namespace slick::orderbook;

// ============================================================================
// Market Data Generators
// ============================================================================

enum class MarketEventType {
    NewLevel,
    ModifyLevel,
    DeleteLevel,
    NewOrder,
    ModifyOrder,
    DeleteOrder,
    ExecuteOrder
};

struct MarketEvent {
    MarketEventType type;
    Side side;
    Price price;
    Quantity quantity;
    OrderId order_id;
    uint64_t priority;
    Timestamp timestamp;
};

class MarketDataGenerator {
public:
    MarketDataGenerator(uint64_t seed = 12345) : rng_(seed) {}

    // Generate realistic L2 market events
    std::vector<MarketEvent> generateL2Events(size_t count, Price base_bid, Price base_ask) {
        std::vector<MarketEvent> events;
        events.reserve(count);

        std::uniform_int_distribution<int> action_dist(0, 9);
        std::uniform_int_distribution<Quantity> qty_dist(100, 10000);
        std::uniform_int_distribution<int> price_offset_dist(0, 50);

        Timestamp ts = 0;

        for (size_t i = 0; i < count; ++i) {
            MarketEvent event;
            event.timestamp = ts++;

            int action = action_dist(rng_);
            event.side = (i % 2 == 0) ? Side::Buy : Side::Sell;

            if (action < 5) {
                // 50% modify existing level
                event.type = MarketEventType::ModifyLevel;
                event.price = (event.side == Side::Buy)
                    ? base_bid - price_offset_dist(rng_) * 10
                    : base_ask + price_offset_dist(rng_) * 10;
                event.quantity = qty_dist(rng_);
            } else if (action < 8) {
                // 30% new level
                event.type = MarketEventType::NewLevel;
                event.price = (event.side == Side::Buy)
                    ? base_bid - (50 + price_offset_dist(rng_)) * 10
                    : base_ask + (50 + price_offset_dist(rng_)) * 10;
                event.quantity = qty_dist(rng_);
            } else {
                // 20% delete level
                event.type = MarketEventType::DeleteLevel;
                event.price = (event.side == Side::Buy)
                    ? base_bid - price_offset_dist(rng_) * 10
                    : base_ask + price_offset_dist(rng_) * 10;
                event.quantity = 0;
            }

            events.push_back(event);
        }

        return events;
    }

    // Generate realistic L3 market events
    std::vector<MarketEvent> generateL3Events(size_t count, Price base_bid, Price base_ask, OrderId start_order_id = 1) {
        std::vector<MarketEvent> events;
        events.reserve(count);

        std::uniform_int_distribution<int> action_dist(0, 9);
        std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
        std::uniform_int_distribution<Quantity> exec_qty_dist(50, 500);
        std::uniform_int_distribution<int> price_offset_dist(0, 20);
        std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

        OrderId order_id = start_order_id;
        std::vector<OrderId> active_orders;
        Timestamp ts = 0;

        for (size_t i = 0; i < count; ++i) {
            MarketEvent event;
            event.timestamp = ts++;

            int action = action_dist(rng_);
            event.side = (i % 2 == 0) ? Side::Buy : Side::Sell;

            if (action < 4) {
                // 40% new order
                event.type = MarketEventType::NewOrder;
                event.order_id = order_id++;
                event.price = (event.side == Side::Buy)
                    ? base_bid - price_offset_dist(rng_) * 10
                    : base_ask + price_offset_dist(rng_) * 10;
                event.quantity = qty_dist(rng_);
                event.priority = priority_dist(rng_);
                active_orders.push_back(event.order_id);
            } else if (action < 6 && !active_orders.empty()) {
                // 20% modify order
                event.type = MarketEventType::ModifyOrder;
                size_t idx = std::uniform_int_distribution<size_t>(0, active_orders.size() - 1)(rng_);
                event.order_id = active_orders[idx];
                event.price = (event.side == Side::Buy)
                    ? base_bid - price_offset_dist(rng_) * 10
                    : base_ask + price_offset_dist(rng_) * 10;
                event.quantity = qty_dist(rng_);
                event.priority = priority_dist(rng_);
            } else if (action < 7 && !active_orders.empty()) {
                // 10% execute order
                event.type = MarketEventType::ExecuteOrder;
                size_t idx = std::uniform_int_distribution<size_t>(0, active_orders.size() - 1)(rng_);
                event.order_id = active_orders[idx];
                event.quantity = exec_qty_dist(rng_);
            } else if (action < 9 && !active_orders.empty()) {
                // 20% delete order
                event.type = MarketEventType::DeleteOrder;
                size_t idx = std::uniform_int_distribution<size_t>(0, active_orders.size() - 1)(rng_);
                event.order_id = active_orders[idx];
                active_orders.erase(active_orders.begin() + idx);
            } else {
                // 10% new order (if active_orders was empty)
                event.type = MarketEventType::NewOrder;
                event.order_id = order_id++;
                event.price = (event.side == Side::Buy)
                    ? base_bid - price_offset_dist(rng_) * 10
                    : base_ask + price_offset_dist(rng_) * 10;
                event.quantity = qty_dist(rng_);
                event.priority = priority_dist(rng_);
                active_orders.push_back(event.order_id);
            }

            events.push_back(event);
        }

        return events;
    }

private:
    std::mt19937_64 rng_;
};

// ============================================================================
// Benchmark: L2 Market Replay - Low Frequency (100 events/sec)
// ============================================================================

static void BM_L2_MarketReplay_LowFreq(benchmark::State& state) {
    MarketDataGenerator gen(12345);
    auto events = gen.generateL2Events(1000, 100000, 100100);

    for (auto _ : state) {
        OrderBookL2 book(1);

        for (const auto& event : events) {
            switch (event.type) {
                case MarketEventType::NewLevel:
                case MarketEventType::ModifyLevel:
                    book.updateLevel(event.side, event.price, event.quantity, event.timestamp, 0);
                    break;
                case MarketEventType::DeleteLevel:
                    book.deleteLevel(event.side, event.price);
                    break;
                default:
                    break;
            }
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * events.size());
}

BENCHMARK(BM_L2_MarketReplay_LowFreq);

// ============================================================================
// Benchmark: L2 Market Replay - High Frequency (10000 events/sec)
// ============================================================================

static void BM_L2_MarketReplay_HighFreq(benchmark::State& state) {
    MarketDataGenerator gen(54321);
    auto events = gen.generateL2Events(10000, 100000, 100100);

    for (auto _ : state) {
        OrderBookL2 book(1);

        for (const auto& event : events) {
            switch (event.type) {
                case MarketEventType::NewLevel:
                case MarketEventType::ModifyLevel:
                    book.updateLevel(event.side, event.price, event.quantity, event.timestamp, 0);
                    break;
                case MarketEventType::DeleteLevel:
                    book.deleteLevel(event.side, event.price);
                    break;
                default:
                    break;
            }
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * events.size());
}

BENCHMARK(BM_L2_MarketReplay_HighFreq);

// ============================================================================
// Benchmark: L3 Market Replay - Low Frequency
// ============================================================================

static void BM_L3_MarketReplay_LowFreq(benchmark::State& state) {
    MarketDataGenerator gen(99999);
    auto events = gen.generateL3Events(1000, 100000, 100100);

    for (auto _ : state) {
        OrderBookL3 book(1);

        for (const auto& event : events) {
            switch (event.type) {
                case MarketEventType::NewOrder:
                case MarketEventType::ModifyOrder:
                    book.addOrModifyOrder(event.order_id, event.side, event.price,
                                         event.quantity, event.timestamp, event.priority, 0);
                    break;
                case MarketEventType::DeleteOrder:
                    book.deleteOrder(event.order_id, 0);
                    break;
                case MarketEventType::ExecuteOrder:
                    book.executeOrder(event.order_id, event.quantity, 0);
                    break;
                default:
                    break;
            }
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * events.size());
}

BENCHMARK(BM_L3_MarketReplay_LowFreq);

// ============================================================================
// Benchmark: L3 Market Replay - High Frequency
// ============================================================================

static void BM_L3_MarketReplay_HighFreq(benchmark::State& state) {
    MarketDataGenerator gen(77777);
    auto events = gen.generateL3Events(10000, 100000, 100100);

    for (auto _ : state) {
        OrderBookL3 book(1);

        for (const auto& event : events) {
            switch (event.type) {
                case MarketEventType::NewOrder:
                case MarketEventType::ModifyOrder:
                    book.addOrModifyOrder(event.order_id, event.side, event.price,
                                         event.quantity, event.timestamp, event.priority, 0);
                    break;
                case MarketEventType::DeleteOrder:
                    book.deleteOrder(event.order_id, 0);
                    break;
                case MarketEventType::ExecuteOrder:
                    book.executeOrder(event.order_id, event.quantity, 0);
                    break;
                default:
                    break;
            }
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * events.size());
}

BENCHMARK(BM_L3_MarketReplay_HighFreq);

// ============================================================================
// Benchmark: Multi-Symbol Market Replay
// ============================================================================

static void BM_MultiSymbol_MarketReplay(benchmark::State& state) {
    const auto num_symbols = state.range(0);
    MarketDataGenerator gen(11111);

    // Generate events for each symbol
    std::vector<std::vector<MarketEvent>> symbol_events;
    for (size_t i = 0; i < num_symbols; ++i) {
        symbol_events.push_back(gen.generateL2Events(1000, 100000, 100100));
    }

    for (auto _ : state) {
        OrderBookManager<OrderBookL2> manager;

        // Interleave events across symbols (simulate real market)
        size_t max_events = 0;
        for (const auto& events : symbol_events) {
            max_events = std::max(max_events, events.size());
        }

        for (size_t event_idx = 0; event_idx < max_events; ++event_idx) {
            for (size_t sym_idx = 0; sym_idx < num_symbols; ++sym_idx) {
                if (event_idx < symbol_events[sym_idx].size()) {
                    const auto& event = symbol_events[sym_idx][event_idx];
                    auto* book = manager.getOrCreateOrderBook(static_cast<SymbolId>(sym_idx + 1));

                    switch (event.type) {
                        case MarketEventType::NewLevel:
                        case MarketEventType::ModifyLevel:
                            book->updateLevel(event.side, event.price, event.quantity, event.timestamp, 0);
                            break;
                        case MarketEventType::DeleteLevel:
                            book->deleteLevel(event.side, event.price);
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        benchmark::DoNotOptimize(manager);
    }

    size_t total_events = 0;
    for (const auto& events : symbol_events) {
        total_events += events.size();
    }

    state.SetItemsProcessed(state.iterations() * total_events);
}

BENCHMARK(BM_MultiSymbol_MarketReplay)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Benchmark: Burst Pattern (Simulating Market Open)
// ============================================================================

static void BM_BurstPattern_MarketOpen(benchmark::State& state) {
    MarketDataGenerator gen(22222);

    for (auto _ : state) {
        OrderBookL2 book(1);

        // Initial snapshot burst (1000 levels)
        auto snapshot_events = gen.generateL2Events(1000, 100000, 100100);
        for (const auto& event : snapshot_events) {
            book.updateLevel(event.side, event.price, event.quantity, event.timestamp, 0);
        }

        // Followed by incremental updates
        auto incremental_events = gen.generateL2Events(5000, 100000, 100100);
        for (const auto& event : incremental_events) {
            switch (event.type) {
                case MarketEventType::NewLevel:
                case MarketEventType::ModifyLevel:
                    book.updateLevel(event.side, event.price, event.quantity, event.timestamp, 0);
                    break;
                case MarketEventType::DeleteLevel:
                    book.deleteLevel(event.side, event.price);
                    break;
                default:
                    break;
            }
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * 6000);
}

BENCHMARK(BM_BurstPattern_MarketOpen);

// ============================================================================
// Benchmark: Throughput - Sustained Updates
// ============================================================================

static void BM_Throughput_SustainedUpdates_L2(benchmark::State& state) {
    OrderBookL2 book(1);

    // Pre-populate book
    for (int i = 0; i < 100; ++i) {
        book.updateLevel(Side::Buy, 100000 - i * 10, 1000, 0, 0);
        book.updateLevel(Side::Sell, 100100 + i * 10, 1000, 0, 0);
    }

    std::mt19937_64 rng(33333);
    std::uniform_int_distribution<Quantity> qty_dist(100, 10000);
    std::uniform_int_distribution<int> price_offset_dist(0, 99);

    size_t updates = 0;

    for (auto _ : state) {
        Side side = (updates % 2 == 0) ? Side::Buy : Side::Sell;
        Price base = (side == Side::Buy) ? 100000 : 100100;
        Price offset = price_offset_dist(rng);
        Price price = (side == Side::Buy) ? base - offset * 10 : base + offset * 10;

        book.updateLevel(side, price, qty_dist(rng), updates, 0);
        ++updates;
    }

    state.SetItemsProcessed(updates);
}

BENCHMARK(BM_Throughput_SustainedUpdates_L2);

static void BM_Throughput_SustainedUpdates_L3(benchmark::State& state) {
    OrderBookL3 book(1);

    std::mt19937_64 rng(44444);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<int> price_offset_dist(0, 20);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    OrderId order_id = 1;
    size_t updates = 0;

    for (auto _ : state) {
        Side side = (updates % 2 == 0) ? Side::Buy : Side::Sell;
        Price base = (side == Side::Buy) ? 100000 : 100100;
        Price offset = price_offset_dist(rng);
        Price price = (side == Side::Buy) ? base - offset * 10 : base + offset * 10;

        book.addOrModifyOrder(order_id++, side, price, qty_dist(rng), updates, priority_dist(rng), 0);
        ++updates;

        // Reset periodically
        if (updates % 100000 == 0) {
            book = OrderBookL3(1);
            order_id = 1;
        }
    }

    state.SetItemsProcessed(updates);
}

BENCHMARK(BM_Throughput_SustainedUpdates_L3);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
