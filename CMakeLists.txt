cmake_minimum_required(VERSION 3.20)
project(slick-orderbook VERSION 1.0.0 LANGUAGES CXX)

# C++23 required
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(SLICK_ORDERBOOK_HEADER_ONLY "Build as header-only library" OFF)
option(SLICK_ORDERBOOK_BUILD_SHARED "Build shared library" OFF)
option(SLICK_ORDERBOOK_BUILD_TESTS "Build tests" ON)
option(SLICK_ORDERBOOK_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(SLICK_ORDERBOOK_BUILD_EXAMPLES "Build examples" ON)

# Optimization flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
        $<$<CONFIG:Release>:-flto>
    )
elseif(MSVC)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_compile_options(
        /W4
        /wd4324  # Suppress C4324: structure was padded due to alignment specifier (intentional for cache alignment)
        /std:c++latest  # Enable C++23 features including flat_map
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>  # Whole program optimization
    )
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>  # Link-time code generation (required with /GL)
    )
endif()

# Source files for compiled library
set(SLICK_ORDERBOOK_SOURCES
    src/core/orderbook_l2.cpp
    src/core/orderbook_l3.cpp
    src/core/orderbook_manager.cpp
)

# Header files
set(SLICK_ORDERBOOK_HEADERS
    include/slick/orderbook/config.hpp
    include/slick/orderbook/types.hpp
    include/slick/orderbook/concepts.hpp
    include/slick/orderbook/events.hpp
    include/slick/orderbook/observer.hpp
    include/slick/orderbook/orderbook_l2.hpp
    include/slick/orderbook/orderbook_l3.hpp
    include/slick/orderbook/orderbook_manager.hpp
    include/slick/orderbook/orderbook.hpp
)

# Private headers
set(SLICK_ORDERBOOK_PRIVATE_HEADERS
    include_private/slick/orderbook/detail/price_level.hpp
    include_private/slick/orderbook/detail/order.hpp
    include_private/slick/orderbook/detail/flat_map.hpp
    include_private/slick/orderbook/detail/intrusive_list.hpp
    include_private/slick/orderbook/detail/memory_pool.hpp
    include_private/slick/orderbook/detail/level_container.hpp
    include_private/slick/orderbook/detail/order_map.hpp
)

# Library target
if(SLICK_ORDERBOOK_HEADER_ONLY)
    # Header-only library
    add_library(slick-orderbook INTERFACE)
    target_compile_definitions(slick-orderbook INTERFACE SLICK_ORDERBOOK_HEADER_ONLY)
    target_include_directories(slick-orderbook INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include_private>
        $<INSTALL_INTERFACE:include>
    )

    # Fix for GCC/Clang with LTO: explicitly link C++ standard library
    # LTO can cause the linker to lose track of C++ stdlib dependency
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_libraries(slick-orderbook INTERFACE stdc++)
    endif()
else()
    # Compiled library (static or shared)
    if(SLICK_ORDERBOOK_BUILD_SHARED)
        add_library(slick-orderbook SHARED ${SLICK_ORDERBOOK_SOURCES})
        target_compile_definitions(slick-orderbook PRIVATE SLICK_ORDERBOOK_BUILD)
    else()
        add_library(slick-orderbook STATIC ${SLICK_ORDERBOOK_SOURCES})
    endif()

    target_include_directories(slick-orderbook
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include_private
    )
endif()

# Add alias for consistent usage
add_library(slick::orderbook ALIAS slick-orderbook)

# Only build tests, benchmarks, and examples if this is the top-level project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Subdirectories
    if(SLICK_ORDERBOOK_BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()

    if(SLICK_ORDERBOOK_BUILD_BENCHMARKS)
        add_subdirectory(benchmarks)
    endif()

    if(SLICK_ORDERBOOK_BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()
endif()

# Installation rules
if(NOT SLICK_ORDERBOOK_HEADER_ONLY)
    install(TARGETS slick-orderbook
        EXPORT slick-orderbook-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

if(NOT SLICK_ORDERBOOK_HEADER_ONLY)
    install(EXPORT slick-orderbook-targets
        FILE slick-orderbook-targets.cmake
        NAMESPACE slick::
        DESTINATION lib/cmake/slick-orderbook
    )
endif()

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/slick-orderbook-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/slick-orderbook-config-version.cmake"
    DESTINATION lib/cmake/slick-orderbook
)
