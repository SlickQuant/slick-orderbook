# Copyright 2026 Slick Quant
# SPDX-License-Identifier: MIT

# Examples for Slick OrderBook

# L2 OrderBook Example
add_executable(simple_l2_orderbook
    simple_l2_orderbook.cpp
)

target_link_libraries(simple_l2_orderbook
    PRIVATE
        slick::orderbook
)

target_include_directories(simple_l2_orderbook
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# L3 OrderBook Example
add_executable(simple_l3_orderbook
    simple_l3_orderbook.cpp
)

target_link_libraries(simple_l3_orderbook
    PRIVATE
        slick::orderbook
)

target_include_directories(simple_l3_orderbook
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Multi-Symbol OrderBook Example
add_executable(multi_symbol_orderbook
    multi_symbol_orderbook.cpp
)

target_link_libraries(multi_symbol_orderbook
    PRIVATE
        slick::orderbook
)

target_include_directories(multi_symbol_orderbook
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

set(SLICK_ORDERBOOK_EXAMPLE_TARGETS simple_l2_orderbook simple_l3_orderbook multi_symbol_orderbook)

# Coinbase integration example (opt-in: pulls in network/crypto dependencies)
if(SLICK_ORDERBOOK_BUILD_COINBASE_EXAMPLE)
    find_package(coinbase-advanced-cpp CONFIG QUIET)
    if(NOT coinbase-advanced-cpp_FOUND)
        include(FetchContent)
        message(STATUS "Fetching coinbase-advanced-cpp...")
        set(BUILD_COINBASE_ADVANCED_TESTS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            coinbase-advanced-cpp
            GIT_REPOSITORY https://github.com/SlickQuant/coinbase-advanced-cpp.git
            GIT_TAG main
        )
        FetchContent_MakeAvailable(coinbase-advanced-cpp)
    endif()

    add_executable(coinbase_integration
        coinbase_integration.cpp
    )

    target_link_libraries(coinbase_integration
        PRIVATE
            slick::orderbook
            slick::coinbase-advanced-cpp
    )

    target_include_directories(coinbase_integration
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
    )

    list(APPEND SLICK_ORDERBOOK_EXAMPLE_TARGETS coinbase_integration)
    message(STATUS "Coinbase integration example enabled")
endif()

# Set output directory for examples
set_target_properties(${SLICK_ORDERBOOK_EXAMPLE_TARGETS}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    LINKER_LANGUAGE CXX
)
