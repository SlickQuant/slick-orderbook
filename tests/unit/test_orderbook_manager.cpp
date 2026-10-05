// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_manager.hpp>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>
#include <chrono>
#include <memory>
#include <type_traits>
#include <utility>
#include <slick/orderbook/config.hpp>


using namespace slick::orderbook;

// Test fixture for OrderBookManager with L2
class OrderBookManagerL2Test : public ::testing::Test {
protected:
    static constexpr SymbolId kSymbol1 = 1;
    static constexpr SymbolId kSymbol2 = 2;
    static constexpr SymbolId kSymbol3 = 3;
    static constexpr Price kPrice100 = 100;
    static constexpr Quantity kQty10 = 10;
    static constexpr Timestamp kTs1 = 1000;
};

// Test fixture for OrderBookManager with L3
class OrderBookManagerL3Test : public ::testing::Test {
protected:
    static constexpr SymbolId kSymbol1 = 1;
    static constexpr SymbolId kSymbol2 = 2;
    static constexpr SymbolId kSymbol3 = 3;
    static constexpr OrderId kOrder1 = 100;
    static constexpr OrderId kOrder2 = 101;
    static constexpr Price kPrice100 = 100;
    static constexpr Price kPrice101 = 101;
    static constexpr Quantity kQty10 = 10;
    static constexpr Quantity kQty20 = 20;
    static constexpr Timestamp kTs1 = 1000;
    static constexpr Timestamp kTs2 = 2000;
};

// ============================================================================
// L2 Manager Tests
// ============================================================================

TEST_F(OrderBookManagerL2Test, InitialState) {
    OrderBookManager<OrderBookL2> manager;
    EXPECT_EQ(manager.symbolCount(), 0);
    EXPECT_FALSE(manager.hasSymbol(kSymbol1));
    EXPECT_EQ(manager.getOrderBook(kSymbol1), nullptr);
}

TEST_F(OrderBookManagerL2Test, GetOrCreateOrderBook) {
    OrderBookManager<OrderBookL2> manager;

    // Create first orderbook
    auto* book1 = manager.getOrCreateOrderBook(kSymbol1);
    ASSERT_NE(book1, nullptr);
    EXPECT_EQ(book1->symbol(), kSymbol1);
    EXPECT_EQ(manager.symbolCount(), 1);
    EXPECT_TRUE(manager.hasSymbol(kSymbol1));

    // Get same orderbook again (should return same pointer)
    auto* book1_again = manager.getOrCreateOrderBook(kSymbol1);
    EXPECT_EQ(book1, book1_again);
    EXPECT_EQ(manager.symbolCount(), 1);

    // Create second orderbook
    auto* book2 = manager.getOrCreateOrderBook(kSymbol2);
    ASSERT_NE(book2, nullptr);
    EXPECT_EQ(book2->symbol(), kSymbol2);
    EXPECT_NE(book1, book2);
    EXPECT_EQ(manager.symbolCount(), 2);
}

TEST_F(OrderBookManagerL2Test, GetOrderBook) {
    OrderBookManager<OrderBookL2> manager;

    // Initially null
    EXPECT_EQ(manager.getOrderBook(kSymbol1), nullptr);

    // Create orderbook
    auto* created = manager.getOrCreateOrderBook(kSymbol1);
    ASSERT_NE(created, nullptr);

    // Get existing (const version)
    const auto& const_manager = manager;
    const auto* book = const_manager.getOrderBook(kSymbol1);
    EXPECT_EQ(book, created);

    // Get existing (mutable version)
    auto* mutable_book = manager.getOrderBook(kSymbol1);
    EXPECT_EQ(mutable_book, created);

    // Non-existent symbol still returns nullptr
    EXPECT_EQ(manager.getOrderBook(kSymbol2), nullptr);
}

TEST_F(OrderBookManagerL2Test, HasSymbol) {
    OrderBookManager<OrderBookL2> manager;

    EXPECT_FALSE(manager.hasSymbol(kSymbol1));
    EXPECT_FALSE(manager.hasSymbol(kSymbol2));

    [[maybe_unused]] auto* book1 = manager.getOrCreateOrderBook(kSymbol1);
    EXPECT_TRUE(manager.hasSymbol(kSymbol1));
    EXPECT_FALSE(manager.hasSymbol(kSymbol2));

    [[maybe_unused]] auto* book2 = manager.getOrCreateOrderBook(kSymbol2);
    EXPECT_TRUE(manager.hasSymbol(kSymbol1));
    EXPECT_TRUE(manager.hasSymbol(kSymbol2));
}

TEST_F(OrderBookManagerL2Test, RemoveOrderBook) {
    OrderBookManager<OrderBookL2> manager;

    // Remove non-existent symbol
    EXPECT_FALSE(manager.removeOrderBook(kSymbol1));
    EXPECT_EQ(manager.symbolCount(), 0);

    // Create and remove
    [[maybe_unused]] auto* book1 = manager.getOrCreateOrderBook(kSymbol1);
    [[maybe_unused]] auto* book2 = manager.getOrCreateOrderBook(kSymbol2);
    EXPECT_EQ(manager.symbolCount(), 2);

    EXPECT_TRUE(manager.removeOrderBook(kSymbol1));
    EXPECT_EQ(manager.symbolCount(), 1);
    EXPECT_FALSE(manager.hasSymbol(kSymbol1));
    EXPECT_TRUE(manager.hasSymbol(kSymbol2));

    // Remove already removed symbol
    EXPECT_FALSE(manager.removeOrderBook(kSymbol1));
    EXPECT_EQ(manager.symbolCount(), 1);

    // Remove remaining symbol
    EXPECT_TRUE(manager.removeOrderBook(kSymbol2));
    EXPECT_EQ(manager.symbolCount(), 0);
}

TEST_F(OrderBookManagerL2Test, GetSymbols) {
    OrderBookManager<OrderBookL2> manager;

    // Empty initially
    auto symbols = manager.getSymbols();
    EXPECT_TRUE(symbols.empty());

    // Add symbols
    [[maybe_unused]] auto* book3 = manager.getOrCreateOrderBook(kSymbol3);
    [[maybe_unused]] auto* book1 = manager.getOrCreateOrderBook(kSymbol1);
    [[maybe_unused]] auto* book2 = manager.getOrCreateOrderBook(kSymbol2);

    symbols = manager.getSymbols();
    EXPECT_EQ(symbols.size(), 3);

    // Verify all symbols present (order may vary)
    std::sort(symbols.begin(), symbols.end());
    EXPECT_EQ(symbols[0], kSymbol1);
    EXPECT_EQ(symbols[1], kSymbol2);
    EXPECT_EQ(symbols[2], kSymbol3);
}

TEST_F(OrderBookManagerL2Test, Clear) {
    OrderBookManager<OrderBookL2> manager;

    [[maybe_unused]] auto* book1 = manager.getOrCreateOrderBook(kSymbol1);
    [[maybe_unused]] auto* book2 = manager.getOrCreateOrderBook(kSymbol2);
    [[maybe_unused]] auto* book3 = manager.getOrCreateOrderBook(kSymbol3);
    EXPECT_EQ(manager.symbolCount(), 3);

    manager.clear();
    EXPECT_EQ(manager.symbolCount(), 0);
    EXPECT_FALSE(manager.hasSymbol(kSymbol1));
    EXPECT_FALSE(manager.hasSymbol(kSymbol2));
    EXPECT_FALSE(manager.hasSymbol(kSymbol3));
}

TEST_F(OrderBookManagerL2Test, Reserve) {
    OrderBookManager<OrderBookL2> manager;

    // Should not throw or crash (may be no-op for std::map)
    manager.reserve(100);
    EXPECT_EQ(manager.symbolCount(), 0);

    // Can still add symbols after reserve
    [[maybe_unused]] auto* book1 = manager.getOrCreateOrderBook(kSymbol1);
    EXPECT_EQ(manager.symbolCount(), 1);
}

// Regression: defaulted move operations were implicitly deleted by the std::shared_mutex member
static_assert(std::is_nothrow_move_constructible_v<OrderBookManager<OrderBookL2>>);
static_assert(std::is_nothrow_move_assignable_v<OrderBookManager<OrderBookL2>>);
static_assert(std::is_nothrow_move_constructible_v<OrderBookManager<OrderBookL3>>);
static_assert(std::is_nothrow_move_assignable_v<OrderBookManager<OrderBookL3>>);

TEST_F(OrderBookManagerL2Test, MoveConstruct) {
    OrderBookManager<OrderBookL2> source;
    auto* book1 = source.getOrCreateOrderBook(kSymbol1);
    book1->updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    auto shared2 = source.getOrCreateSharedOrderBook(kSymbol2);

    OrderBookManager<OrderBookL2> moved(std::move(source));

    EXPECT_EQ(moved.symbolCount(), 2);
    EXPECT_EQ(moved.getOrderBook(kSymbol1), book1);  // Orderbooks are transferred, not copied
    EXPECT_EQ(moved.getSharedOrderBook(kSymbol2), shared2);
    ASSERT_NE(moved.getOrderBook(kSymbol1)->getBestBid(), nullptr);
    EXPECT_EQ(moved.getOrderBook(kSymbol1)->getBestBid()->quantity, kQty10);

    // Moved-from manager is empty but still usable
    EXPECT_EQ(source.symbolCount(), 0);
    EXPECT_FALSE(source.hasSymbol(kSymbol1));
    EXPECT_NE(source.getOrCreateOrderBook(kSymbol3), nullptr);
    EXPECT_EQ(source.symbolCount(), 1);
}

TEST_F(OrderBookManagerL3Test, MoveAssign) {
    OrderBookManager<OrderBookL3> source;
    auto* book1 = source.getOrCreateOrderBook(kSymbol1);
    ASSERT_TRUE(book1->addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));

    OrderBookManager<OrderBookL3> target;
    auto old_book = target.getOrCreateSharedOrderBook(kSymbol2);

    target = std::move(source);

    EXPECT_EQ(target.symbolCount(), 1);
    EXPECT_EQ(target.getOrderBook(kSymbol1), book1);
    EXPECT_FALSE(target.hasSymbol(kSymbol2));  // Previous contents replaced
    EXPECT_EQ(old_book.use_count(), 1);        // ...and released by the manager
    EXPECT_NE(target.getOrderBook(kSymbol1)->findOrder(kOrder1), nullptr);
    EXPECT_EQ(source.symbolCount(), 0);

    // Self-move leaves the manager intact
    auto& self = target;
    target = std::move(self);
    EXPECT_EQ(target.symbolCount(), 1);
}

// Regression: move assignment destroys the destination's previous orderbooks, so raw pointers
// obtained from the destination dangle; only the source's orderbooks survive the move.
// Lifetimes are observed through weak_ptr, never by dereferencing a possibly dangling pointer.
TEST_F(OrderBookManagerL2Test, MoveAssignLifetime) {
    OrderBookManager<OrderBookL2> source;
    std::weak_ptr<OrderBookL2> source_book = source.getOrCreateSharedOrderBook(kSymbol1);
    const OrderBookL2* source_raw = source.getOrderBook(kSymbol1);

    OrderBookManager<OrderBookL2> target;
    std::weak_ptr<OrderBookL2> target_book = target.getOrCreateSharedOrderBook(kSymbol2);  // Owned by target only
    auto retained = target.getOrCreateSharedOrderBook(kSymbol3);                           // Also held by caller
    ASSERT_FALSE(target_book.expired());

    target = std::move(source);

    EXPECT_TRUE(target_book.expired());             // Destination's raw pointers to it now dangle
    EXPECT_EQ(retained.use_count(), 1);             // A retained handle keeps its orderbook alive
    ASSERT_FALSE(source_book.expired());            // Source's orderbook is transferred, not destroyed
    EXPECT_EQ(target.getOrderBook(kSymbol1), source_raw);
    EXPECT_EQ(target.getSharedOrderBook(kSymbol1), source_book.lock());
}

TEST_F(OrderBookManagerL2Test, OrderBookOperations) {
    OrderBookManager<OrderBookL2> manager;

    // Create orderbook and perform operations
    auto* book = manager.getOrCreateOrderBook(kSymbol1);
    ASSERT_NE(book, nullptr);

    // Verify orderbook works
    book->updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    auto best_bid = book->getBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price, kPrice100);
    EXPECT_EQ(best_bid->quantity, kQty10);

    // Get orderbook again and verify state persists
    auto* same_book = manager.getOrderBook(kSymbol1);
    EXPECT_EQ(same_book, book);
    best_bid = same_book->getBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price, kPrice100);
}

// ============================================================================
// L3 Manager Tests
// ============================================================================

TEST_F(OrderBookManagerL3Test, InitialState) {
    OrderBookManager<OrderBookL3> manager;
    EXPECT_EQ(manager.symbolCount(), 0);
    EXPECT_FALSE(manager.hasSymbol(kSymbol1));
    EXPECT_EQ(manager.getOrderBook(kSymbol1), nullptr);
}

TEST_F(OrderBookManagerL3Test, GetOrCreateOrderBook) {
    OrderBookManager<OrderBookL3> manager;

    auto* book1 = manager.getOrCreateOrderBook(kSymbol1);
    ASSERT_NE(book1, nullptr);
    EXPECT_EQ(book1->symbol(), kSymbol1);
    EXPECT_EQ(manager.symbolCount(), 1);

    auto* book1_again = manager.getOrCreateOrderBook(kSymbol1);
    EXPECT_EQ(book1, book1_again);

    auto* book2 = manager.getOrCreateOrderBook(kSymbol2);
    ASSERT_NE(book2, nullptr);
    EXPECT_NE(book1, book2);
    EXPECT_EQ(manager.symbolCount(), 2);
}

TEST_F(OrderBookManagerL3Test, OrderBookOperations) {
    OrderBookManager<OrderBookL3> manager;

    // Create orderbook and perform L3 operations
    auto* book = manager.getOrCreateOrderBook(kSymbol1);
    ASSERT_NE(book, nullptr);

    // Add orders
    EXPECT_TRUE(book->addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_TRUE(book->addOrder(kOrder2, Side::Buy, kPrice101, kQty20, kTs2));
    EXPECT_EQ(book->orderCount(), 2);

    // Verify top of book
    auto tob = book->getTopOfBook();
    EXPECT_EQ(tob.best_bid, kPrice101);
    EXPECT_EQ(tob.bid_quantity, kQty20);

    // Get orderbook again and verify state persists
    auto* same_book = manager.getOrderBook(kSymbol1);
    EXPECT_EQ(same_book, book);
    EXPECT_EQ(same_book->orderCount(), 2);
}

TEST_F(OrderBookManagerL3Test, MultipleSymbolsIndependence) {
    OrderBookManager<OrderBookL3> manager;

    auto* book1 = manager.getOrCreateOrderBook(kSymbol1);
    auto* book2 = manager.getOrCreateOrderBook(kSymbol2);

    // Add order to book1
    EXPECT_TRUE(book1->addOrder(kOrder1, Side::Buy, kPrice100, kQty10, kTs1));
    EXPECT_EQ(book1->orderCount(), 1);
    EXPECT_EQ(book2->orderCount(), 0);

    // Add order to book2 (can use same OrderId - different symbol)
    EXPECT_TRUE(book2->addOrder(kOrder1, Side::Sell, kPrice101, kQty20, kTs2));
    EXPECT_EQ(book1->orderCount(), 1);
    EXPECT_EQ(book2->orderCount(), 1);

    // Verify orders are independent
    auto* order_book1 = book1->findOrder(kOrder1);
    auto* order_book2 = book2->findOrder(kOrder1);
    ASSERT_NE(order_book1, nullptr);
    ASSERT_NE(order_book2, nullptr);
    EXPECT_EQ(order_book1->price, kPrice100);
    EXPECT_EQ(order_book2->price, kPrice101);
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

TEST_F(OrderBookManagerL2Test, ConcurrentGetOrCreate) {
    OrderBookManager<OrderBookL2> manager;
    constexpr int kNumThreads = 8;
    constexpr int kNumSymbols = 100;

    // All threads try to create the same symbols concurrently
    std::vector<std::thread> threads;
    std::vector<std::vector<OrderBookL2*>> thread_results(kNumThreads);

    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&manager, &thread_results, t]() {
            for (SymbolId symbol = 1; symbol <= kNumSymbols; ++symbol) {
                auto* book = manager.getOrCreateOrderBook(symbol);
                thread_results[t].push_back(book);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify: exactly kNumSymbols orderbooks created
    EXPECT_EQ(manager.symbolCount(), kNumSymbols);

    // Verify: all threads got the same pointers for each symbol
    for (SymbolId symbol = 1; symbol <= kNumSymbols; ++symbol) {
        auto* expected = thread_results[0][symbol - 1];
        for (int t = 1; t < kNumThreads; ++t) {
            EXPECT_EQ(thread_results[t][symbol - 1], expected);
        }
    }
}

TEST_F(OrderBookManagerL3Test, ConcurrentDifferentSymbols) {
    OrderBookManager<OrderBookL3> manager;
    constexpr int16_t kNumThreads = 4;
    constexpr int16_t kSymbolsPerThread = 25;
    constexpr int kOrdersPerSymbol = 100;

    // Each thread creates its own symbols and adds orders
    std::vector<std::thread> threads;

    for (int16_t t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&manager, t]() {
            SymbolId base_symbol = t * kSymbolsPerThread + 1;
            for (int16_t s = 0; s < kSymbolsPerThread; ++s) {
                SymbolId symbol = base_symbol + s;
                auto* book = manager.getOrCreateOrderBook(symbol);

                // Add orders to this symbol
                for (int o = 0; o < kOrdersPerSymbol; ++o) {
                    OrderId order_id = o + 1;
                    Price price = 10000 + o;
                    book->addOrder(order_id, Side::Buy, price, 100, o);
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify: all symbols created
    EXPECT_EQ(manager.symbolCount(), kNumThreads * kSymbolsPerThread);

    // Verify: each orderbook has correct number of orders
    for (int16_t t = 0; t < kNumThreads; ++t) {
        SymbolId base_symbol = t * kSymbolsPerThread + 1;
        for (int16_t s = 0; s < kSymbolsPerThread; ++s) {
            SymbolId symbol = base_symbol + s;
            auto* book = manager.getOrderBook(symbol);
            ASSERT_NE(book, nullptr);
            EXPECT_EQ(book->orderCount(), kOrdersPerSymbol);
        }
    }
}

// Test concurrent read/write across multiple symbols
// This validates thread safety of the OrderBookManager's symbol map access
// and the lock-free SWMR design of individual orderbooks.
// Each symbol has ONE writer thread but MULTIPLE reader threads accessing concurrently.
TEST_F(OrderBookManagerL2Test, ConcurrentReadWrite) {
#if SLICK_TSAN_ENABLED
    GTEST_SKIP() << "Disabled under TSAN (lock-free read path is not TSAN-friendly).";
#endif
    OrderBookManager<OrderBookL2> manager;
    constexpr int kNumReaders = 8;
    constexpr int kNumSymbols = 10;
    constexpr auto kDuration = std::chrono::milliseconds(100);

    // Pre-create symbols
    for (SymbolId s = 1; s <= kNumSymbols; ++s) {
        [[maybe_unused]] auto* book = manager.getOrCreateOrderBook(s);
    }

    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;

    // Writer threads: each writer owns a subset of symbols (single-writer per symbol)
    // Writer 1 handles odd symbols, Writer 2 handles even symbols
    threads.emplace_back([&manager, &stop]() {
        int counter = 0;
        while (!stop.load()) {
            for (SymbolId s = 1; s <= kNumSymbols; s += 2) {  // Odd symbols
                auto* book = manager.getOrderBook(s);
                if (book) {
                    Price price = 10000 + (counter % 100);
                    book->updateLevel(Side::Buy, price, 100, counter);
                    ++counter;
                }
            }
        }
    });

    threads.emplace_back([&manager, &stop]() {
        int counter = 0;
        while (!stop.load()) {
            for (SymbolId s = 2; s <= kNumSymbols; s += 2) {  // Even symbols
                auto* book = manager.getOrderBook(s);
                if (book) {
                    Price price = 10000 + (counter % 100);
                    book->updateLevel(Side::Buy, price, 100, counter);
                    ++counter;
                }
            }
        }
    });

    // Reader threads: continuously query orderbooks (all symbols)
    // These run concurrently with writers to test lock-free read access
    for (int r = 0; r < kNumReaders; ++r) {
        threads.emplace_back([&manager, &stop]() {
            while (!stop.load()) {
                for (SymbolId s = 1; s <= kNumSymbols; ++s) {
                    auto* book = manager.getOrderBook(s);
                    if (book) {
                        // Thread-safe: getBestBid/getBestAsk/getTopOfBook use seqlocks
                        [[maybe_unused]] auto* best_bid = book->getBestBid();
                        [[maybe_unused]] auto* best_ask = book->getBestAsk();
                        [[maybe_unused]] auto tob = book->getTopOfBook();
                    }
                }
            }
        });
    }

    // Run for duration
    std::this_thread::sleep_for(kDuration);
    stop.store(true);

    for (auto& thread : threads) {
        thread.join();
    }

    // If we got here without crashes or data races, the test passed
    EXPECT_EQ(manager.symbolCount(), kNumSymbols);
}

TEST_F(OrderBookManagerL2Test, ConcurrentRemoveAndAccess) {
    OrderBookManager<OrderBookL2> manager;
    constexpr int kNumSymbols = 50;

    // Create symbols
    for (SymbolId s = 1; s <= kNumSymbols; ++s) {
        [[maybe_unused]] auto* book = manager.getOrCreateOrderBook(s);
    }

    EXPECT_EQ(manager.symbolCount(), kNumSymbols);

    // One thread removes symbols, another tries to access
    std::atomic<bool> stop{false};
    std::thread remover([&manager, &stop]() {
        for (SymbolId s = 1; s <= kNumSymbols; s += 2) {  // Remove odd symbols
            manager.removeOrderBook(s);
        }
        stop.store(true);
    });

    std::thread accessor([&manager, &stop]() {
        while (!stop.load()) {
            for (SymbolId s = 1; s <= kNumSymbols; ++s) {
                [[maybe_unused]] auto* book = manager.getOrderBook(s);
                // Book may be null if removed
            }
        }
    });

    remover.join();
    accessor.join();

    // Half the symbols should remain
    EXPECT_EQ(manager.symbolCount(), kNumSymbols / 2);
}

// Regression: shared handles keep the orderbook alive after concurrent removal
TEST_F(OrderBookManagerL2Test, SharedOrderBookOutlivesRemoval) {
    OrderBookManager<OrderBookL2> manager;

    auto book = manager.getOrCreateSharedOrderBook(kSymbol1);
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book.get(), manager.getOrderBook(kSymbol1));
    EXPECT_EQ(book, manager.getSharedOrderBook(kSymbol1));
    EXPECT_EQ(manager.getSharedOrderBook(kSymbol2), nullptr);

    EXPECT_TRUE(manager.removeOrderBook(kSymbol1));
    EXPECT_EQ(manager.getSharedOrderBook(kSymbol1), nullptr);

    // Handle still valid and usable after removal
    book->updateLevel(Side::Buy, kPrice100, kQty10, kTs1);
    EXPECT_EQ(book->levelCount(Side::Buy), 1);

    auto other = manager.getOrCreateSharedOrderBook(kSymbol2);
    manager.clear();
    EXPECT_EQ(other->symbol(), kSymbol2);
}

TEST_F(OrderBookManagerL3Test, ConcurrentRemoveWithSharedHandles) {
    OrderBookManager<OrderBookL3> manager;
    constexpr SymbolId kNumSymbols = 50;
    constexpr int kRounds = 200;

    std::atomic<bool> stop{false};
    std::thread remover([&manager, &stop]() {
        for (int round = 0; round < kRounds; ++round) {
            for (SymbolId s = 1; s <= kNumSymbols; ++s) {
                manager.removeOrderBook(s);
            }
        }
        stop.store(true);
    });

    std::thread writer([&manager, &stop]() {
        OrderId next_id = 1;
        while (!stop.load()) {
            for (SymbolId s = 1; s <= kNumSymbols; ++s) {
                auto book = manager.getOrCreateSharedOrderBook(s);
                // Book may be removed from the manager concurrently; the handle keeps it alive
                EXPECT_TRUE(book->addOrder(next_id++, Side::Buy, 100, 10, 1000));
            }
        }
    });

    remover.join();
    writer.join();
}
