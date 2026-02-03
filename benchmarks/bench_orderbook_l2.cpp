/**
 * @file bench_orderbook_l2.cpp
 * @brief Performance benchmarks for OrderBookL2
 *
 * Measures latency of core operations:
 * - addOrModifyLevel (new level insertion)
 * - addOrModifyLevel (quantity modification)
 * - deleteLevel
 * - getBestBid/getBestAsk
 * - getTopOfBook
 * - Spread calculations
 *
 * Target: < 100ns p99 latency for all operations
 */

#include <slick/orderbook/orderbook.hpp>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

using namespace slick::orderbook;

// ============================================================================
// Test Data Generation
// ============================================================================

struct PriceLevelData {
    Price price;
    Quantity quantity;
};

std::vector<PriceLevelData> generateRandomLevels(size_t count, Price base_price, Price spread) {
    std::vector<PriceLevelData> levels;
    levels.reserve(count);

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<Quantity> qty_dist(100, 10000);

    for (size_t i = 0; i < count; ++i) {
        levels.push_back({
            base_price + static_cast<Price>(i) * spread,
            qty_dist(rng)
        });
    }

    return levels;
}

// ============================================================================
// Benchmark: Add New Level
// ============================================================================

static void BM_L2_AddNewLevel(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_levels = state.range(0);

    // Pre-generate price levels
    auto bid_levels = generateRandomLevels(num_levels, 100000, -10);
    auto ask_levels = generateRandomLevels(num_levels, 100100, 10);

    size_t idx = 0;
    for (auto _ : state) {
        const auto& level = (idx % 2 == 0) ? bid_levels[idx % bid_levels.size()]
                                            : ask_levels[idx % ask_levels.size()];
        const Side side = (idx % 2 == 0) ? Side::Buy : Side::Sell;

        book.updateLevel(side, level.price, level.quantity, 0, 0);

        ++idx;

        // Prevent book from growing indefinitely
        if (idx % (num_levels * 4) == 0) {
            book = OrderBookL2(1);
            idx = 0;
        }
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L2_AddNewLevel)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: Modify Existing Level
// ============================================================================

static void BM_L2_ModifyExistingLevel(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_levels = state.range(0);

    // Build initial book
    auto bid_levels = generateRandomLevels(num_levels, 100000, -10);
    auto ask_levels = generateRandomLevels(num_levels, 100100, 10);

    for (const auto& level : bid_levels) {
        book.updateLevel(Side::Buy, level.price, level.quantity, 0, 0);
    }
    for (const auto& level : ask_levels) {
        book.updateLevel(Side::Sell, level.price, level.quantity, 0, 0);
    }

    std::mt19937_64 rng(54321);
    std::uniform_int_distribution<Quantity> qty_dist(100, 10000);
    size_t idx = 0;

    for (auto _ : state) {
        const auto& level = (idx % 2 == 0) ? bid_levels[idx % bid_levels.size()]
                                            : ask_levels[idx % ask_levels.size()];
        const Side side = (idx % 2 == 0) ? Side::Buy : Side::Sell;
        const Quantity new_qty = qty_dist(rng);

        book.updateLevel(side, level.price, new_qty, 0, 0);

        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L2_ModifyExistingLevel)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: Delete Level
// ============================================================================

static void BM_L2_DeleteLevel(benchmark::State& state) {
    const auto num_levels = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();

        // Setup: build book with num_levels on each side
        OrderBookL2 book(1);
        auto bid_levels = generateRandomLevels(num_levels, 100000, -10);
        auto ask_levels = generateRandomLevels(num_levels, 100100, 10);

        for (const auto& level : bid_levels) {
            book.updateLevel(Side::Buy, level.price, level.quantity, 0, 0);
        }
        for (const auto& level : ask_levels) {
            book.updateLevel(Side::Sell, level.price, level.quantity, 0, 0);
        }

        state.ResumeTiming();

        // Delete all levels
        for (const auto& level : bid_levels) {
            book.deleteLevel(Side::Buy, level.price);
        }
        for (const auto& level : ask_levels) {
            book.deleteLevel(Side::Sell, level.price);
        }
    }

    state.SetItemsProcessed(state.iterations() * num_levels * 2);
}

BENCHMARK(BM_L2_DeleteLevel)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: Get Best Bid/Ask (Hot Path)
// ============================================================================

static void BM_L2_GetBestBid(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_levels = state.range(0);

    // Build book
    auto bid_levels = generateRandomLevels(num_levels, 100000, -10);
    for (const auto& level : bid_levels) {
        book.updateLevel(Side::Buy, level.price, level.quantity, 0, 0);
    }

    for (auto _ : state) {
        auto best = book.getBestBid();
        benchmark::DoNotOptimize(best);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L2_GetBestBid)->Arg(10)->Arg(50)->Arg(100);

static void BM_L2_GetBestAsk(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_levels = state.range(0);

    // Build book
    auto ask_levels = generateRandomLevels(num_levels, 100100, 10);
    for (const auto& level : ask_levels) {
        book.updateLevel(Side::Sell, level.price, level.quantity, 0, 0);
    }

    for (auto _ : state) {
        auto best = book.getBestAsk();
        benchmark::DoNotOptimize(best);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L2_GetBestAsk)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: Get Top of Book
// ============================================================================

static void BM_L2_GetTopOfBook(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_levels = state.range(0);

    // Build book
    auto bid_levels = generateRandomLevels(num_levels, 100000, -10);
    auto ask_levels = generateRandomLevels(num_levels, 100100, 10);

    for (const auto& level : bid_levels) {
        book.updateLevel(Side::Buy, level.price, level.quantity, 0, 0);
    }
    for (const auto& level : ask_levels) {
        book.updateLevel(Side::Sell, level.price, level.quantity, 0, 0);
    }

    for (auto _ : state) {
        auto tob = book.getTopOfBook();
        benchmark::DoNotOptimize(tob);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L2_GetTopOfBook)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: Get Levels (Iteration)
// ============================================================================

static void BM_L2_GetLevels(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_levels = state.range(0);

    // Build book
    auto bid_levels = generateRandomLevels(num_levels, 100000, -10);
    auto ask_levels = generateRandomLevels(num_levels, 100100, 10);

    for (const auto& level : bid_levels) {
        book.updateLevel(Side::Buy, level.price, level.quantity, 0, 0);
    }
    for (const auto& level : ask_levels) {
        book.updateLevel(Side::Sell, level.price, level.quantity, 0, 0);
    }

    for (auto _ : state) {
        const auto& bids = book.getLevels(Side::Buy);
        const auto& asks = book.getLevels(Side::Sell);

        benchmark::DoNotOptimize(bids);
        benchmark::DoNotOptimize(asks);

        // Iterate through levels
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

BENCHMARK(BM_L2_GetLevels)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: Mixed Workload (Realistic)
// ============================================================================

static void BM_L2_MixedWorkload(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_levels = state.range(0);

    // Build initial book
    auto bid_levels = generateRandomLevels(num_levels, 100000, -10);
    auto ask_levels = generateRandomLevels(num_levels, 100100, 10);

    for (const auto& level : bid_levels) {
        book.updateLevel(Side::Buy, level.price, level.quantity, 0, 0);
    }
    for (const auto& level : ask_levels) {
        book.updateLevel(Side::Sell, level.price, level.quantity, 0, 0);
    }

    std::mt19937_64 rng(99999);
    std::uniform_int_distribution<int> action_dist(0, 9);  // 0-9
    std::uniform_int_distribution<Quantity> qty_dist(100, 10000);
    std::uniform_int_distribution<size_t> level_dist(0, num_levels - 1);

    size_t operations = 0;

    for (auto _ : state) {
        int action = action_dist(rng);
        Side side = (action % 2 == 0) ? Side::Buy : Side::Sell;
        const auto& levels = (side == Side::Buy) ? bid_levels : ask_levels;

        if (action < 4) {
            // 40% modify existing level
            const auto& level = levels[level_dist(rng)];
            book.updateLevel(side, level.price, qty_dist(rng), 0, 0);
        } else if (action < 7) {
            // 30% add new level
            Price new_price = (side == Side::Buy) ? (100000 - (num_levels + 10) * 10)
                                                   : (100100 + (num_levels + 10) * 10);
            book.updateLevel(side, new_price, qty_dist(rng), 0, 0);
        } else if (action < 9) {
            // 20% delete level
            if (!levels.empty()) {
                const auto& level = levels[level_dist(rng)];
                book.deleteLevel(side, level.price);
            }
        } else {
            // 10% query top of book
            auto tob = book.getTopOfBook();
            benchmark::DoNotOptimize(tob);
        }

        ++operations;
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_L2_MixedWorkload)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
