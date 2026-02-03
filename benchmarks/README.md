# Slick OrderBook Benchmarks

Performance benchmarking suite for the Slick OrderBook library.

## Overview

This directory contains comprehensive benchmarks measuring the performance characteristics of the orderbook library:

- **Latency Measurements**: p50, p99, p99.9, p99.99 percentiles
- **Throughput Tests**: Operations per second under sustained load
- **Memory Profiling**: Footprint and allocation patterns
- **Cache Performance**: Alignment, locality, and prefetching effects
- **Observer Overhead**: Notification system impact
- **Realistic Workloads**: Market data replay simulations

## Building Benchmarks

```bash
# Configure with benchmarks enabled
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSLICK_ORDERBOOK_BUILD_BENCHMARKS=ON

# Build
cmake --build build -j

# Benchmarks will be in: build/benchmarks/
```

## Running Benchmarks

### Individual Benchmarks

```bash
cd build/benchmarks

# L2 orderbook benchmarks
./bench_orderbook_l2

# L3 orderbook benchmarks
./bench_orderbook_l3

# Multi-symbol manager benchmarks
./bench_orderbook_manager

# Observer notification overhead
./bench_observer_overhead

# Memory usage profiling
./bench_memory_usage

# Realistic market replay
./bench_market_replay

# Cache alignment and locality
./bench_cache_alignment
```

### Run All Benchmarks

```bash
cd build
cmake --build . --target run_benchmarks

# Results will be in build/benchmarks/*.json
```

### Benchmark Options

Google Benchmark supports many command-line options:

```bash
# Run specific benchmark by name
./bench_orderbook_l2 --benchmark_filter=BM_L2_AddNewLevel

# Run for longer to get more stable results
./bench_orderbook_l2 --benchmark_min_time=10

# Output to console and JSON
./bench_orderbook_l2 --benchmark_out=results.json --benchmark_out_format=json

# Show only summary statistics
./bench_orderbook_l2 --benchmark_counters_tabular=true

# Run with specific thread count
./bench_orderbook_manager --benchmark_filter=Concurrent
```

## Benchmark Descriptions

### 1. bench_orderbook_l2

**Purpose**: Measure L2 orderbook operation latencies

**Benchmarks**:
- `BM_L2_AddNewLevel`: Adding new price levels
- `BM_L2_ModifyExistingLevel`: Modifying quantities at existing levels
- `BM_L2_DeleteLevel`: Removing price levels
- `BM_L2_GetBestBid/Ask`: Top-of-book queries (hot path)
- `BM_L2_GetTopOfBook`: Full ToB snapshot
- `BM_L2_GetLevels`: Iterating through all levels
- `BM_L2_MixedWorkload`: Realistic mix of operations

**Target Performance**: < 100ns p99 for all operations

### 2. bench_orderbook_l3

**Purpose**: Measure L3 orderbook operation latencies

**Benchmarks**:
- `BM_L3_AddNewOrder`: Adding new orders
- `BM_L3_ModifyExistingOrder`: Modifying order quantities
- `BM_L3_DeleteOrder`: Removing orders
- `BM_L3_ExecuteOrderPartial`: Partial order fills
- `BM_L3_ExecuteOrderComplete`: Complete order fills
- `BM_L3_GetLevelsL2`: L2 aggregation from L3 data
- `BM_L3_GetLevelsL3`: Iterating through all orders
- `BM_L3_MixedWorkload`: Realistic mix of operations

**Target Performance**: < 200ns p99 for all operations

### 3. bench_orderbook_manager

**Purpose**: Measure multi-symbol overhead and scalability

**Benchmarks**:
- `BM_Manager_GetOrCreateL2/L3`: Symbol lookup and creation
- `BM_Manager_MultiSymbolL2/L3Updates`: Distributed updates across symbols
- `BM_Manager_SymbolLookup`: Raw lookup performance
- `BM_Manager_SymbolChurn`: Add/remove symbol cycles
- `BM_Manager_IterateAllSymbols`: Iteration overhead
- `BM_Manager_ConcurrentReadHeavy`: Multi-threaded read scaling
- `BM_SingleSymbol_*` vs `BM_Manager_*`: Overhead comparison

**Target Performance**: Minimal overhead compared to single-symbol operations

### 4. bench_observer_overhead

**Purpose**: Measure notification system impact

**Benchmarks**:
- `BM_L2/L3_NoObservers`: Baseline (no observers attached)
- `BM_L2/L3_WithCountingObservers`: Minimal observer overhead
- `BM_L2/L3_WithComputingObservers`: Observer with computation
- `BM_L2/L3_EmitSnapshot`: Snapshot emission cost
- `BM_AddRemoveObserver`: Observer management overhead

Tests with 1, 5, 10, 50, and 100 observers to measure scaling.

**Target Performance**: < 50ns per observer notification

### 5. bench_memory_usage

**Purpose**: Profile memory footprint and allocation patterns

**Benchmarks**:
- `BM_L2/L3_MemoryFootprint`: Static memory usage at different sizes
- `BM_L2/L3_MemoryGrowth`: Incremental growth patterns
- `BM_L2/L3_MemoryChurn`: Add/delete cycle efficiency
- `BM_Manager_MemoryFootprint_*`: Multi-symbol memory usage
- `BM_L2_CacheLocality_*`: Sequential vs random access patterns

**Target Memory**:
- L2: < 1KB per symbol
- L3: < 10KB per symbol (with 1000 orders)

**Note**: For accurate absolute memory measurements, use external tools:

```bash
# Valgrind massif
valgrind --tool=massif ./bench_memory_usage

# Heaptrack (Linux)
heaptrack ./bench_memory_usage

# Instruments (macOS)
instruments -t Allocations ./bench_memory_usage
```

### 6. bench_market_replay

**Purpose**: Simulate realistic market data patterns

**Benchmarks**:
- `BM_L2/L3_MarketReplay_LowFreq`: 100 events/sec (typical)
- `BM_L2/L3_MarketReplay_HighFreq`: 10,000 events/sec (HFT)
- `BM_MultiSymbol_MarketReplay`: Multi-symbol scenarios
- `BM_BurstPattern_MarketOpen`: Snapshot + incremental updates
- `BM_Throughput_SustainedUpdates_*`: Maximum sustained throughput

**Event Distribution**:
- L2: 50% modify, 30% add, 20% delete
- L3: 40% new order, 20% modify, 20% delete, 10% execute, 10% query

### 7. bench_cache_alignment

**Purpose**: Diagnostic tool for cache optimization

**Features**:
- Prints alignment information for all critical structures
- Compares aligned vs unaligned access patterns
- Measures false sharing impact
- Array-of-Structures (AoS) vs Structure-of-Arrays (SoA)
- Sequential vs random access (cache locality)
- Prefetch effectiveness
- Hot vs cold cache performance

**Running**:

```bash
./bench_cache_alignment

# Look for alignment info in output:
# ✓ Cache-aligned (64 bytes)
# ⚠ SIMD-aligned (16 bytes) but not cache-aligned
# ✗ Not aligned for optimal performance
```

## Performance Targets

| Component | Operation | Target (p99) |
|-----------|-----------|--------------|
| L2 OrderBook | Add/Modify/Delete Level | < 100 nanoseconds |
| L2 OrderBook | Get Best Bid/Ask | < 10 nanoseconds |
| L2 OrderBook | Get Top of Book | < 10 nanoseconds |
| L3 OrderBook | Add/Modify/Delete Order | < 200 nanoseconds |
| L3 OrderBook | Execute Order | < 200 nanoseconds |
| L3 OrderBook | L2 Aggregation | < 500 nanoseconds |
| Observer | Notification per Observer | < 50 nanoseconds |
| Manager | Symbol Lookup | < 50 nanoseconds |

## Interpreting Results

### Reading Benchmark Output

```
---------------------------------------------------------------------------
Benchmark                                     Time             CPU   Iterations
---------------------------------------------------------------------------
BM_L2_AddNewLevel/10                       68.2 ns         68.1 ns     10240000
BM_L2_AddNewLevel/50                       89.5 ns         89.4 ns      7823104
BM_L2_AddNewLevel/100                       112 ns          112 ns      6241280
```

- **Time**: Wall-clock time per iteration
- **CPU**: CPU time per iteration (may differ from wall time with I/O or threading)
- **Iterations**: Number of times benchmark ran to get stable results

### Key Metrics

1. **Latency**: Time per operation (lower is better)
2. **Throughput**: Operations per second (higher is better)
3. **Scalability**: Performance vs problem size (flatter is better)
4. **Overhead**: Difference between baseline and feature (smaller is better)

### Performance Analysis

**Good Performance**:
- L2 operations consistently < 100ns
- L3 operations consistently < 200ns
- Linear or better scaling with size
- Minimal observer overhead (< 10% per observer)

**Performance Issues**:
- Operations > 1μs (investigate why)
- Exponential scaling with size (O(n²) behavior)
- Large variance between runs (inconsistent)
- High observer overhead (> 50ns per observer)

## Advanced Profiling

### CPU Performance Counters (Linux)

```bash
# Cache misses
perf stat -e cache-references,cache-misses ./bench_orderbook_l2

# Branch prediction
perf stat -e branches,branch-misses ./bench_orderbook_l2

# Detailed profiling
perf record -g ./bench_orderbook_l2
perf report
```

### VTune (Intel)

```bash
vtune -collect hotspots -result-dir vtune_results ./bench_orderbook_l2
```

### Tracy Profiler

Integrate Tracy for frame-based profiling:

```cpp
#include <tracy/Tracy.hpp>

void addOrModifyLevel(...) {
    ZoneScoped;  // Automatic function profiling
    // ... implementation
}
```

## Continuous Performance Monitoring

Set up CI to track performance over time:

1. Run benchmarks on every commit
2. Store results in time-series database
3. Alert on regressions (> 10% slowdown)
4. Visualize trends with Grafana or similar

Example: GitHub Actions + benchmark-action

## Optimization Workflow

1. **Profile**: Run benchmarks to identify bottlenecks
2. **Hypothesize**: Form theory about what's slow and why
3. **Optimize**: Make targeted code changes
4. **Verify**: Re-run benchmarks to confirm improvement
5. **Iterate**: Repeat until targets met

Always measure before and after optimizations!

## Contributing

When adding new features:

1. Add corresponding benchmarks
2. Document expected performance
3. Verify no regressions in existing benchmarks
4. Update this README with new benchmark descriptions

## Resources

- [Google Benchmark Documentation](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [Performance Analysis Tools](https://easyperf.net/blog/)
- [Optimization Techniques](https://www.agner.org/optimize/)
- [Cache Effects](https://igoro.com/archive/gallery-of-processor-cache-effects/)
