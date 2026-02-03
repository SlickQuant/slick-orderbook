/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Slick OrderBook", "index.html", [
    [ "✨ Features", "index.html#autotoc_md1", null ],
    [ "📈 Performance", "index.html#autotoc_md2", null ],
    [ "🚀 Quick Start", "index.html#autotoc_md3", [
      [ "Prerequisites", "index.html#autotoc_md4", null ],
      [ "Installation", "index.html#autotoc_md5", null ],
      [ "Basic Usage - Level 2 OrderBook", "index.html#autotoc_md6", null ],
      [ "Basic Usage - Level 3 OrderBook", "index.html#autotoc_md7", null ],
      [ "Multi-Symbol Management", "index.html#autotoc_md8", null ],
      [ "Observer Pattern - Real-Time Notifications", "index.html#autotoc_md9", null ]
    ] ],
    [ "📚 Documentation", "index.html#autotoc_md10", null ],
    [ "🏗️ Architecture Highlights", "index.html#autotoc_md11", [
      [ "Cache-Optimized Data Structures", "index.html#autotoc_md12", null ],
      [ "Zero-Cost Abstractions", "index.html#autotoc_md13", null ],
      [ "Lock-Free Design", "index.html#autotoc_md14", null ]
    ] ],
    [ "🔧 CMake Options", "index.html#autotoc_md15", null ],
    [ "📦 Integration", "index.html#autotoc_md16", [
      [ "CMake FetchContent", "index.html#autotoc_md17", null ],
      [ "Header-Only Mode", "index.html#autotoc_md18", null ]
    ] ],
    [ "🧪 Testing", "index.html#autotoc_md19", null ],
    [ "📊 Benchmarking", "index.html#autotoc_md20", null ],
    [ "📄 License", "index.html#autotoc_md21", null ],
    [ "🙏 Acknowledgments", "index.html#autotoc_md22", null ],
    [ "📧 Contact", "index.html#autotoc_md23", null ],
    [ "Slick OrderBook - Architecture Guide", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html", [
      [ "Table of Contents", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md27", null ],
      [ "Overview", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md29", [
        [ "Architecture Principles", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md30", null ]
      ] ],
      [ "Core Components", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md32", [
        [ "OrderBookL2 - Aggregated Price Levels", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md33", null ],
        [ "OrderBookL3 - Order-by-Order Tracking", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md34", null ],
        [ "OrderBookManager - Multi-Symbol Coordination", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md35", null ]
      ] ],
      [ "Data Structures", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md37", [
        [ "FlatMap - Cache-Friendly Sorted Storage", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md38", null ],
        [ "IntrusiveList - Zero-Allocation Order Queue", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md39", null ],
        [ "ObjectPool - Pre-Allocated Memory", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md40", null ]
      ] ],
      [ "Memory Management", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md44", [
        [ "Cache Alignment Strategy", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md46", null ],
        [ "Memory Footprint", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md48", null ]
      ] ],
      [ "Threading Model", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md50", [
        [ "Single-Writer, Multiple-Reader (SWMR)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md51", null ],
        [ "Observer Notifications", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md52", null ]
      ] ],
      [ "Performance Optimizations", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md54", [
        [ "1. Cached TopOfBook", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md55", null ],
        [ "2. Array Indexing by Side", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md56", null ],
        [ "3. Quantity=0 Deletion", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md58", null ],
        [ "4. Return Pairs for Efficiency", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md59", null ],
        [ "5. Zero-Copy Iteration", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md65", null ]
      ] ],
      [ "Event System", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md68", [
        [ "Event Types", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md69", null ],
        [ "Observer Interface", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md72", null ]
      ] ],
      [ "Design Patterns", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md76", [
        [ "1. Template-Based Polymorphism (CRTP)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md77", null ],
        [ "2. Policy-Based Design", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md79", null ],
        [ "3. Observer Pattern (Non-Virtual)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md81", null ],
        [ "4. RAII for Resource Management", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md84", null ]
      ] ],
      [ "Build Modes", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md114", [
        [ "Compiled Library (Default)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md115", null ],
        [ "Header-Only Mode", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md116", null ]
      ] ],
      [ "Performance Profile", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md118", [
        [ "Measured Results", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md119", null ],
        [ "Hot Spots (from profiling)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md120", null ],
        [ "Cache Behavior", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md121", null ]
      ] ],
      [ "Future Considerations", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md123", [
        [ "Potential Enhancements", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md124", null ],
        [ "Not Planned (Premature Optimization)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md125", null ]
      ] ],
      [ "Conclusion", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2ARCHITECTURE.html#autotoc_md127", null ]
    ] ],
    [ "Performance Guide", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html", [
      [ "Table of Contents", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md41", null ],
      [ "Performance Characteristics", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md45", [
        [ "Measured Performance (p99 Latency)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md47", null ],
        [ "Memory Footprint", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md57", null ]
      ] ],
      [ "Best Practices", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md61", [
        [ "1. Choose the Right Orderbook Type", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md62", null ],
        [ "2. Pre-Allocate When Possible", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md63", null ],
        [ "3. Use Batch Updates for Snapshots", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md64", null ],
        [ "4. Filter Events at the Observer", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md66", null ],
        [ "5. Minimize Observer Count", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md70", null ],
        [ "6. Use Const References", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md71", null ],
        [ "7. Cache TopOfBook Queries", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md73", null ],
        [ "8. Use Fixed-Point Arithmetic", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md75", null ],
        [ "9. Thread Affinity for Critical Paths", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md78", null ],
        [ "10. Use Release Builds", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md80", null ]
      ] ],
      [ "Common Pitfalls", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md83", [
        [ "❌ Pitfall 1: Copying Orderbooks", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md86", null ],
        [ "❌ Pitfall 2: Excessive Observer Notifications", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md87", null ],
        [ "❌ Pitfall 3: Unnecessary Conversions", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md88", null ],
        [ "❌ Pitfall 4: Not Using OrderBookManager", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md89", null ],
        [ "❌ Pitfall 5: Debug Assertions in Production", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md90", null ]
      ] ],
      [ "Optimization Techniques", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md92", [
        [ "Technique 1: Compiler Optimizations", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md93", null ],
        [ "Technique 2: Profile-Guided Optimization (PGO)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md94", null ],
        [ "Technique 3: Huge Pages (Linux)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md95", null ],
        [ "Technique 4: NUMA Awareness", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md96", null ]
      ] ],
      [ "Benchmarking", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md98", [
        [ "Running Benchmarks", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md99", null ],
        [ "Interpreting Results", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md100", null ],
        [ "Custom Benchmarks", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md101", null ]
      ] ],
      [ "Profiling", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md103", [
        [ "Visual Studio Performance Profiler (Windows)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md104", null ],
        [ "Linux perf", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md105", null ],
        [ "Intel VTune", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md106", null ]
      ] ],
      [ "Performance Targets", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md108", [
        [ "Target Latencies (p99)", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md109", null ],
        [ "Target Throughput", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md110", null ],
        [ "Memory Targets", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md111", null ]
      ] ],
      [ "Conclusion", "md__2home_2runner_2work_2slick-orderbook_2slick-orderbook_2docs_2PERFORMANCE.html#autotoc_md113", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ],
        [ "Enumerator", "namespacemembers_eval.html", null ]
      ] ]
    ] ],
    [ "Concepts", "concepts.html", "concepts" ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ],
        [ "Typedefs", "functions_type.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"classslick_1_1orderbook_1_1detail_1_1IntrusiveList.html#a7dfea41cdc1d25524c34a7c655a3039e",
"config_8hpp.html#a1c7f1c2705e8bf21227caebde57a2dae",
"price__level__l3_8hpp_source.html",
"structslick_1_1orderbook_1_1detail_1_1PriceLevelL3.html#a3b3d70eda680ff10c1eac512ce4029e2"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';