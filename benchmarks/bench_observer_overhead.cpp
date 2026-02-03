/**
 * @file bench_observer_overhead.cpp
 * @brief Performance benchmarks for Observer notification overhead
 *
 * Measures the cost of:
 * - Observer notifications (different observer counts)
 * - Notification dispatch latency
 * - Impact on orderbook operations
 * - Snapshot emission overhead
 *
 * Target: < 50ns per observer notification
 */

#include <slick/orderbook/orderbook.hpp>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <atomic>

using namespace slick::orderbook;

// ============================================================================
// Test Observer Implementations
// ============================================================================

// Minimal observer that just counts notifications
class CountingObserver : public IOrderBookObserver {
public:
    void onPriceLevelUpdate([[maybe_unused]] const PriceLevelUpdate& update) override {
        ++level_update_count_;
    }

    void onOrderUpdate([[maybe_unused]] const OrderUpdate& update) override {
        ++order_update_count_;
    }

    void onTrade([[maybe_unused]] const Trade& trade) override {
        ++trade_count_;
    }

    void onTopOfBookUpdate([[maybe_unused]] const TopOfBook& tob) override {
        ++tob_change_count_;
    }

    void onSnapshotBegin([[maybe_unused]] SymbolId symbol, [[maybe_unused]] uint64_t seq_num, [[maybe_unused]] Timestamp timestamp) override {
        ++snapshot_begin_count_;
    }

    void onSnapshotEnd([[maybe_unused]] SymbolId symbol, [[maybe_unused]] uint64_t seq_num, [[maybe_unused]] Timestamp timestamp) override {
        ++snapshot_end_count_;
    }

    size_t level_update_count_ = 0;
    size_t order_update_count_ = 0;
    size_t trade_count_ = 0;
    size_t tob_change_count_ = 0;
    size_t snapshot_begin_count_ = 0;
    size_t snapshot_end_count_ = 0;
};

// Observer that does some computation
class ComputingObserver : public IOrderBookObserver {
public:
    void onPriceLevelUpdate(const PriceLevelUpdate& update) override {
        // Simulate some work
        computed_value_ += update.price * update.quantity;
        benchmark::DoNotOptimize(computed_value_);
    }

    void onOrderUpdate(const OrderUpdate& update) override {
        computed_value_ += update.price * update.quantity;
        benchmark::DoNotOptimize(computed_value_);
    }

    void onTrade(const Trade& trade) override {
        computed_value_ += trade.price * trade.quantity;
        benchmark::DoNotOptimize(computed_value_);
    }

    void onTopOfBookUpdate(const TopOfBook& tob) override {
        if (tob.best_bid && tob.best_ask) {
            computed_value_ += (tob.best_bid + tob.best_ask) / 2;
        }
        benchmark::DoNotOptimize(computed_value_);
    }

    void onSnapshotBegin([[maybe_unused]] SymbolId symbol, [[maybe_unused]] uint64_t seq_num, [[maybe_unused]] Timestamp timestamp) override {
        computed_value_ = 0;
    }

    void onSnapshotEnd([[maybe_unused]] SymbolId symbol, [[maybe_unused]] uint64_t seq_num, [[maybe_unused]] Timestamp timestamp) override {
        benchmark::DoNotOptimize(computed_value_);
    }

    int64_t computed_value_ = 0;
};

// ============================================================================
// Benchmark: L2 Operations - No Observers
// ============================================================================

static void BM_L2_NoObservers(benchmark::State& state) {
    OrderBookL2 book(1);

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<Quantity> qty_dist(100, 10000);

    size_t operations = 0;

    for (auto _ : state) {
        Price price = 100000 + (operations % 100) * 10;
        Side side = (operations % 2 == 0) ? Side::Buy : Side::Sell;
        book.updateLevel(side, price, qty_dist(rng), 0, 0);
        ++operations;
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_L2_NoObservers);

// ============================================================================
// Benchmark: L2 Operations - With Counting Observers
// ============================================================================

static void BM_L2_WithCountingObservers(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_observers = state.range(0);

    // Add observers
    std::vector<std::shared_ptr<CountingObserver>> observers;
    for (auto i = 0; i < num_observers; ++i) {
        observers.push_back(std::make_shared<CountingObserver>());
        book.addObserver(observers.back());
    }

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<Quantity> qty_dist(100, 10000);

    size_t operations = 0;

    for (auto _ : state) {
        Price price = 100000 + (operations % 100) * 10;
        Side side = (operations % 2 == 0) ? Side::Buy : Side::Sell;
        book.updateLevel(side, price, qty_dist(rng), 0, 0);
        ++operations;
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_L2_WithCountingObservers)->Arg(1)->Arg(5)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: L2 Operations - With Computing Observers
// ============================================================================

static void BM_L2_WithComputingObservers(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_observers = state.range(0);

    // Add observers
    std::vector<std::shared_ptr<ComputingObserver>> observers;
    for (auto i = 0; i < num_observers; ++i) {
        observers.push_back(std::make_shared<ComputingObserver>());
        book.addObserver(observers.back());
    }

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<Quantity> qty_dist(100, 10000);

    size_t operations = 0;

    for (auto _ : state) {
        Price price = 100000 + (operations % 100) * 10;
        Side side = (operations % 2 == 0) ? Side::Buy : Side::Sell;
        book.updateLevel(side, price, qty_dist(rng), 0, 0);
        ++operations;
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_L2_WithComputingObservers)->Arg(1)->Arg(5)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: L3 Operations - No Observers
// ============================================================================

static void BM_L3_NoObservers(benchmark::State& state) {
    OrderBookL3 book(1);

    std::mt19937_64 rng(54321);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    OrderId order_id = 1;
    size_t operations = 0;

    for (auto _ : state) {
        Price price = 100000 + (operations % 20) * 10;
        Side side = (operations % 2 == 0) ? Side::Buy : Side::Sell;
        book.addOrModifyOrder(order_id++, side, price, qty_dist(rng), 0, priority_dist(rng), 0);
        ++operations;

        if (operations % 10000 == 0) {
            book = OrderBookL3(1);
            order_id = 1;
        }
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_L3_NoObservers);

// ============================================================================
// Benchmark: L3 Operations - With Counting Observers
// ============================================================================

static void BM_L3_WithCountingObservers(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_observers = state.range(0);

    // Add observers
    std::vector<std::shared_ptr<CountingObserver>> observers;
    for (auto i = 0; i < num_observers; ++i) {
        observers.push_back(std::make_shared<CountingObserver>());
        book.addObserver(observers.back());
    }

    std::mt19937_64 rng(54321);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    OrderId order_id = 1;
    size_t operations = 0;

    for (auto _ : state) {
        Price price = 100000 + (operations % 20) * 10;
        Side side = (operations % 2 == 0) ? Side::Buy : Side::Sell;
        book.addOrModifyOrder(order_id++, side, price, qty_dist(rng), 0, priority_dist(rng), 0);
        ++operations;

        if (operations % 10000 == 0) {
            book = OrderBookL3(1);
            for (auto& obs : observers) {
                book.addObserver(obs);
            }
            order_id = 1;
        }
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_L3_WithCountingObservers)->Arg(1)->Arg(5)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: L3 Operations - With Computing Observers
// ============================================================================

static void BM_L3_WithComputingObservers(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_observers = state.range(0);

    // Add observers
    std::vector<std::shared_ptr<ComputingObserver>> observers;
    for (auto i = 0; i < num_observers; ++i) {
        observers.push_back(std::make_shared<ComputingObserver>());
        book.addObserver(observers.back());
    }

    std::mt19937_64 rng(54321);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    OrderId order_id = 1;
    size_t operations = 0;

    for (auto _ : state) {
        Price price = 100000 + (operations % 20) * 10;
        Side side = (operations % 2 == 0) ? Side::Buy : Side::Sell;
        book.addOrModifyOrder(order_id++, side, price, qty_dist(rng), 0, priority_dist(rng), 0);
        ++operations;

        if (operations % 10000 == 0) {
            book = OrderBookL3(1);
            for (auto& obs : observers) {
                book.addObserver(obs);
            }
            order_id = 1;
        }
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_L3_WithComputingObservers)->Arg(1)->Arg(5)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Benchmark: Snapshot Emission - L2
// ============================================================================

static void BM_L2_EmitSnapshot(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_levels = state.range(0);

    // Build book with num_levels on each side
    for (auto i = 0; i < num_levels; ++i) {
        book.updateLevel(Side::Buy, 100000 - static_cast<Price>(i) * 10, 1000, 0, 0);
        book.updateLevel(Side::Sell, 100100 + static_cast<Price>(i) * 10, 1000, 0, 0);
    }

    auto observer = std::make_shared<CountingObserver>();
    book.addObserver(observer);

    for (auto _ : state) {
        book.emitSnapshot(0);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L2_EmitSnapshot)->Arg(10)->Arg(50)->Arg(100)->Arg(500);

// ============================================================================
// Benchmark: Snapshot Emission - L3
// ============================================================================

static void BM_L3_EmitSnapshot(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_orders = state.range(0);

    // Build book with num_orders
    std::mt19937_64 rng(99999);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<Price> price_offset_dist(0, 10);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    for (auto i = 0; i < num_orders; ++i) {
        Price price = 100000 + static_cast<Price>(price_offset_dist(rng)) * 10;
        Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
        book.addOrModifyOrder(i + 1, side, price, qty_dist(rng), 0, priority_dist(rng), 0);
    }

    auto observer = std::make_shared<CountingObserver>();
    book.addObserver(observer);

    for (auto _ : state) {
        book.emitSnapshot(0);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L3_EmitSnapshot)->Arg(100)->Arg(500)->Arg(1000)->Arg(5000);

// ============================================================================
// Benchmark: Observer Add/Remove Overhead
// ============================================================================

static void BM_AddRemoveObserver(benchmark::State& state) {
    OrderBookL2 book(1);
    const auto num_observers = state.range(0);

    std::vector<std::shared_ptr<CountingObserver>> observers;
    for (auto i = 0; i < num_observers; ++i) {
        observers.push_back(std::make_shared<CountingObserver>());
    }

    for (auto _ : state) {
        // Add all observers
        for (auto& obs : observers) {
            book.addObserver(obs);
        }

        // Remove all observers
        for (auto& obs : observers) {
            book.removeObserver(obs);
        }
    }

    state.SetItemsProcessed(state.iterations() * num_observers * 2);
}

BENCHMARK(BM_AddRemoveObserver)->Arg(1)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
