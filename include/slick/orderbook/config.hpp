// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

// Version information
#define SLICK_ORDERBOOK_VERSION_MAJOR 1
#define SLICK_ORDERBOOK_VERSION_MINOR 0
#define SLICK_ORDERBOOK_VERSION_PATCH 0

// API export/import macros
#ifdef SLICK_ORDERBOOK_HEADER_ONLY
    // Header-only mode: everything is inline
    #define SLICK_API inline
#else
    // Compiled library mode: handle DLL export/import
    #ifdef _WIN32
        #ifdef SLICK_ORDERBOOK_BUILD
            // Building the library
            #define SLICK_API __declspec(dllexport)
        #else
            // Using the library
            #define SLICK_API __declspec(dllimport)
        #endif
    #else
        // Unix-like systems
        #define SLICK_API __attribute__((visibility("default")))
    #endif
#endif

// Compiler feature detection
#if defined(__cpp_concepts) && __cpp_concepts >= 201907L
    #define SLICK_HAS_CONCEPTS 1
#else
    #define SLICK_HAS_CONCEPTS 0
#endif

// Likely/unlikely hints for branch prediction
#if defined(__GNUC__) || defined(__clang__)
    #define SLICK_LIKELY(x) __builtin_expect(!!(x), 1)
    #define SLICK_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define SLICK_LIKELY(x) (x)
    #define SLICK_UNLIKELY(x) (x)
#endif

// Cache line size (platform-specific)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define SLICK_CACHE_LINE_SIZE 64
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define SLICK_CACHE_LINE_SIZE 64
#else
    #define SLICK_CACHE_LINE_SIZE 64  // Conservative default
#endif

// Force inline hints
#if defined(_MSC_VER)
    #define SLICK_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define SLICK_FORCE_INLINE inline __attribute__((always_inline))
#else
    #define SLICK_FORCE_INLINE inline
#endif

// No inline hints
#if defined(_MSC_VER)
    #define SLICK_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    #define SLICK_NO_INLINE __attribute__((noinline))
#else
    #define SLICK_NO_INLINE
#endif

// Alignment macros
#define SLICK_ALIGNAS(n) alignas(n)
#define SLICK_CACHE_ALIGNED SLICK_ALIGNAS(SLICK_CACHE_LINE_SIZE)

// Namespace macros for easier internal usage
#define SLICK_NAMESPACE_BEGIN namespace slick::orderbook {
#define SLICK_NAMESPACE_END }

#define SLICK_DETAIL_NAMESPACE_BEGIN namespace slick::orderbook::detail {
#define SLICK_DETAIL_NAMESPACE_END }

// Debug assertions
#ifndef NDEBUG
    #include <cassert>
    #define SLICK_ASSERT(expr) assert(expr)
#else
    #define SLICK_ASSERT(expr) ((void)0)
#endif

// Unreachable code hint
#if defined(__GNUC__) || defined(__clang__)
    #define SLICK_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
    #define SLICK_UNREACHABLE() __assume(0)
#else
    #define SLICK_UNREACHABLE() ((void)0)
#endif
