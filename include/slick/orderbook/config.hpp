// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#pragma once

// Version information (generated from CMakeLists.txt project version)
#include <slick/orderbook/version.hpp>

// API export/import macros (applied to exported classes)
// SLICK_ORDERBOOK_SHARED is propagated by CMake to everything linking the shared library
#if defined(SLICK_ORDERBOOK_HEADER_ONLY) || !defined(SLICK_ORDERBOOK_SHARED)
    // Header-only or static library: nothing to export
    #define SLICK_API
#elif defined(_WIN32)
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

// Explicit template instantiation exports:
// MSVC needs dllexport on the instantiation definition (it is incompatible with `extern template`),
// GCC/Clang take the visibility attribute on the `extern template` declaration instead.
#if defined(_WIN32)
    #define SLICK_TEMPLATE_INSTANTIATION_API SLICK_API
    #if defined(SLICK_ORDERBOOK_BUILD) && defined(SLICK_ORDERBOOK_SHARED)
        #define SLICK_EXTERN_TEMPLATE_DECLS 0
    #else
        #define SLICK_EXTERN_TEMPLATE_DECLS 1
    #endif
#else
    #define SLICK_TEMPLATE_INSTANTIATION_API
    #define SLICK_EXTERN_TEMPLATE_DECLS 1
#endif

// Exported classes hold STL members (C4251); clients must use a compatible toolset anyway
#if defined(_MSC_VER)
    #define SLICK_DLL_INTERFACE_WARNINGS_PUSH __pragma(warning(push)) __pragma(warning(disable: 4251))
    #define SLICK_DLL_INTERFACE_WARNINGS_POP __pragma(warning(pop))
#else
    #define SLICK_DLL_INTERFACE_WARNINGS_PUSH
    #define SLICK_DLL_INTERFACE_WARNINGS_POP
#endif

// Compiler feature detection
#if defined(__has_feature)
    #define SLICK_HAS_FEATURE(x) __has_feature(x)
#else
    #define SLICK_HAS_FEATURE(x) 0
#endif

// ThreadSanitizer detection
#ifndef SLICK_TSAN_ENABLED
#if defined(__SANITIZE_THREAD__)
    #define SLICK_TSAN_ENABLED 1
#elif defined(__TSAN__)
    #define SLICK_TSAN_ENABLED 1
#elif SLICK_HAS_FEATURE(thread_sanitizer)
    #define SLICK_TSAN_ENABLED 1
#else
    #define SLICK_TSAN_ENABLED 0
#endif

#endif

#if defined(__SANITIZE_ADDRESS__) || defined(__ASAN__) || SLICK_HAS_FEATURE(address_sanitizer)
#define SLICK_ASAN_ENABLED 1
#else
#define SLICK_ASAN_ENABLED 0
#endif

// Compiler concepts detection
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
