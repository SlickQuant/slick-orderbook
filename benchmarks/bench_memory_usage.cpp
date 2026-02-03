/**
 * @file bench_memory_usage.cpp
 * @brief Memory usage profiling and benchmarks
 *
 * Measures memory footprint and allocation patterns:
 * - L2 orderbook memory per symbol
 * - L3 orderbook memory per symbol
 * - Memory growth patterns
 * - Object pool efficiency
 * - Memory locality and cache performance
 *
 * Target: L2 < 1KB, L3 < 10KB per symbol (1000 orders)
 */

#include <slick/orderbook/orderbook.hpp>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

using namespace slick::orderbook;

// ============================================================================
// Memory Tracking Utilities
// ============================================================================

// Note: For accurate memory measurements, run with tools like:
// - Valgrind massif: valgrind --tool=massif ./bench_memory_usage
// - heaptrack: heaptrack ./bench_memory_usage
// - perf: perf mem record ./bench_memory_usage

// These benchmarks measure runtime and can give relative memory comparisons
// For absolute memory usage, use external profiling tools

// ============================================================================
// Benchmark: L2 Memory Footprint (Different Sizes)
// ============================================================================

static void BM_L2_MemoryFootprint(benchmark::State& state) {
    const auto num_levels = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookL2 book(1);
        state.ResumeTiming();

        // Build book with num_levels on each side
        for (size_t i = 0; i < num_levels; ++i) {
            book.updateLevel(Side::Buy, 100000 - static_cast<Price>(i) * 10, 1000, 0, 0);
            book.updateLevel(Side::Sell, 100100 + static_cast<Price>(i) * 10, 1000, 0, 0);
        }

        benchmark::DoNotOptimize(book);

        state.PauseTiming();
        // book destroyed here
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel(std::to_string(num_levels) + " levels/side");
}

BENCHMARK(BM_L2_MemoryFootprint)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000);

// ============================================================================
// Benchmark: L3 Memory Footprint (Different Sizes)
// ============================================================================

static void BM_L3_MemoryFootprint(benchmark::State& state) {
    const auto num_orders = state.range(0);

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<Price> price_offset_dist(0, 10);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookL3 book(1);
        state.ResumeTiming();

        // Build book with num_orders
        for (size_t i = 0; i < num_orders; ++i) {
            Price price = 100000 + static_cast<Price>(price_offset_dist(rng)) * 10;
            Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
            book.addOrModifyOrder(i + 1, side, price, qty_dist(rng), 0, priority_dist(rng), 0);
        }

        benchmark::DoNotOptimize(book);

        state.PauseTiming();
        // book destroyed here
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel(std::to_string(num_orders) + " orders");
}

BENCHMARK(BM_L3_MemoryFootprint)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Arg(5000)
    ->Arg(10000);

// ============================================================================
// Benchmark: Memory Growth Pattern - L2
// ============================================================================

static void BM_L2_MemoryGrowth(benchmark::State& state) {
    for (auto _ : state) {
        OrderBookL2 book(1);

        // Grow from 0 to 1000 levels incrementally
        for (size_t i = 0; i < 1000; ++i) {
            book.updateLevel(Side::Buy, 100000 - static_cast<Price>(i) * 10, 1000, 0, 0);
            book.updateLevel(Side::Sell, 100100 + static_cast<Price>(i) * 10, 1000, 0, 0);
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L2_MemoryGrowth);

// ============================================================================
// Benchmark: Memory Growth Pattern - L3
// ============================================================================

static void BM_L3_MemoryGrowth(benchmark::State& state) {
    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<Price> price_offset_dist(0, 10);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    for (auto _ : state) {
        OrderBookL3 book(1);

        // Grow from 0 to 10000 orders incrementally
        for (size_t i = 0; i < 10000; ++i) {
            Price price = 100000 + static_cast<Price>(price_offset_dist(rng)) * 10;
            Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
            book.addOrModifyOrder(i + 1, side, price, qty_dist(rng), 0, priority_dist(rng), 0);
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L3_MemoryGrowth);

// ============================================================================
// Benchmark: Memory Churn - L2 (Add/Delete Cycles)
// ============================================================================

static void BM_L2_MemoryChurn(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_levels = state.range(0);

    size_t cycle = 0;

    for (auto _ : state) {
        // Add levels
        for (size_t i = 0; i < num_levels; ++i) {
            Price offset = static_cast<Price>(cycle * 1000 + i);
            book.updateLevel(Side::Buy, 100000 - offset * 10, 1000, 0, 0);
            book.updateLevel(Side::Sell, 100100 + offset * 10, 1000, 0, 0);
        }

        // Delete levels
        for (size_t i = 0; i < num_levels; ++i) {
            Price offset = static_cast<Price>(cycle * 1000 + i);
            book.deleteLevel(Side::Buy, 100000 - offset * 10);
            book.deleteLevel(Side::Sell, 100100 + offset * 10);
        }

        ++cycle;
    }

    state.SetItemsProcessed(state.iterations() * num_levels * 2);
}

BENCHMARK(BM_L2_MemoryChurn)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: Memory Churn - L3 (Add/Delete Cycles)
// ============================================================================

static void BM_L3_MemoryChurn(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_orders = state.range(0);

    std::mt19937_64 rng(54321);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    OrderId order_id = 1;
    std::vector<OrderId> order_ids;
    order_ids.reserve(num_orders);

    for (auto _ : state) {
        order_ids.clear();

        // Add orders
        for (size_t i = 0; i < num_orders; ++i) {
            Price price = 100000 + (i % 10) * 10;
            Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
            book.addOrModifyOrder(order_id, side, price, qty_dist(rng), 0, priority_dist(rng), 0);
            order_ids.push_back(order_id);
            ++order_id;
        }

        // Delete orders
        for (OrderId id : order_ids) {
            book.deleteOrder(id, 0);
        }
    }

    state.SetItemsProcessed(state.iterations() * num_orders);
}

BENCHMARK(BM_L3_MemoryChurn)->Arg(100)->Arg(500)->Arg(1000);

// ============================================================================
// Benchmark: Multi-Symbol Memory Footprint
// ============================================================================

static void BM_Manager_MemoryFootprint_L2(benchmark::State& state) {
    const auto num_symbols = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookManager<OrderBookL2> manager;
        state.ResumeTiming();

        // Create orderbooks for num_symbols
        for (SymbolId i = 1; i <= num_symbols; ++i) {
            auto* book = manager.getOrCreateOrderBook(i);

            // Populate with 10 levels each
            for (int j = 0; j < 10; ++j) {
                book->updateLevel(Side::Buy, 100000 - j * 10, 1000, 0, 0);
                book->updateLevel(Side::Sell, 100100 + j * 10, 1000, 0, 0);
            }
        }

        benchmark::DoNotOptimize(manager);

        state.PauseTiming();
        // manager destroyed here
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel(std::to_string(num_symbols) + " symbols");
}

BENCHMARK(BM_Manager_MemoryFootprint_L2)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// ============================================================================
// Benchmark: Multi-Symbol Memory Footprint - L3
// ============================================================================

static void BM_Manager_MemoryFootprint_L3(benchmark::State& state) {
    const auto num_symbols = state.range(0);

    std::mt19937_64 rng(99999);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookManager<OrderBookL3> manager;
        OrderId order_id = 1;
        state.ResumeTiming();

        // Create orderbooks for num_symbols
        for (SymbolId i = 1; i <= num_symbols; ++i) {
            auto* book = manager.getOrCreateOrderBook(i);

            // Populate with 100 orders each
            for (int j = 0; j < 100; ++j) {
                Price price = 100000 + (j % 10) * 10;
                Side side = (j % 2 == 0) ? Side::Buy : Side::Sell;
                book->addOrModifyOrder(order_id++, side, price, qty_dist(rng), 0, priority_dist(rng), 0);
            }
        }

        benchmark::DoNotOptimize(manager);

        state.PauseTiming();
        // manager destroyed here
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel(std::to_string(num_symbols) + " symbols");
}

BENCHMARK(BM_Manager_MemoryFootprint_L3)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

// ============================================================================
// Benchmark: Cache Locality - Sequential Access
// ============================================================================

static void BM_L2_CacheLocality_Sequential(benchmark::State& state) {
    OrderBookL2 book(1);

    // Build book with 100 levels
    for (size_t i = 0; i < 100; ++i) {
        book.updateLevel(Side::Buy, 100000 - static_cast<Price>(i) * 10, 1000, 0, 0);
        book.updateLevel(Side::Sell, 100100 + static_cast<Price>(i) * 10, 1000, 0, 0);
    }

    for (auto _ : state) {
        // Sequential iteration through all levels
        const auto& bids = book.getLevels(Side::Buy);
        const auto& asks = book.getLevels(Side::Sell);

        Quantity total_bid_qty = 0;
        Quantity total_ask_qty = 0;

        for (const auto& level : bids) {
            total_bid_qty += level.quantity;
        }

        for (const auto& level : asks) {
            total_ask_qty += level.quantity;
        }

        benchmark::DoNotOptimize(total_bid_qty);
        benchmark::DoNotOptimize(total_ask_qty);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L2_CacheLocality_Sequential);

// ============================================================================
// Benchmark: Cache Locality - Random Access
// ============================================================================

static void BM_L2_CacheLocality_Random(benchmark::State& state) {
    OrderBookL2 book(1);
    std::vector<Price> bid_prices;
    std::vector<Price> ask_prices;

    // Build book with 100 levels
    for (size_t i = 0; i < 100; ++i) {
        Price bid_price = 100000 - static_cast<Price>(i) * 10;
        Price ask_price = 100100 + static_cast<Price>(i) * 10;
        book.updateLevel(Side::Buy, bid_price, 1000, 0, 0);
        book.updateLevel(Side::Sell, ask_price, 1000, 0, 0);
        bid_prices.push_back(bid_price);
        ask_prices.push_back(ask_price);
    }

    std::mt19937_64 rng(77777);
    std::uniform_int_distribution<Quantity> qty_dist(500, 1500);
    std::uniform_int_distribution<size_t> price_dist(0, bid_prices.size() - 1);

    for (auto _ : state) {
        // Random access updates
        for (int i = 0; i < 10; ++i) {
            size_t idx = price_dist(rng);
            book.updateLevel(Side::Buy, bid_prices[idx], qty_dist(rng), 0, 0);
            book.updateLevel(Side::Sell, ask_prices[idx], qty_dist(rng), 0, 0);
        }
    }

    state.SetItemsProcessed(state.iterations() * 20);
}

BENCHMARK(BM_L2_CacheLocality_Random);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
