/**
 * @file bench_orderbook_manager.cpp
 * @brief Performance benchmarks for OrderBookManager
 *
 * Measures latency of multi-symbol operations:
 * - getOrCreateOrderBook
 * - Multi-symbol update distribution
 * - Symbol lookup overhead
 * - Concurrent access patterns
 * - Symbol churn (add/remove)
 *
 * Target: Minimal overhead compared to single-symbol operations
 */

#include <slick/orderbook/orderbook.hpp>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <thread>

using namespace slick::orderbook;

// ============================================================================
// Test Data Generation
// ============================================================================

std::vector<SymbolId> generateSymbolIds(size_t count, SymbolId start_id = 1) {
    std::vector<SymbolId> symbols;
    symbols.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        symbols.push_back(start_id + static_cast<SymbolId>(i));
    }
    return symbols;
}

// ============================================================================
// Benchmark: Get Or Create OrderBook (L2)
// ============================================================================

static void BM_Manager_GetOrCreateL2(benchmark::State& state) {
    OrderBookManager<OrderBookL2> manager;
    const auto num_symbols = state.range(0);
    auto symbols = generateSymbolIds(num_symbols);

    // Pre-create some symbols
    for (size_t i = 0; i < num_symbols / 2; ++i) {
        manager.getOrCreateOrderBook(symbols[i]);
    }

    size_t idx = 0;
    for (auto _ : state) {
        auto* book = manager.getOrCreateOrderBook(symbols[idx % symbols.size()]);
        benchmark::DoNotOptimize(book);
        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Manager_GetOrCreateL2)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Benchmark: Get Or Create OrderBook (L3)
// ============================================================================

static void BM_Manager_GetOrCreateL3(benchmark::State& state) {
    OrderBookManager<OrderBookL3> manager;
    const auto num_symbols = state.range(0);
    auto symbols = generateSymbolIds(num_symbols);

    // Pre-create some symbols
    for (size_t i = 0; i < num_symbols / 2; ++i) {
        manager.getOrCreateOrderBook(symbols[i]);
    }

    size_t idx = 0;
    for (auto _ : state) {
        auto* book = manager.getOrCreateOrderBook(symbols[idx % symbols.size()]);
        benchmark::DoNotOptimize(book);
        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Manager_GetOrCreateL3)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Benchmark: Multi-Symbol L2 Updates
// ============================================================================

static void BM_Manager_MultiSymbolL2Updates(benchmark::State& state) {
    OrderBookManager<OrderBookL2> manager;
    const auto num_symbols = state.range(0);
    auto symbols = generateSymbolIds(num_symbols);

    // Create all orderbooks upfront
    for (auto symbol_id : symbols) {
        manager.getOrCreateOrderBook(symbol_id);
    }

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<size_t> symbol_dist(0, symbols.size() - 1);
    std::uniform_int_distribution<Quantity> qty_dist(100, 10000);

    size_t operations = 0;

    for (auto _ : state) {
        SymbolId symbol = symbols[symbol_dist(rng)];
        auto* book = manager.getOrCreateOrderBook(symbol);

        // Add a level
        Price price = 100000 + (operations % 100) * 10;
        Side side = (operations % 2 == 0) ? Side::Buy : Side::Sell;

        book->updateLevel(side, price, qty_dist(rng), 0, 0);

        ++operations;
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_Manager_MultiSymbolL2Updates)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Benchmark: Multi-Symbol L3 Updates
// ============================================================================

static void BM_Manager_MultiSymbolL3Updates(benchmark::State& state) {
    OrderBookManager<OrderBookL3> manager;
    const auto num_symbols = state.range(0);
    auto symbols = generateSymbolIds(num_symbols);

    // Create all orderbooks upfront
    for (auto symbol_id : symbols) {
        manager.getOrCreateOrderBook(symbol_id);
    }

    std::mt19937_64 rng(54321);
    std::uniform_int_distribution<size_t> symbol_dist(0, symbols.size() - 1);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    OrderId order_id = 1;
    size_t operations = 0;

    for (auto _ : state) {
        SymbolId symbol = symbols[symbol_dist(rng)];
        auto* book = manager.getOrCreateOrderBook(symbol);

        // Add an order
        Price price = 100000 + (operations % 20) * 10;
        Side side = (operations % 2 == 0) ? Side::Buy : Side::Sell;

        book->addOrModifyOrder(order_id++, side, price, qty_dist(rng), 0, priority_dist(rng), 0);

        ++operations;

        // Reset periodically to prevent unbounded growth
        if (operations % 10000 == 0) {
            manager.clear();
            for (auto sym : symbols) {
                manager.getOrCreateOrderBook(sym);
            }
            order_id = 1;
        }
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_Manager_MultiSymbolL3Updates)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Benchmark: Symbol Lookup Overhead
// ============================================================================

static void BM_Manager_SymbolLookup(benchmark::State& state) {
    OrderBookManager<OrderBookL2> manager;
    const auto num_symbols = state.range(0);
    auto symbols = generateSymbolIds(num_symbols);

    // Create all orderbooks
    for (auto symbol_id : symbols) {
        manager.getOrCreateOrderBook(symbol_id);
    }

    std::mt19937_64 rng(99999);
    std::uniform_int_distribution<size_t> symbol_dist(0, symbols.size() - 1);

    for (auto _ : state) {
        SymbolId symbol = symbols[symbol_dist(rng)];
        auto* book = manager.getOrderBook(symbol);
        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Manager_SymbolLookup)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);

// ============================================================================
// Benchmark: Symbol Churn (Add/Remove)
// ============================================================================

static void BM_Manager_SymbolChurn(benchmark::State& state) {
    OrderBookManager<OrderBookL2> manager;
    const auto active_symbols = state.range(0);

    SymbolId next_symbol_id = 1;
    size_t operations = 0;

    for (auto _ : state) {
        // Add a new symbol
        auto* book = manager.getOrCreateOrderBook(next_symbol_id);
        book->updateLevel(Side::Buy, 100000, 1000, 0, 0);

        ++next_symbol_id;
        ++operations;

        // Remove oldest symbol if we exceed active_symbols
        if (next_symbol_id > static_cast<SymbolId>(active_symbols)) {
            SymbolId to_remove = next_symbol_id - active_symbols - 1;
            manager.removeOrderBook(to_remove);
        }
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_Manager_SymbolChurn)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Benchmark: Iteration Over All Symbols
// ============================================================================

static void BM_Manager_IterateAllSymbols(benchmark::State& state) {
    OrderBookManager<OrderBookL2> manager;
    const auto num_symbols = state.range(0);
    auto symbols = generateSymbolIds(num_symbols);

    // Create orderbooks with some data
    for (auto symbol_id : symbols) {
        auto* book = manager.getOrCreateOrderBook(symbol_id);
        book->updateLevel(Side::Buy, 100000, 1000, 0, 0);
        book->updateLevel(Side::Sell, 100100, 1000, 0, 0);
    }

    for (auto _ : state) {
        // Iterate through all symbols
        size_t count = 0;
        Quantity total_bid_qty = 0;

        auto all_symbols = manager.getSymbols();
        for (auto symbol : all_symbols) {
            ++count;
            auto* book = manager.getOrderBook(symbol);
            if (book) {
                auto best_bid = book->getBestBid();
                if (best_bid) {
                    total_bid_qty += best_bid->quantity;
                }
            }
        }

        benchmark::DoNotOptimize(count);
        benchmark::DoNotOptimize(total_bid_qty);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Manager_IterateAllSymbols)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Benchmark: Concurrent Symbol Access (Read-Heavy)
// ============================================================================

static void BM_Manager_ConcurrentReadHeavy(benchmark::State& state) {
    static OrderBookManager<OrderBookL2> manager;
    static std::once_flag init_flag;

    // Initialize once across all threads
    std::call_once(init_flag, [&]() {
        auto symbols = generateSymbolIds(100);
        for (auto symbol_id : symbols) {
            auto* book = manager.getOrCreateOrderBook(symbol_id);
            // Populate with initial data
            for (int i = 0; i < 10; ++i) {
                book->updateLevel(Side::Buy, 100000 - i * 10, 1000, 0, 0);
                book->updateLevel(Side::Sell, 100100 + i * 10, 1000, 0, 0);
            }
        }
    });

    auto symbols = generateSymbolIds(100);
    std::mt19937_64 rng(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_int_distribution<size_t> symbol_dist(0, symbols.size() - 1);

    for (auto _ : state) {
        SymbolId symbol = symbols[symbol_dist(rng)];
        auto* book = manager.getOrderBook(symbol);

        if (book) {
            auto tob = book->getTopOfBook();
            benchmark::DoNotOptimize(tob);
        }
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Manager_ConcurrentReadHeavy)->Threads(1)->Threads(2)->Threads(4)->Threads(8);

// ============================================================================
// Benchmark: Comparison - Single Symbol vs Manager Overhead
// ============================================================================

static void BM_SingleSymbol_L2Update(benchmark::State& state) {
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

BENCHMARK(BM_SingleSymbol_L2Update);

static void BM_Manager_SingleSymbol_L2Update(benchmark::State& state) {
    OrderBookManager<OrderBookL2> manager;
    auto* book = manager.getOrCreateOrderBook(1);

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<Quantity> qty_dist(100, 10000);

    size_t operations = 0;

    for (auto _ : state) {
        Price price = 100000 + (operations % 100) * 10;
        Side side = (operations % 2 == 0) ? Side::Buy : Side::Sell;
        book->updateLevel(side, price, qty_dist(rng), 0, 0);
        ++operations;
    }

    state.SetItemsProcessed(operations);
}

BENCHMARK(BM_Manager_SingleSymbol_L2Update);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
