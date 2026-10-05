# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.0] - 2026-10-04

### Breaking Changes

- **ABI**: The compiled library is not ABI compatible with 1.0.x; rebuild code that links it.
  - `OrderBookL2::deleteLevel()`, `OrderBookL2::clearSide()`, `OrderBookL2::clear()`,
    `OrderBookL3::clearSide()` and `OrderBookL3::clear()` take a trailing `Timestamp timestamp = 0`
    parameter (used for the top-of-book notification) and are no longer `noexcept`, since they may
    now call observers. Existing calls still compile.
  - `OrderBookManager` stores orderbooks in `std::shared_ptr` instead of `std::unique_ptr`.
- **CMake package versioning**: A compiled (static or shared) install only satisfies `find_package`
  requests for the same `MAJOR.MINOR` (`SameMinorVersion`), so `find_package(slick-orderbook 1.0)`
  no longer resolves to a 1.1 install. Header-only installs accept any lower or equal version with
  the same major version (`SameMajorVersion`).
- **Shared library**: SOVERSION is now `MAJOR.MINOR` (e.g. `libslick-orderbook.so.1.1`).
- **Examples**: The Coinbase integration example is no longer built by default; enable it with
  `-DSLICK_ORDERBOOK_BUILD_COINBASE_EXAMPLE=ON` (needs nlohmann-json, OpenSSL, Boost.Beast and
  jwt-cpp installed).

### Added

- **OrderBookManager**: `getOrCreateSharedOrderBook()` and `getSharedOrderBook()` return
  `std::shared_ptr` handles that keep an orderbook alive after `removeOrderBook()`/`clear()`, for
  symbols that may be removed while other threads use them. Raw-pointer getters are unchanged and
  zero-overhead; their lifetime rules are now documented.
- **CMake**: `SLICK_ORDERBOOK_ENABLE_NATIVE_ARCH` option (default `ON`) controls `-march=native` in
  Release builds; turn it off for binaries that must run on other machines.
- **CMake**: All install rules belong to the `slick-orderbook` component, so
  `cmake --install <build> --component slick-orderbook` installs only this library.
- **CMake**: `include/slick/orderbook/version.hpp` is generated from the `project(VERSION ...)` in
  `CMakeLists.txt`, the single source of truth for the version.

### Fixed

- **OrderBookL2/L3**: `deleteLevel()`, `clearSide()` and `clear()` left the cached top-of-book stale,
  so `getTopOfBook()` and (L2) `getBestBid()`/`getBestAsk()` kept reporting removed levels. They now
  refresh the cache and call `onTopOfBookUpdate()` when the best bid/ask changes.
- **OrderBookL3**: `PriceLevelUpdate::num_orders` reported the order count of the whole book instead
  of the order count at that price level. It is also clamped instead of wrapping beyond 65535.
- **Shared library (Windows)**: The DLL exported no symbols, so consumers could not link it.
  `OrderBookL2`, `OrderBookL3` and the `OrderBookManager<OrderBookL2>`/`<OrderBookL3>`
  instantiations are now exported, and `SLICK_ORDERBOOK_SHARED` is propagated to consumers so they
  import them.
- **CMake**: `find_package(slick-orderbook)` failed because no package config file was installed. The
  package now installs `slick-orderbook-config.cmake` for compiled and header-only builds and exports
  the target as `slick::orderbook`.
- **CMake**: The exported target now requires C++23 (`cxx_std_23`); previously consumers had to set
  the language standard themselves.
- **CMake**: A clean default configure failed unless nlohmann_json was installed, because the
  Coinbase example was always configured.
- **CMake**: Building tests with a fetched googletest no longer installs gtest with `cmake --install`.
- **Version macros**: `SLICK_ORDERBOOK_VERSION_*` reported 1.0.3; they now match the project version.
- **OrderBookManager**: Move construction and move assignment were declared but implicitly deleted
  (the class owns a `std::shared_mutex`). They are now implemented: the source's orderbooks are
  transferred, and move assignment releases the destination's previous orderbooks.

### CI

- **Release**: The release workflow no longer rebuilds. It publishes the packages that CI built and
  tested for the tagged commit, after waiting for that commit's CI run on `main` to succeed. The tag
  must match the project version in `CMakeLists.txt`.
- **Release**: Packages are built without LTO and `-march=native` so they run on any machine of the
  target architecture, and contain only the library's install component.
- **CI**: The `ci-success` gate checks every required job, including coverage and static analysis.
- **CI**: Upgraded to `codecov/codecov-action@v5`; a failed Codecov upload (e.g. a TLS error reaching
  Codecov) is reported as a warning instead of failing the coverage job.
- **CI**: Builds the Coinbase example explicitly now that it is opt-in.

### Documentation

- **README**: Fixed examples that did not compile (`forEachOrderBook`, `registerObserver` and the
  `TopOfBook` member names do not exist), corrected the test command, and documented the shared
  library, the install component and the version policy.

### Tests

- **OrderBookL2**: `DeleteBestLevelRefreshesTopOfBook`, `ClearSideRefreshesTopOfBook`,
  `ClearRefreshesTopOfBookAndNotifies`.
- **OrderBookL3**: `ClearSideRefreshesTopOfBook`, `ClearRefreshesTopOfBook`,
  `LevelUpdateNumOrdersIsPerLevel`.
- **OrderBookManager**: `SharedOrderBookOutlivesRemoval`, `ConcurrentRemoveWithSharedHandles`,
  `MoveConstruct`, `MoveAssign`, `MoveAssignLifetime`, plus compile-time move-trait checks.
- Shared-library builds on Windows copy the DLL next to the test executable so tests can run.

## [1.0.5] - 2026-08-09

### Fixed

- Constructor initialization order in OrderUpdate struct
- Add missing algorithm include which causes compiling error in Linux

## [1.0.4] - 2026-08-08

### CI

- **Docs**: Fix generated API docs at slickquant.com always showing version `1.0.0` regardless of the
  actual release — the Doxygen config's `PROJECT_NUMBER` was a hardcoded literal that was never wired
  to the project version.
  - Converted `Doxyfile` to a `Doxyfile.in` template with `PROJECT_NUMBER = @PROJECT_VERSION@`.
  - Added a CMake `docs` target (top-level builds only, requires Doxygen) that `configure_file()`s
    `Doxyfile.in` using the live `PROJECT_VERSION` from `project()` before invoking doxygen, so the
    version updates automatically on every reconfigure.
  - Updated `.github/workflows/documentation.yml` to configure CMake (tests/benchmarks/examples off)
    and build the `docs` target instead of hand-rolling a Doxyfile via `sed`.

### Fixed

- **OrderBookL3**: Fix structured-binding reference in `clearSide()` — iterating `level_map` with
  `auto& [price, level]` does not correctly bind to `std::flat_map`'s non-reference iterator proxy;
  changed to `auto&& [price, level]` so all price levels are visited and cleared correctly.

### Tests

- Added `ClearSideMultipleLevelsAndOrders` to exercise `clearSide()` with multiple price levels and
  multiple orders per level, verifying the other side is left untouched.

## [1.0.3] - 2026-06-22

### Fixed

- **OrderBookL3**: Bug fix in `addOrModifyOrder` and `modifyOrder` where `timestamp` was not forwarded to the internal
  `deleteOrder` call for fully-executed orders — `seq_num` was incorrectly passed in the timestamp
  position, corrupting the delete event's timestamp and seq_num fields in observer notifications.

### Tests

- Added 38 `WithSeqNum` test variants for all `OrderBookL3` functional test sections (initial
  `seq_num = 2`), covering Add, Find, Modify, Delete, Execute, TopOfBook, L2 aggregation, L3 level
  access, Clear, and Observer operations.
- `ExecuteOrderFullyWithSeqNum` directly validates the bug fix by asserting that the delete
  notification carries `timestamp = kTs2` (not the `seq_num` value) and `seq_num = 3`.

### CI

- Use action cache in GitHub Actions workflows to reduce CI build times.

## [1.0.2] - 2026-05-02

### Added

- **OrderBookL3**: Add `interested_num_levels` parameter to constructor for efficient top-N level tracking
  - New constructor signature includes optional `interested_num_levels` parameter (defaults to 10)
  - When set to a non-zero value, only top-N level changes trigger observer notifications
  - Setting to 0 maintains backward compatibility (all levels trigger notifications)
  - Reduces observer overhead for use cases that only care about top of book
- **OrderBookL3**: Add template method `getBestLevel<Side>()` for compile-time side selection
  - Returns pointer to best price level for the specified side (Buy or Sell)
  - Template specializations for `Side::Buy` and `Side::Sell`
  - More efficient than runtime branching for side-agnostic algorithms
  - Returns `nullptr` if no orders exist on that side
- **PriceLevelUpdate**: Add num_orders in PriceLevelUpdate; update related notifyPriceLevelUpdate methods
- **TopOfBookUpdate**: Add change_flags in TopOfBook event; update related notifyTopOfBookUpdate methods

### Changed

- **BREAKING CHANGE**: `OrderBookL3::modifyOrder()` signature updated to include timestamp and priority parameters
  - Old signature: `modifyOrder(OrderId, Price, Quantity, seq_num=0, is_last_in_batch=true)`
  - New signature: `modifyOrder(OrderId, Price, Quantity, Timestamp new_timestamp, uint64_t new_priority=0, seq_num=0, is_last_in_batch=true)`
  - `new_timestamp` is now required (no default value)
  - `new_priority` defaults to 0, meaning "use timestamp as priority" for FIFO ordering (matches `addOrder` behavior)
  - Priority changes at same price level now trigger re-insertion to maintain correct queue position
  - Order timestamp and priority are properly updated on modification
- **BREAKING CHANGE**: `OrderBookL3::deleteOrder()` signature updated to include timestamp parameter
  - Old signature: `deleteOrder(OrderId, seq_num=0, is_last_in_batch=true)`
  - New signature: `deleteOrder(OrderId, Timestamp timestamp, seq_num=0, is_last_in_batch=true)`
  - `timestamp` is now required (no default value)
  - Rename PriceLevelChangeFlag to ChangeFlag
- Changed Quantity type from int64_t to uint64_t in types.hpp
  - Negative quantities are now invalid at the type system level
  - Prevents semantic errors and improves type safety
- Update executeOrder method to include timestamp parameter; adjust related tests
- Changed executeOrder to virtual function

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

[Unreleased]: https://github.com/SlickQuant/slick-orderbook/compare/v1.0.3...HEAD
[1.0.3]: https://github.com/SlickQuant/slick-orderbook/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/SlickQuant/slick-orderbook/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/SlickQuant/slick-orderbook/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/SlickQuant/slick-orderbook/releases/tag/v1.0.0
