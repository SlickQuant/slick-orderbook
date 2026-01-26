# Copyright 2026 Slick Quant LLC
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
        ${CMAKE_SOURCE_DIR}/include_private
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
        ${CMAKE_SOURCE_DIR}/include_private
)

# Set output directory for examples
set_target_properties(simple_l2_orderbook simple_l2_orderbook
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

# # Install examples (optional)
# install(TARGETS simple_l2_orderbook simple_l2_orderbook
#     RUNTIME DESTINATION examples
# )

set_target_properties(simple_l3_orderbook simple_l3_orderbook
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

# # Install examples (optional)
# install(TARGETS simple_l3_orderbook simple_l3_orderbook
#     RUNTIME DESTINATION examples
# )
