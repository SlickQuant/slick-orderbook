/**
 * @file bench_orderbook_l3.cpp
 * @brief Performance benchmarks for OrderBookL3
 *
 * Measures latency of core operations:
 * - addOrModifyOrder (new order insertion)
 * - addOrModifyOrder (order modification)
 * - deleteOrder
 * - executeOrder
 * - L2 aggregation from L3
 * - Order lookup and iteration
 *
 * Target: < 200ns p99 latency for all operations
 */

#include <slick/orderbook/orderbook.hpp>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

using namespace slick::orderbook;

// ============================================================================
// Test Data Generation
// ============================================================================

struct OrderData {
    OrderId order_id;
    Price price;
    Quantity quantity;
    uint64_t priority;
};

std::vector<OrderData> generateRandomOrders(size_t count, Price base_price, Price spread, OrderId start_id) {
    std::vector<OrderData> orders;
    orders.reserve(count);

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    std::uniform_int_distribution<Price> price_offset_dist(0, 10);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    for (size_t i = 0; i < count; ++i) {
        orders.push_back({
            start_id + i,
            base_price + static_cast<Price>(price_offset_dist(rng)) * spread,
            qty_dist(rng),
            priority_dist(rng)
        });
    }

    return orders;
}

// ============================================================================
// Benchmark: Add New Order
// ============================================================================

static void BM_L3_AddNewOrder(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_orders = state.range(0);

    // Pre-generate orders
    auto bid_orders = generateRandomOrders(num_orders * 10, 100000, -10, 1);
    auto ask_orders = generateRandomOrders(num_orders * 10, 100100, 10, num_orders * 10 + 1);

    size_t idx = 0;
    for (auto _ : state) {
        const auto& order = (idx % 2 == 0) ? bid_orders[idx % bid_orders.size()]
                                            : ask_orders[idx % ask_orders.size()];
        const Side side = (idx % 2 == 0) ? Side::Buy : Side::Sell;

        benchmark::DoNotOptimize(
            book.addOrModifyOrder(order.order_id, side, order.price, order.quantity, 0, order.priority, 0)
        );

        ++idx;

        // Prevent book from growing indefinitely
        if (idx % (num_orders * 20) == 0) {
            book = OrderBookL3(1);
            idx = 0;
        }
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L3_AddNewOrder)->Arg(100)->Arg(500)->Arg(1000)->Arg(5000);

// ============================================================================
// Benchmark: Modify Existing Order
// ============================================================================

static void BM_L3_ModifyExistingOrder(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_orders = state.range(0);

    // Build initial book
    auto bid_orders = generateRandomOrders(num_orders, 100000, -10, 1);
    auto ask_orders = generateRandomOrders(num_orders, 100100, 10, num_orders + 1);

    for (const auto& order : bid_orders) {
        book.addOrModifyOrder(order.order_id, Side::Buy, order.price, order.quantity, 0, order.priority, 0);
    }
    for (const auto& order : ask_orders) {
        book.addOrModifyOrder(order.order_id, Side::Sell, order.price, order.quantity, 0, order.priority, 0);
    }

    std::mt19937_64 rng(54321);
    std::uniform_int_distribution<Quantity> qty_dist(50, 1500);
    size_t idx = 0;

    for (auto _ : state) {
        const auto& order = (idx % 2 == 0) ? bid_orders[idx % bid_orders.size()]
                                            : ask_orders[idx % ask_orders.size()];
        const Side side = (idx % 2 == 0) ? Side::Buy : Side::Sell;
        const Quantity new_qty = qty_dist(rng);

        benchmark::DoNotOptimize(
            book.addOrModifyOrder(order.order_id, side, order.price, new_qty, 0, order.priority, 0)
        );

        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L3_ModifyExistingOrder)->Arg(100)->Arg(500)->Arg(1000)->Arg(5000);

// ============================================================================
// Benchmark: Delete Order
// ============================================================================

static void BM_L3_DeleteOrder(benchmark::State& state) {
    const auto num_orders = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();

        // Setup: build book with num_orders on each side
        OrderBookL3 book(1);
        auto bid_orders = generateRandomOrders(num_orders, 100000, -10, 1);
        auto ask_orders = generateRandomOrders(num_orders, 100100, 10, num_orders + 1);

        for (const auto& order : bid_orders) {
            book.addOrModifyOrder(order.order_id, Side::Buy, order.price, order.quantity, 0, order.priority, 0);
        }
        for (const auto& order : ask_orders) {
            book.addOrModifyOrder(order.order_id, Side::Sell, order.price, order.quantity, 0, order.priority, 0);
        }

        state.ResumeTiming();

        // Delete all orders
        for (const auto& order : bid_orders) {
            benchmark::DoNotOptimize(
                book.deleteOrder(order.order_id, 0)
            );
        }
        for (const auto& order : ask_orders) {
            benchmark::DoNotOptimize(
                book.deleteOrder(order.order_id, 0)
            );
        }
    }

    state.SetItemsProcessed(state.iterations() * num_orders * 2);
}

BENCHMARK(BM_L3_DeleteOrder)->Arg(100)->Arg(500)->Arg(1000);

// ============================================================================
// Benchmark: Execute Order (Partial Fill)
// ============================================================================

static void BM_L3_ExecuteOrderPartial(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_orders = state.range(0);

    // Build initial book
    auto bid_orders = generateRandomOrders(num_orders, 100000, -10, 1);
    auto ask_orders = generateRandomOrders(num_orders, 100100, 10, num_orders + 1);

    for (const auto& order : bid_orders) {
        book.addOrModifyOrder(order.order_id, Side::Buy, order.price, order.quantity, 0, order.priority, 0);
    }
    for (const auto& order : ask_orders) {
        book.addOrModifyOrder(order.order_id, Side::Sell, order.price, order.quantity, 0, order.priority, 0);
    }

    size_t idx = 0;

    for (auto _ : state) {
        const auto& order = (idx % 2 == 0) ? bid_orders[idx % bid_orders.size()]
                                            : ask_orders[idx % ask_orders.size()];

        // Execute half the quantity
        Quantity exec_qty = order.quantity / 2;

        benchmark::DoNotOptimize(
            book.executeOrder(order.order_id, exec_qty, 0)
        );

        ++idx;

        // Reset book periodically to maintain order quantities
        if (idx % (num_orders * 4) == 0) {
            book = OrderBookL3(1);
            for (const auto& o : bid_orders) {
                book.addOrModifyOrder(o.order_id, Side::Buy, o.price, o.quantity, 0, o.priority, 0);
            }
            for (const auto& o : ask_orders) {
                book.addOrModifyOrder(o.order_id, Side::Sell, o.price, o.quantity, 0, o.priority, 0);
            }
        }
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L3_ExecuteOrderPartial)->Arg(100)->Arg(500)->Arg(1000);

// ============================================================================
// Benchmark: Execute Order (Complete Fill)
// ============================================================================

static void BM_L3_ExecuteOrderComplete(benchmark::State& state) {
    const auto num_orders = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();

        // Setup: build book with num_orders on each side
        OrderBookL3 book(1);
        auto bid_orders = generateRandomOrders(num_orders, 100000, -10, 1);
        auto ask_orders = generateRandomOrders(num_orders, 100100, 10, num_orders + 1);

        for (const auto& order : bid_orders) {
            book.addOrModifyOrder(order.order_id, Side::Buy, order.price, order.quantity, 0, order.priority, 0);
        }
        for (const auto& order : ask_orders) {
            book.addOrModifyOrder(order.order_id, Side::Sell, order.price, order.quantity, 0, order.priority, 0);
        }

        state.ResumeTiming();

        // Execute complete fills on all orders
        for (const auto& order : bid_orders) {
            benchmark::DoNotOptimize(
                book.executeOrder(order.order_id, order.quantity, 0)
            );
        }
        for (const auto& order : ask_orders) {
            benchmark::DoNotOptimize(
                book.executeOrder(order.order_id, order.quantity, 0)
            );
        }
    }

    state.SetItemsProcessed(state.iterations() * num_orders * 2);
}

BENCHMARK(BM_L3_ExecuteOrderComplete)->Arg(100)->Arg(500)->Arg(1000);

// ============================================================================
// Benchmark: L2 Aggregation from L3
// ============================================================================

static void BM_L3_GetLevelsL2(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_orders = state.range(0);

    // Build book with orders at multiple price levels
    auto bid_orders = generateRandomOrders(num_orders, 100000, -10, 1);
    auto ask_orders = generateRandomOrders(num_orders, 100100, 10, num_orders + 1);

    for (const auto& order : bid_orders) {
        book.addOrModifyOrder(order.order_id, Side::Buy, order.price, order.quantity, 0, order.priority, 0);
    }
    for (const auto& order : ask_orders) {
        book.addOrModifyOrder(order.order_id, Side::Sell, order.price, order.quantity, 0, order.priority, 0);
    }

    for (auto _ : state) {
        auto l2_bids = book.getLevelsL2(Side::Buy);
        auto l2_asks = book.getLevelsL2(Side::Sell);

        benchmark::DoNotOptimize(l2_bids);
        benchmark::DoNotOptimize(l2_asks);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L3_GetLevelsL2)->Arg(100)->Arg(500)->Arg(1000)->Arg(5000);

// ============================================================================
// Benchmark: L3 Order Iteration
// ============================================================================

static void BM_L3_GetLevelsL3(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_orders = state.range(0);

    // Build book
    auto bid_orders = generateRandomOrders(num_orders, 100000, -10, 1);
    auto ask_orders = generateRandomOrders(num_orders, 100100, 10, num_orders + 1);

    for (const auto& order : bid_orders) {
        book.addOrModifyOrder(order.order_id, Side::Buy, order.price, order.quantity, 0, order.priority, 0);
    }
    for (const auto& order : ask_orders) {
        book.addOrModifyOrder(order.order_id, Side::Sell, order.price, order.quantity, 0, order.priority, 0);
    }

    for (auto _ : state) {
        const auto& bid_levels = book.getLevelsL3(Side::Buy);
        const auto& ask_levels = book.getLevelsL3(Side::Sell);

        // Iterate through all orders
        size_t total_orders = 0;
        Quantity total_bid_qty = 0;
        Quantity total_ask_qty = 0;

        for (const auto& [price, level] : bid_levels) {
            for (const auto& order : level.orders) {
                total_bid_qty += order.quantity;
                ++total_orders;
            }
        }

        for (const auto& [price, level] : ask_levels) {
            for (const auto& order : level.orders) {
                total_ask_qty += order.quantity;
                ++total_orders;
            }
        }

        benchmark::DoNotOptimize(total_orders);
        benchmark::DoNotOptimize(total_bid_qty);
        benchmark::DoNotOptimize(total_ask_qty);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_L3_GetLevelsL3)->Arg(100)->Arg(500)->Arg(1000)->Arg(5000);

// ============================================================================
// Benchmark: Mixed Workload (Realistic)
// ============================================================================

static void BM_L3_MixedWorkload(benchmark::State& state) {
    OrderBookL3 book(1);
    const auto num_orders = state.range(0);

    // Build initial book
    auto bid_orders = generateRandomOrders(num_orders, 100000, -10, 1);
    auto ask_orders = generateRandomOrders(num_orders, 100100, 10, num_orders + 1);

    for (const auto& order : bid_orders) {
        book.addOrModifyOrder(order.order_id, Side::Buy, order.price, order.quantity, 0, order.priority, 0);
    }
    for (const auto& order : ask_orders) {
        book.addOrModifyOrder(order.order_id, Side::Sell, order.price, order.quantity, 0, order.priority, 0);
    }

    std::mt19937_64 rng(99999);
    std::uniform_int_distribution<int> action_dist(0, 9);
    std::uniform_int_distribution<Quantity> qty_dist(50, 1500);
    std::uniform_int_distribution<size_t> order_dist(0, num_orders - 1);
    std::uniform_int_distribution<uint64_t> priority_dist(0, 1000000);

    OrderId next_order_id = num_orders * 2 + 1;
    size_t operations = 0;

    for (auto _ : state) {
        int action = action_dist(rng);
        Side side = (action % 2 == 0) ? Side::Buy : Side::Sell;
        const auto& orders = (side == Side::Buy) ? bid_orders : ask_orders;

        if (action < 3) {
            // 30% modify existing order
            const auto& order = orders[order_dist(rng)];
            book.addOrModifyOrder(order.order_id, side, order.price, qty_dist(rng), 0, order.priority, 0);
        } else if (action < 5) {
            // 20% add new order
            Price new_price = (side == Side::Buy) ? 99900 : 100200;
            book.addOrModifyOrder(next_order_id++, side, new_price, qty_dist(rng), 0, priority_dist(rng), 0);
        } else if (action < 7) {
            // 20% delete order
            if (!orders.empty()) {
                const auto& order = orders[order_dist(rng)];
                book.deleteOrder(order.order_id, 0);
            }
        } else if (action < 9) {
            // 20% execute order (partial)
            if (!orders.empty()) {
                const auto& order = orders[order_dist(rng)];
                Quantity exec_qty = std::min<Quantity>(order.quantity / 2, 100);
                book.executeOrder(order.order_id, exec_qty, 0);
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

BENCHMARK(BM_L3_MixedWorkload)->Arg(100)->Arg(500)->Arg(1000);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
