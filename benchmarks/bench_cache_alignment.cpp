/**
 * @file bench_cache_alignment.cpp
 * @brief Cache alignment verification and optimization analysis
 *
 * Diagnostic tool to:
 * - Verify cache line alignment of critical structures
 * - Measure cache miss rates (via performance counters if available)
 * - Compare aligned vs unaligned access patterns
 * - Identify false sharing opportunities
 * - Benchmark SIMD-ready data layouts
 */

#include <slick/orderbook/orderbook.hpp>
#include <slick/orderbook/detail/price_level_l3.hpp>
#include <slick/orderbook/detail/order.hpp>
#include <benchmark/benchmark.h>
#include <iostream>
#include <type_traits>
#include <cstddef>
#include <random>
#include <algorithm>

using namespace slick::orderbook;
using namespace slick::orderbook::detail;

// ============================================================================
// Cache Line Size Detection
// ============================================================================

constexpr size_t CACHE_LINE_SIZE = 64; // Standard on x86-64

// ============================================================================
// Alignment Verification
// ============================================================================

template<typename T>
void verifyAlignment(const char* type_name) {
    std::cout << "\n=== " << type_name << " ===\n";
    std::cout << "Size: " << sizeof(T) << " bytes\n";
    std::cout << "Alignment: " << alignof(T) << " bytes\n";
    std::cout << "Cache lines: " << (sizeof(T) + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE << "\n";

    if constexpr (alignof(T) >= CACHE_LINE_SIZE) {
        std::cout << "Status: ✓ Cache-aligned\n";
    } else if constexpr (alignof(T) >= 16) {
        std::cout << "Status: ⚠ SIMD-aligned (16 bytes) but not cache-aligned\n";
    } else {
        std::cout << "Status: ✗ Not aligned for optimal performance\n";
    }
}

static void BM_PrintAlignmentInfo(benchmark::State& state) {
    // This benchmark just prints alignment info once
    if (state.thread_index() == 0 && state.iterations() == 0) {
        std::cout << "\n========================================\n";
        std::cout << "Cache Line Size: " << CACHE_LINE_SIZE << " bytes\n";
        std::cout << "========================================\n";

        verifyAlignment<PriceLevelL2>("PriceLevelL2");
        verifyAlignment<PriceLevelL3>("PriceLevelL3");
        verifyAlignment<Order>("Order");
        verifyAlignment<OrderBookL2>("OrderBookL2");
        verifyAlignment<OrderBookL3>("OrderBookL3");
        verifyAlignment<TopOfBook>("TopOfBook");
        verifyAlignment<PriceLevelUpdate>("PriceLevelUpdate");
        verifyAlignment<OrderUpdate>("OrderUpdate");
        verifyAlignment<Trade>("Trade");

        std::cout << "\n========================================\n";
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(CACHE_LINE_SIZE);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_PrintAlignmentInfo)->Iterations(1);

// ============================================================================
// Benchmark: Aligned vs Unaligned Struct Access
// ============================================================================

struct alignas(64) AlignedStruct {
    int64_t a;
    int64_t b;
    int64_t c;
    int64_t d;
};

struct UnalignedStruct {
    int64_t a;
    int64_t b;
    int64_t c;
    int64_t d;
};

static void BM_AlignedStructAccess(benchmark::State& state) {
    const size_t count = 1000;
    std::vector<AlignedStruct> data(count);

    for (auto _ : state) {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i) {
            sum += data[i].a + data[i].b + data[i].c + data[i].d;
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_AlignedStructAccess);

static void BM_UnalignedStructAccess(benchmark::State& state) {
    const size_t count = 1000;
    std::vector<UnalignedStruct> data(count);

    for (auto _ : state) {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i) {
            sum += data[i].a + data[i].b + data[i].c + data[i].d;
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_UnalignedStructAccess);

// ============================================================================
// Benchmark: False Sharing Test
// ============================================================================

struct NoFalseSharing {
    alignas(64) int64_t counter1;
    alignas(64) int64_t counter2;
};

struct WithFalseSharing {
    int64_t counter1;
    int64_t counter2;
};

static void BM_NoFalseSharing_Thread1(benchmark::State& state) {
    static NoFalseSharing data{};

    for (auto _ : state) {
        ++data.counter1;
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_NoFalseSharing_Thread2(benchmark::State& state) {
    static NoFalseSharing data{};

    for (auto _ : state) {
        ++data.counter2;
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_WithFalseSharing_Thread1(benchmark::State& state) {
    static WithFalseSharing data{};

    for (auto _ : state) {
        ++data.counter1;
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_WithFalseSharing_Thread2(benchmark::State& state) {
    static WithFalseSharing data{};

    for (auto _ : state) {
        ++data.counter2;
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_NoFalseSharing_Thread1)->Threads(2);
BENCHMARK(BM_NoFalseSharing_Thread2)->Threads(2);
BENCHMARK(BM_WithFalseSharing_Thread1)->Threads(2);
BENCHMARK(BM_WithFalseSharing_Thread2)->Threads(2);

// ============================================================================
// Benchmark: Array of Structures (AoS) vs Structure of Arrays (SoA)
// ============================================================================

struct PriceQty {
    Price price;
    Quantity quantity;
};

static void BM_ArrayOfStructures(benchmark::State& state) {
    const size_t count = 1000;
    std::vector<PriceQty> data(count);

    for (size_t i = 0; i < count; ++i) {
        data[i] = {100000 + static_cast<Price>(i), 1000 + static_cast<Quantity>(i)};
    }

    for (auto _ : state) {
        // Sum all quantities (common operation in orderbook)
        Quantity total = 0;
        for (size_t i = 0; i < count; ++i) {
            total += data[i].quantity;
        }
        benchmark::DoNotOptimize(total);
    }

    state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_ArrayOfStructures);

static void BM_StructureOfArrays(benchmark::State& state) {
    const size_t count = 1000;
    std::vector<Price> prices(count);
    std::vector<Quantity> quantities(count);

    for (size_t i = 0; i < count; ++i) {
        prices[i] = 100000 + static_cast<Price>(i);
        quantities[i] = 1000 + static_cast<Quantity>(i);
    }

    for (auto _ : state) {
        // Sum all quantities (common operation in orderbook)
        Quantity total = 0;
        for (size_t i = 0; i < count; ++i) {
            total += quantities[i];
        }
        benchmark::DoNotOptimize(total);
    }

    state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_StructureOfArrays);

// ============================================================================
// Benchmark: Sequential vs Random Access (Cache Locality)
// ============================================================================

static void BM_SequentialAccess(benchmark::State& state) {
    const size_t count = 10000;
    std::vector<int64_t> data(count);

    for (size_t i = 0; i < count; ++i) {
        data[i] = static_cast<int64_t>(i);
    }

    for (auto _ : state) {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i) {
            sum += data[i];
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_SequentialAccess);

static void BM_RandomAccess(benchmark::State& state) {
    const size_t count = 10000;
    std::vector<int64_t> data(count);
    std::vector<size_t> indices(count);

    for (size_t i = 0; i < count; ++i) {
        data[i] = static_cast<int64_t>(i);
        indices[i] = i;
    }

    // Shuffle indices
    std::mt19937_64 rng(12345);
    std::shuffle(indices.begin(), indices.end(), rng);

    for (auto _ : state) {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i) {
            sum += data[indices[i]];
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_RandomAccess);

// ============================================================================
// Benchmark: Prefetch Impact
// ============================================================================

static void BM_NoPrefetch(benchmark::State& state) {
    const size_t count = 1000;
    std::vector<int64_t> data(count);
    std::vector<size_t> indices(count);

    for (size_t i = 0; i < count; ++i) {
        data[i] = static_cast<int64_t>(i);
        indices[i] = i;
    }

    std::mt19937_64 rng(54321);
    std::shuffle(indices.begin(), indices.end(), rng);

    for (auto _ : state) {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i) {
            sum += data[indices[i]];
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_NoPrefetch);

static void BM_WithPrefetch(benchmark::State& state) {
    const size_t count = 1000;
    std::vector<int64_t> data(count);
    std::vector<size_t> indices(count);

    for (size_t i = 0; i < count; ++i) {
        data[i] = static_cast<int64_t>(i);
        indices[i] = i;
    }

    std::mt19937_64 rng(54321);
    std::shuffle(indices.begin(), indices.end(), rng);

    for (auto _ : state) {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i) {
            // Prefetch next element
            if (i + 1 < count) {
#ifdef __GNUC__
                __builtin_prefetch(&data[indices[i + 1]], 0, 0);
#endif
            }
            sum += data[indices[i]];
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_WithPrefetch);

// ============================================================================
// Benchmark: OrderBook Operations - Cache Effects
// ============================================================================

static void BM_OrderBook_ColdCache(benchmark::State& state) {
    const size_t num_books = 100;
    std::vector<OrderBookL2> books;
    books.reserve(num_books);

    for (size_t i = 0; i < num_books; ++i) {
        books.emplace_back(static_cast<SymbolId>(i));
        // Add some levels
        for (int j = 0; j < 10; ++j) {
            books[i].updateLevel(Side::Buy, 100000 - j * 10, 1000, 0, 0);
            books[i].updateLevel(Side::Sell, 100100 + j * 10, 1000, 0, 0);
        }
    }

    size_t book_idx = 0;

    for (auto _ : state) {
        // Access different orderbooks each time (cold cache)
        auto tob = books[book_idx % num_books].getTopOfBook();
        benchmark::DoNotOptimize(tob);
        ++book_idx;
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_OrderBook_ColdCache);

static void BM_OrderBook_HotCache(benchmark::State& state) {
    OrderBookL2 book(1);

    // Add some levels
    for (int i = 0; i < 10; ++i) {
        book.updateLevel(Side::Buy, 100000 - i * 10, 1000, 0, 0);
        book.updateLevel(Side::Sell, 100100 + i * 10, 1000, 0, 0);
    }

    for (auto _ : state) {
        // Access same orderbook (hot cache)
        auto tob = book.getTopOfBook();
        benchmark::DoNotOptimize(tob);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_OrderBook_HotCache);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
