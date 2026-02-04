# Performance Profiling Guide

## Quick Profiling with Visual Studio (Windows)

### Troubleshooting

**Q: Instrumentation option is grayed out?**
- **A**: Use **CPU Usage (sampling)** instead - it's the recommended option for C++ code
- Instrumentation is often unavailable for native x64 executables or requires special build flags
- CPU Usage provides excellent hot-path analysis without instrumentation overhead

**Q: Can't find the executable in Performance Profiler?**
- **A**: Use "Browse" button and navigate to `y:\repo\slick-orderbook\build\benchmarks\Release\bench_*.exe`

**Q: Profiler shows no useful data?**
- **A**: Make sure you're running Release build (not Debug)
- Ensure benchmark runs long enough (use `--benchmark_min_time=5` argument)

### Option 1: Visual Studio Performance Profiler (Recommended)

1. **Open Visual Studio 2022**
   ```
   File → Open → Project/Solution
   Navigate to: y:\repo\slick-orderbook\build\slick-orderbook.sln
   ```

2. **Set Release Configuration**
   - Build → Configuration Manager → Active Solution Configuration → Release
   - Rebuild the project to ensure fresh binaries with PDB files

3. **Configure Profiling**
   ```
   Debug → Performance Profiler (Alt+F2)

   Select tools:
   ✓ CPU Usage (sampling) - RECOMMENDED, always available
   ☐ Instrumentation (optional, may be grayed out)

   Note: If Instrumentation is grayed out, use CPU Usage (sampling) instead.
   It provides sufficient detail for identifying hot paths without instrumentation overhead.

   Target: Set to one of the benchmark executables
   ```

4. **Profile a Benchmark**
   ```
   Target: Executable
   Command Line: y:\repo\slick-orderbook\build\benchmarks\Release\bench_orderbook_l2.exe
   Arguments: --benchmark_filter="AddNewLevel/100" --benchmark_min_time=5
   Working Directory: y:\repo\slick-orderbook
   ```

   **Alternative (Easier)**: Profile without arguments for all benchmarks:
   ```
   Command Line: y:\repo\slick-orderbook\build\benchmarks\Release\bench_orderbook_l2.exe
   Arguments: (leave empty to run all benchmarks)
   ```

5. **Run the Profiler**
   - Click "Start" button
   - Wait for benchmark to complete (10-60 seconds depending on benchmark)
   - Profiler will automatically open results when done

6. **Analyze Results**
   - **Hot Path** view shows most expensive functions (start here)
   - **Call Tree** shows function hierarchy and who calls what
   - **Functions** view shows time per function sorted by CPU time

   Look for:
   - Top 5-10 functions by CPU time percentage
   - Functions with high "Self Time" (time spent in function itself, not callees)
   - Unexpected hot spots (functions that shouldn't be expensive)
   - Cache misses (if running with hardware counters enabled)
   - Memory allocation calls (`operator new`, `malloc`) in hot path

### Option 2: Command-Line Profiling with Google Benchmark

Run benchmarks with detailed statistics:

```bash
# CPU profiling with detailed timers
y:\repo\slick-orderbook\build\benchmarks\Release\bench_orderbook_l2.exe --benchmark_out=profile_l2.json --benchmark_out_format=json --benchmark_counters_tabular=true

# Analyze specific hot paths
y:\repo\slick-orderbook\build\benchmarks\Release\bench_orderbook_l3.exe --benchmark_filter="AddOrModify" --benchmark_min_time=10
```

### Option 3: Windows Performance Analyzer (WPA)

For deep system-level analysis:

1. **Capture trace**
   ```cmd
   wpr -start CPU -start FileIO
   y:\repo\slick-orderbook\build\benchmarks\Release\bench_market_replay.exe
   wpr -stop perf_trace.etl
   ```

2. **Analyze**
   ```cmd
   wpa perf_trace.etl
   ```

## What to Look For

### 1. Cache Performance
- **L1/L2/L3 cache miss rates**
  - Target: <1% L1 miss rate, <5% L2 miss rate
  - Our cache alignment should reduce these

### 2. CPU Bottlenecks
- **Hot functions** (top 5 by CPU time)
  - `updateLevel()` / `addOrModifyOrder()`
  - `getOrCreateLevel()` helpers
  - Comparator functions in sorted containers
  - Observer notification loops

### 3. Memory Patterns
- **Allocation hot spots**
  - Should see zero allocations in hot path (ObjectPool working)
  - Any `new`/`malloc` calls indicate missed optimization

### 4. Branch Prediction
- **Misprediction rates**
  - Target: <5% misprediction rate
  - Side enum comparisons should predict well
  - Level existence checks might mispredict

## Common Optimization Opportunities

Based on profiling, look for:

1. **Frequent function calls** → Inline candidates
2. **Virtual dispatch** → Can we devirtualize?
3. **Unnecessary copies** → Pass by reference
4. **Repeated lookups** → Cache results
5. **Linear searches** → Binary search or hash lookup

## Benchmark-Specific Profiling

### High-Frequency Path (Most Important)
```bash
# L2 Add/Modify/Delete - the hottest path
bench_orderbook_l2.exe --benchmark_filter="AddNewLevel|ModifyLevel|DeleteLevel" --benchmark_min_time=10

# L3 Order operations
bench_orderbook_l3.exe --benchmark_filter="AddOrModify|DeleteOrder" --benchmark_min_time=10
```

### Multi-Symbol Scenarios
```bash
# Where cache alignment matters most
bench_market_replay.exe --benchmark_filter="MultiSymbol" --benchmark_min_time=10
bench_orderbook_manager.exe --benchmark_filter="MultiSymbolL2Updates" --benchmark_min_time=10
```

### Observer Impact
```bash
# Notification overhead
bench_observer_overhead.exe --benchmark_filter="WithCountingObservers" --benchmark_min_time=5
```

## Intel VTune (Advanced - Optional)

If you want the deepest insights:

1. **Download Intel VTune Profiler** (free)
   https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html

2. **Run Hotspot Analysis**
   ```cmd
   vtune -collect hotspots -- bench_orderbook_l2.exe --benchmark_filter="AddNewLevel/100" --benchmark_min_time=10
   ```

3. **Analyze Cache Misses**
   ```cmd
   vtune -collect memory-access -- bench_market_replay.exe --benchmark_filter="MultiSymbol/100"
   ```

4. **Review Results**
   - Top-down tree shows expensive call paths
   - Bottom-up shows hottest functions
   - Cache metrics validate alignment benefits

## Current Performance Baseline

From our benchmarks:

| Operation | Latency | Throughput | Target |
|-----------|---------|------------|--------|
| L2 Add    | 21-28ns | 36-49M/s  | <100ns ✓ |
| L2 Modify | 24-31ns | 32-42M/s  | <100ns ✓ |
| L3 Add    | 59ns    | 17M/s     | <200ns ✓ |
| Best Bid  | 0.25ns  | 4G/s      | <10ns ✓ |
| Observer  | 2-3ns   | 400M/s    | <50ns ✓ |

**All targets exceeded** - profiling is for micro-optimization only.

## Expected Findings

Based on our architecture:

### ✅ Should Be Fast (Cache-Friendly)
- Sequential iteration over price levels (std::vector)
- Best bid/ask queries (cached TopOfBook)
- Order lookup by ID (hash table)

### ⚠️ Potential Hot Spots
- Binary search in sorted price levels (O(log n))
- Comparator function calls in FlatMap
- Observer iteration (linear in observer count)
- Order priority sorting at insertion

### 🎯 Optimization Opportunities
If profiling shows issues:
1. Inline comparator functions
2. SIMD for observer notifications
3. Prefetch next price level during search
4. Custom allocator tuning

## Next Steps After Profiling

1. **Document findings** in benchmarks/PROFILING_RESULTS.md
2. **Prioritize** based on real-world impact
3. **Implement** only high-ROI optimizations
4. **Re-benchmark** to validate improvements
5. **Update** CLAUDE.md with decisions

---

**Note**: Given that all performance targets are already exceeded by 2-40x, focus profiling on understanding behavior rather than finding critical issues.
