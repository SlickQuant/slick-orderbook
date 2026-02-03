# Slick OrderBook - Examples

This directory contains practical examples demonstrating how to use the Slick OrderBook library in real-world scenarios.

## Table of Contents

1. [Building Examples](#building-examples)
2. [Example Descriptions](#example-descriptions)
3. [Running Examples](#running-examples)
4. [Common Patterns](#common-patterns)

---

## Building Examples

Examples are built automatically when you enable the `SLICK_ORDERBOOK_BUILD_EXAMPLES` option:

```bash
# Configure with examples enabled (default)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSLICK_ORDERBOOK_BUILD_EXAMPLES=ON
cmake --build build -j

# Run an example
./build/examples/simple_l2_orderbook
```

---

## Example Descriptions

### 1. simple_l2_orderbook.cpp

**Purpose**: Introduction to Level 2 orderbook basics

**What it demonstrates**:
- Creating an `OrderBookL2` instance
- Adding and modifying price levels
- Deleting levels (using quantity=0)
- Querying best bid/ask
- Getting top-of-book snapshot
- Iterating over all price levels

**Key Concepts**:
- L2 orderbook only tracks aggregated quantities at each price
- `updateLevel()` is used for both add and modify operations
- Setting quantity to 0 deletes the level
- `getTopOfBook()` provides O(1) best bid/ask access

**Output Example**:
```
=== Simple L2 OrderBook Example ===

Adding price levels...
Querying best bid/ask...
Best Bid: 9990 x 200
Best Ask: 10010 x 150
Spread: 20
Mid Price: 10000

All Bid Levels (5 levels):
  Price: 9990, Quantity: 200
  Price: 9980, Quantity: 250
  ...
```

**Use Case**: Perfect for applications that only need price/quantity aggregation without order-level detail (e.g., market data displays, trading signals).

---

### 2. simple_l3_orderbook.cpp

**Purpose**: Introduction to Level 3 orderbook with individual order tracking

**What it demonstrates**:
- Creating an `OrderBookL3` instance
- Adding orders with unique OrderIds
- Modifying existing orders
- Executing partial orders
- Deleting orders by OrderId
- Accessing L3 order-by-order data
- Aggregating L3 data to L2 view

**Key Concepts**:
- L3 tracks every individual order with unique IDs
- Orders have priority for time/price sorting
- `addOrModifyOrder()` handles both add and modify cases
- `getLevelsL3()` provides zero-copy access to order queues
- `getLevelsL2()` aggregates L3 to L2 on-the-fly

**Output Example**:
```
=== Simple L3 OrderBook Example ===

Adding orders...
Order 1001 added: Buy 100 @ 10000
Order 1002 added: Buy 50 @ 10000 (lower priority)
...

Executing partial order...
Remaining quantity for order 2001: 50

L3 Order Details (Bid side):
  Level @ 10000 (2 orders):
    Order 1001: qty=150, priority=0
    Order 1002: qty=50, priority=1
```

**Use Case**: Essential for matching engines, full orderbook visualization, order routing systems, and any application needing order-level visibility.

---

### 3. multi_symbol_orderbook.cpp

**Purpose**: Managing multiple symbols efficiently with `OrderBookManager`

**What it demonstrates**:
- Creating an `OrderBookManager` for multiple symbols
- Getting or creating orderbooks on-demand
- Thread-safe symbol management
- Iterating over all managed orderbooks
- Querying orderbook statistics
- Removing orderbooks
- Managing observer subscriptions per symbol

**Key Concepts**:
- `OrderBookManager` provides thread-safe symbol registry
- `getOrCreateOrderBook()` uses double-checked locking pattern
- Per-symbol isolation (no cross-symbol locking)
- Template-based design supports both L2 and L3
- Efficient memory management with unique_ptr

**Output Example**:
```
=== Multi-Symbol OrderBook Manager Example ===

Managing 3 symbols...

Symbol 1 (BTC): Best Bid=45000.00 / Best Ask=45005.00
Symbol 2 (ETH): Best Bid=2500.00 / Best Ask=2502.00
Symbol 3 (SOL): Best Bid=100.00 / Best Ask=100.10

Statistics:
  Total symbols: 3
  Total orderbooks: 3
  Active symbols: 3
```

**Use Case**: Multi-asset trading platforms, portfolio managers, market data aggregators handling hundreds or thousands of symbols.

---

### 4. coinbase_integration.cpp

**Purpose**: Real-world integration with Coinbase Advanced Trade WebSocket API

**What it demonstrates**:
- Connecting to Coinbase WebSocket feed
- Parsing Level 2 channel messages (JSON)
- Converting exchange data to library format (double → fixed-point)
- Handling snapshot and incremental updates
- Observer pattern for real-time notifications
- Error handling and reconnection logic
- Multiple product subscriptions

**Key Concepts**:
- Exchange integration requires protocol-specific parsing
- Type conversions: `double` prices → `int64_t` fixed-point
- Snapshot callbacks for initial orderbook load
- Incremental updates maintain book state
- Level index tracking for efficient top-N filtering

**Output Example**:
```
=== Coinbase Integration Example ===

Connecting to Coinbase WebSocket...
Subscribing to level2 channel for BTC-USD...

[Snapshot] Received 50 bid levels, 50 ask levels
  Best Bid: 43250.50 x 1.25 BTC
  Best Ask: 43252.00 x 0.85 BTC

[Update] BTC-USD: Bid @ 43250.50, qty=1.35 (+0.10)
[Update] BTC-USD: Ask @ 43251.50, qty=0.00 (deleted)
...
```

**Use Case**: Real exchange connectivity, live market data feeds, algorithmic trading systems, price discovery.

---

## Running Examples

### Simple L2 OrderBook

```bash
cd build/examples
./simple_l2_orderbook
```

**No arguments needed** - runs a predefined scenario demonstrating L2 operations.

---

### Simple L3 OrderBook

```bash
cd build/examples
./simple_l3_orderbook
```

**No arguments needed** - demonstrates L3 order tracking with a predefined sequence.

---

### Multi-Symbol OrderBook

```bash
cd build/examples
./multi_symbol_orderbook
```

**No arguments needed** - manages 3 symbols (BTC, ETH, SOL) with simulated market data.

---

### Coinbase Integration

```bash
cd build/examples
./coinbase_integration [product_id]

# Examples:
./coinbase_integration BTC-USD
./coinbase_integration ETH-USD
./coinbase_integration SOL-USD
```

**Arguments**:
- `product_id` (optional): Coinbase product identifier (default: BTC-USD)

**Requirements**:
- Internet connection (connects to wss://ws-feed.exchange.coinbase.com)
- No API keys needed (public WebSocket feed)
- `slick-net` library (dependency for WebSocket)

**Ctrl+C** to stop the feed gracefully.

---

## Common Patterns

### Pattern 1: Observer for Real-Time Updates

All examples can be extended with custom observers:

```cpp
class MyObserver : public IOrderBookObserver {
public:
    void onPriceLevelUpdate(const PriceLevelUpdate& update) override {
        // React to level changes
        if (update.isTopN(5)) {
            std::cout << "Top 5 level changed\n";
        }
    }

    void onTopOfBookUpdate(const TopOfBook& tob) override {
        // React to ToB changes
        std::cout << "New spread: " << tob.getSpread() << "\n";
    }

    // Implement other required callbacks...
    void onOrderUpdate(const OrderUpdate&) override {}
    void onTrade(const Trade&) override {}
    void onSnapshotBegin(SymbolId, uint64_t, Timestamp) override {}
    void onSnapshotEnd(SymbolId, uint64_t, Timestamp) override {}
};

// Usage:
OrderBookL2 book(1);
auto observer = std::make_shared<MyObserver>();
book.registerObserver(observer);
```

---

### Pattern 2: Fixed-Point Price Conversion

Exchanges use floating-point, but orderbook uses fixed-point:

```cpp
// Exchange provides double
double exchange_price = 100.2550;

// Convert to fixed-point (4 decimal places)
constexpr int64_t PRICE_SCALE = 10000;
Price fixed_price = static_cast<Price>(exchange_price * PRICE_SCALE);
// Result: 1002550

// Convert back to display
double display_price = static_cast<double>(fixed_price) / PRICE_SCALE;
// Result: 100.2550
```

**Why fixed-point?**
- Exact decimal representation (no floating-point errors)
- Faster integer arithmetic
- Deterministic comparisons

---

### Pattern 3: Batch Updates with Snapshots

For initial orderbook load, use snapshot mode:

```cpp
OrderBookL2 book(1);

// Notify observers of snapshot start
book.emitSnapshot(seq_num, timestamp);

// Load all levels (observers notified once at end)
for (const auto& level : snapshot_data) {
    book.updateLevel(level.side, level.price, level.qty, timestamp, seq_num);
}

// Snapshot end automatically emitted
```

**Benefit**: Reduces notification overhead from O(n) to O(1).

---

### Pattern 4: Top-N Level Filtering

Only react to changes in top levels:

```cpp
void onPriceLevelUpdate(const PriceLevelUpdate& update) override {
    // Only care about top 10 levels
    if (update.isTopN(10)) {
        processTopLevelChange(update);
    }
}
```

**Benefit**: Efficient filtering without maintaining separate state.

---

## Building Custom Examples

Template for your own example:

```cpp
#include <slick/orderbook/orderbook.hpp>
#include <iostream>

using namespace slick::orderbook;

int main() {
    // Create orderbook
    OrderBookL2 book(1);

    // Your custom logic here...

    // Query and display results
    auto tob = book.getTopOfBook();
    std::cout << "Best Bid: " << tob.best_bid_price
              << " x " << tob.best_bid_quantity << "\n";

    return 0;
}
```

**Build with CMake**:

```cmake
# In your CMakeLists.txt
add_executable(my_example my_example.cpp)
target_link_libraries(my_example PRIVATE slick::orderbook)
```

---

## Troubleshooting

### Issue: "slick/orderbook/orderbook.hpp not found"

**Solution**: Ensure you've installed the library or are linking against the build directory:

```cmake
# Option 1: Install first
cmake --install build --prefix /usr/local

# Option 2: Use build tree
target_include_directories(my_example PRIVATE ../include)
```

---

### Issue: Linker errors with header-only mode

**Solution**: Define `SLICK_ORDERBOOK_HEADER_ONLY` before including headers:

```cpp
#define SLICK_ORDERBOOK_HEADER_ONLY
#include <slick/orderbook/orderbook.hpp>
```

---

### Issue: Coinbase example won't connect

**Possible causes**:
1. No internet connection
2. Firewall blocking WebSocket (port 443)
3. Coinbase API changed (check [Coinbase API docs](https://docs.cloud.coinbase.com/))

**Debug**: Add logging to see WebSocket connection status.

---

## Next Steps

- **Modify examples** to experiment with different scenarios
- **Profile examples** using the [Profiling Guide](../benchmarks/PROFILING_GUIDE.md)
- **Benchmark examples** to measure performance in your use case
- **Extend with your exchange** by adapting the Coinbase integration pattern

---

## Additional Resources

- [README.md](../README.md) - Library overview and quick start
- [ARCHITECTURE.md](../ARCHITECTURE.md) - Internal design details
- [docs/PERFORMANCE.md](../docs/PERFORMANCE.md) - Performance guide

---

**Questions or Issues?** Open an issue on GitHub or check the documentation.
