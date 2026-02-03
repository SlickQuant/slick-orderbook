# Slick OrderBook API Documentation

Welcome to the Slick OrderBook API documentation. This library provides high-performance, low-latency order book management for financial trading systems.

## Quick Links

- @ref slick::orderbook::OrderBookL2 "OrderBookL2" - Level 2 (aggregated) order book
- @ref slick::orderbook::OrderBookL3 "OrderBookL3" - Level 3 (order-by-order) order book
- @ref slick::orderbook::OrderBookManager "OrderBookManager" - Multi-symbol management
- @ref slick::orderbook::IOrderBookObserver "IOrderBookObserver" - Observer interface for events

## Getting Started

### Level 2 OrderBook

The simplest way to track aggregated price levels:

```cpp
#include <slick/orderbook/orderbook.hpp>

using namespace slick::orderbook;

OrderBookL2 book(symbol_id);
book.updateLevel(Side::Buy, 10000, 100, timestamp, seq_num);
auto tob = book.getTopOfBook();
```

See @ref slick::orderbook::OrderBookL2 for full API.

### Level 3 OrderBook

For order-by-order tracking with individual order IDs:

```cpp
OrderBookL3 book(symbol_id);
book.addOrModifyOrder(order_id, Side::Buy, 10000, 100, timestamp, priority, seq_num);
book.executeOrder(order_id, 25, seq_num);
```

See @ref slick::orderbook::OrderBookL3 for full API.

### Multi-Symbol Management

Efficiently manage multiple symbols:

```cpp
OrderBookManager<OrderBookL2> manager;
auto* book = manager.getOrCreateOrderBook(symbol_id);
book->updateLevel(...);
```

See @ref slick::orderbook::OrderBookManager for full API.

## Core Types

- @ref slick::orderbook::Price "Price" - Fixed-point price type (int64_t)
- @ref slick::orderbook::Quantity "Quantity" - Volume/quantity type (int64_t)
- @ref slick::orderbook::OrderId "OrderId" - Unique order identifier (uint64_t)
- @ref slick::orderbook::SymbolId "SymbolId" - Symbol identifier (uint32_t)
- @ref slick::orderbook::Side "Side" - Buy/Sell enum
- @ref slick::orderbook::Timestamp "Timestamp" - Nanosecond timestamp (uint64_t)

## Event Structures

- @ref slick::orderbook::PriceLevelUpdate "PriceLevelUpdate" - L2 price level changes
- @ref slick::orderbook::OrderUpdate "OrderUpdate" - L3 individual order changes
- @ref slick::orderbook::Trade "Trade" - Executed trades
- @ref slick::orderbook::TopOfBook "TopOfBook" - Best bid/ask snapshot

## Performance

All performance targets exceeded:

| Operation | Target | Actual (p99) |
|-----------|--------|--------------|
| L2 Add/Modify/Delete | <100ns | **21-33ns** |
| L3 Add/Modify/Delete | <200ns | **59-490ns** |
| Best Bid/Ask Query | <10ns | **0.25ns** |
| Observer Notification | <50ns | **2-3ns** |

See [Performance Guide](../../docs/PERFORMANCE.md) for optimization techniques.

## Architecture

Key design principles:

- **Zero Allocations**: Object pooling eliminates runtime allocations
- **Cache Friendly**: 64-byte cache alignment, contiguous memory layouts
- **Template-Based**: Zero-cost abstractions, no virtual dispatch in hot paths
- **Lock-Free**: Single-writer per symbol, lock-free reads

See [Architecture Guide](../../ARCHITECTURE.md) for internal details.

## Examples

Practical usage examples:

- `simple_l2_orderbook.cpp` - L2 basics
- `simple_l3_orderbook.cpp` - L3 order tracking
- `multi_symbol_orderbook.cpp` - Managing multiple symbols
- `coinbase_integration.cpp` - Real exchange integration

See [examples/README.md](../../examples/README.md) for detailed descriptions.

## Build Modes

### Compiled Library (Default)

```cmake
find_package(slick-orderbook REQUIRED)
target_link_libraries(your_target PRIVATE slick::orderbook)
```

### Header-Only Mode

```cmake
add_compile_definitions(SLICK_ORDERBOOK_HEADER_ONLY)
target_include_directories(your_target PRIVATE /path/to/slick-orderbook/include)
```

## Namespaces

- @ref slick::orderbook - Main public API
- @ref slick::orderbook::detail - Internal implementation details (not for public use)

## Additional Resources

- [README.md](../../README.md) - Project overview and quick start
- [ARCHITECTURE.md](../../ARCHITECTURE.md) - Design and implementation details
- [docs/PERFORMANCE.md](../../docs/PERFORMANCE.md) - Performance guide
- [GitHub Repository](https://github.com/SlickQuant/slick-orderbook)

## License

This project is licensed under the MIT License. See [LICENSE](../../LICENSE) for details.

---

**Organization**: Slick Quant

**Version**: 1.0.0
