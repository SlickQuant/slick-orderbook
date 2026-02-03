# Test configuration for slick-orderbook
cmake_minimum_required(VERSION 3.20)

# Find Google Test
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # If not found, fetch it from GitHub
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Enable testing
enable_testing()

# Test executable for unit tests
add_executable(slick_orderbook_tests
    unit/test_intrusive_list.cpp
    unit/test_memory_pool.cpp
    unit/test_orderbook_l2.cpp
    unit/test_orderbook_l3.cpp
    unit/test_orderbook_manager.cpp
)

target_link_libraries(slick_orderbook_tests
    PRIVATE
        slick::orderbook
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(slick_orderbook_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include_private
)

# Set linker language to C++ (needed for LTO with header-only mode)
set_target_properties(slick_orderbook_tests
    PROPERTIES
    LINKER_LANGUAGE CXX
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(slick_orderbook_tests)

# Add test target
add_custom_target(run_tests
    COMMAND slick_orderbook_tests
    DEPENDS slick_orderbook_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests"
)
