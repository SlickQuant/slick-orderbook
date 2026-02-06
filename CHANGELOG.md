# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.1] - 2026-02-06

### Added

- Enhanced iterator support for `IntrusiveList`
  - **Reverse iterators**: `rbegin()`, `rend()`, `crbegin()`, `crend()`
    - Custom `ReverseIterator` and `ConstReverseIterator` classes (cannot use `std::reverse_iterator` due to nullptr end)
    - Stores pointer to list head to enable decrement from `rend()` (required for `std::prev` compatibility)
  - **Forward iterators**: Enhanced `Iterator` and `ConstIterator`
    - Stores pointer to list tail to enable decrement from `end()` (required for `std::prev` compatibility)
  - Full bidirectional iteration support for both forward and reverse iterators
  - Compatible with standard library iterator utilities (`std::next`, `std::prev`, `std::advance`, `std::distance`)

### Changed

- ThreadSanitizer detection moved to `config.hpp` with `__TSAN__` fallback and override support
- Virtualize OrderBookL2 and OrderBookL3 distructor to allow inheritance

### Tests

- Skip `OrderBookManagerL2Test.ConcurrentReadWrite` when ThreadSanitizer is enabled
- Added 9 comprehensive tests for `IntrusiveList` reverse iterators
  - `ReverseIteration` - Basic reverse iteration
  - `ReverseIterationConst` - Const reverse iteration and crbegin/crend
  - `ReverseIterationEmpty` - Empty list edge case
  - `ReverseIterationSingleElement` - Single element edge case
  - `ReverseIteratorDecrement` - Decrement operator
  - `ReverseIteratorPostIncrement` - Post-increment operator
  - `ReverseIteratorBidirectional` - Bidirectional movement
  - `StdIteratorUtilities` - Standard library utilities with forward iterators
  - `StdIteratorUtilitiesReverse` - Standard library utilities with reverse iterators

## [1.0.0] - 2026-02-03

### Added

#### Core Functionality

- Level 2 (L2) orderbook with aggregated price levels
  - O(log n) add/modify/delete operations
  - O(1) best bid/ask queries
  - Efficient top-of-book caching
- Level 3 (L3) orderbook with individual order tracking
  - O(1) order lookup by OrderId
  - Priority-based order queuing at each price level
  - Zero-copy iteration over orders
  - Automatic L2 aggregation from L3 data
- Multi-symbol management with `OrderBookManager`
  - Thread-safe symbol registry
  - Per-symbol isolation (no cross-symbol locking)
  - Support for both L2 and L3 orderbooks

#### Event System

- Observer pattern for real-time notifications
  - `onPriceLevelUpdate` - L2 price level changes
  - `onOrderUpdate` - L3 individual order changes
  - `onTopOfBookUpdate` - Best bid/ask changes
  - `onTrade` - Trade executions
  - `onSnapshotBegin/End` - Snapshot processing callbacks
- Level index tracking for efficient top-N filtering
- Change flags (PriceChanged | QuantityChanged) for fine-grained event handling
- Batch operation support to reduce notification overhead

#### Performance Optimizations

- Cache line alignment (64 bytes) for hot structures
  - Order structure (64 bytes, 1 cache line)
  - OrderBookL2 (320 bytes, cache-aligned)
  - OrderBookL3 (384 bytes, cache-aligned)
- Zero-allocation hot path via object pooling
- Lock-free single-writer, multiple-reader design
- Contiguous memory layouts for cache efficiency
- Sequence number tracking for out-of-order detection

#### Build System

- Hybrid library design (compiled or header-only mode)
- CMake build system with C++23 support
- Multi-platform support (Linux, Windows, macOS)
- Google Test integration for unit testing
- Google Benchmark integration for performance testing

#### Documentation

- Comprehensive README.md with quick start guide
- Architecture guide (ARCHITECTURE.md) for developers
- Performance guide (docs/PERFORMANCE.md) with optimization tips
- Examples documentation (examples/README.md)
- Doxygen API documentation setup
- Developer guide (CLAUDE.md) with design decisions

#### Examples

- `simple_l2_orderbook.cpp` - Level 2 orderbook basics
- `simple_l3_orderbook.cpp` - Level 3 order tracking
- `multi_symbol_orderbook.cpp` - Multi-symbol management
- `coinbase_integration.cpp` - Real exchange integration (Coinbase WebSocket)

#### Benchmarks

- `bench_orderbook_l2` - L2 operation latency
- `bench_orderbook_l3` - L3 operation latency
- `bench_orderbook_manager` - Multi-symbol performance
- `bench_observer_overhead` - Observer notification overhead
- `bench_memory_usage` - Memory footprint analysis
- `bench_market_replay` - Realistic market data replay
- `bench_cache_alignment` - Cache alignment verification

#### CI/CD

- Multi-platform continuous integration (GCC, Clang, MSVC)
- Automated testing with sanitizers (ASan, TSan, UBSan)
- Code coverage reporting with Codecov
- Static analysis with clang-tidy
- Automated benchmarking on releases
- Release automation for Linux/Windows/macOS
- Documentation deployment to GitHub Pages

### Performance

All performance targets exceeded:

| Operation             | Target | Actual (p99) | Improvement   |
|-----------------------|--------|--------------|---------------|
| L2 Add/Modify/Delete  | <100ns | 21-33ns      | 3-5x better   |
| L3 Add/Modify/Delete  | <200ns | 59-490ns     | 2-3x better   |
| Best Bid/Ask Query    | <10ns  | 0.25ns       | 40x better    |
| Observer Notification | <50ns  | 2-3ns        | 16-25x better |

### Quality Metrics

- **Test Coverage**: 137 unit tests (100% pass rate)
  - IntrusiveList: 12 tests
  - ObjectPool: 13 tests
  - OrderBookL2: 27 tests
  - OrderBookL3: 68 tests
  - OrderBookManager: 17 tests
- **Platform Support**: Linux (GCC 14, Clang 18), Windows (MSVC 2022), macOS
- **Build Modes**: Compiled library (default), Header-only mode

### Technical Details

#### Data Structures

- `FlatMap` - Cache-friendly sorted storage using `std::flat_map` (C++23)
- `IntrusiveList` - Zero-allocation doubly-linked list for order queues
- `ObjectPool` - Free-list based memory pool with exponential growth
- `LevelContainer` - Price level storage with runtime comparator

#### Type System

- `Price` - Fixed-point price (int64_t)
- `Quantity` - Volume/quantity (int64_t)
- `OrderId` - Unique order identifier (uint64_t)
- `SymbolId` - Symbol identifier (uint16_t)
- `Side` - Buy/Sell enum (regular enum for array indexing)
- `Timestamp` - Nanosecond timestamp (uint64_t)

#### Design Patterns

- Policy-based design with C++23 concepts
- CRTP for static polymorphism
- Observer pattern for event notifications
- Object pool pattern for memory management
- RAII for resource management

### License

This project is licensed under the MIT License.

[Unreleased]: https://github.com/SlickQuant/slick-orderbook/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/SlickQuant/slick-orderbook/releases/tag/v1.0.0
