cmake_minimum_required(VERSION 3.20)
project(slick-orderbook VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(SLICK_ORDERBOOK_HEADER_ONLY "Build as header-only library" OFF)
option(SLICK_ORDERBOOK_BUILD_SHARED "Build shared library" OFF)
option(SLICK_ORDERBOOK_BUILD_TESTS "Build tests" ON)
option(SLICK_ORDERBOOK_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(SLICK_ORDERBOOK_BUILD_EXAMPLES "Build examples" ON)
option(SLICK_ORDERBOOK_BUILD_COINBASE_EXAMPLE "Build the Coinbase integration example (fetches coinbase-advanced-cpp; needs nlohmann-json, OpenSSL, Boost.Beast, jwt-cpp)" OFF)
option(SLICK_ORDERBOOK_ENABLE_LTO "Enable LTO/IPO in Release builds" ON)
option(SLICK_ORDERBOOK_ENABLE_NATIVE_ARCH "Tune Release builds for the build machine (-march=native, GCC/Clang)" ON)

# Optimization flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O3>
        $<$<AND:$<CONFIG:Release>,$<BOOL:${SLICK_ORDERBOOK_ENABLE_NATIVE_ARCH}>>:-march=native>
        $<$<AND:$<CONFIG:Release>,$<BOOL:${SLICK_ORDERBOOK_ENABLE_LTO}>>:-flto>
    )
elseif(MSVC)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_compile_options(
        /W4
        /wd4324  # Suppress C4324: structure was padded due to alignment specifier (intentional for cache alignment)
        /wd4996
        /std:c++latest  # Enable C++23 features including flat_map
        $<$<CONFIG:Release>:/O2>
        $<$<AND:$<CONFIG:Release>,$<BOOL:${SLICK_ORDERBOOK_ENABLE_LTO}>>:/GL>  # Whole program optimization
    )
    add_link_options(
        $<$<AND:$<CONFIG:Release>,$<BOOL:${SLICK_ORDERBOOK_ENABLE_LTO}>>:/LTCG>  # Link-time code generation (required with /GL)
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

# Detail headers (now public)
set(SLICK_ORDERBOOK_DETAIL_HEADERS
    include/slick/orderbook/detail/price_level_l2.hpp
    include/slick/orderbook/detail/price_level_l3.hpp
    include/slick/orderbook/detail/order.hpp
    include/slick/orderbook/detail/flat_map.hpp
    include/slick/orderbook/detail/intrusive_list.hpp
    include/slick/orderbook/detail/memory_pool.hpp
    include/slick/orderbook/detail/level_container.hpp
    include/slick/orderbook/detail/order_map.hpp
)

# Generate version header from the project version (single source of truth)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/slick/orderbook/version.hpp"
    @ONLY
    NEWLINE_STYLE LF
)

# Library target
if(SLICK_ORDERBOOK_HEADER_ONLY)
    # Header-only library
    add_library(slick-orderbook INTERFACE)
    set(SLICK_ORDERBOOK_INCLUDE_SCOPE INTERFACE)
    target_compile_definitions(slick-orderbook INTERFACE SLICK_ORDERBOOK_HEADER_ONLY)

    # Fix for GCC/Clang with LTO: explicitly link C++ standard library
    # LTO can cause the linker to lose track of C++ stdlib dependency
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_libraries(slick-orderbook INTERFACE stdc++)
    endif()
else()
    # Compiled library (static or shared)
    if(SLICK_ORDERBOOK_BUILD_SHARED)
        add_library(slick-orderbook SHARED ${SLICK_ORDERBOOK_SOURCES})
        target_compile_definitions(slick-orderbook
            PUBLIC SLICK_ORDERBOOK_SHARED   # Selects dllimport/visibility in SLICK_API for consumers
            PRIVATE SLICK_ORDERBOOK_BUILD
        )
        # The ABI may change between minor versions, so the soname tracks MAJOR.MINOR
        set_target_properties(slick-orderbook PROPERTIES
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        )
    else()
        add_library(slick-orderbook STATIC ${SLICK_ORDERBOOK_SOURCES})
    endif()
    set(SLICK_ORDERBOOK_INCLUDE_SCOPE PUBLIC)
endif()

target_include_directories(slick-orderbook ${SLICK_ORDERBOOK_INCLUDE_SCOPE}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Public headers require C++23 (std::flat_map); CMAKE_CXX_STANDARD is not transitive to consumers
target_compile_features(slick-orderbook ${SLICK_ORDERBOOK_INCLUDE_SCOPE} cxx_std_23)

# Add alias for consistent usage; installed/exported target is also slick::orderbook
add_library(slick::orderbook ALIAS slick-orderbook)
set_target_properties(slick-orderbook PROPERTIES EXPORT_NAME orderbook)

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

    # Doxygen API documentation
    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            @ONLY
        )
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen (version ${PROJECT_VERSION})"
            VERBATIM
        )
    endif()
endif()

# Installation rules
# All rules belong to one component so packaging can install just this library
# (`cmake --install <build> --component slick-orderbook`), excluding anything fetched dependencies install
set(SLICK_ORDERBOOK_INSTALL_COMPONENT slick-orderbook)

install(TARGETS slick-orderbook
    EXPORT slick-orderbook-targets
    LIBRARY DESTINATION lib COMPONENT ${SLICK_ORDERBOOK_INSTALL_COMPONENT}
    ARCHIVE DESTINATION lib COMPONENT ${SLICK_ORDERBOOK_INSTALL_COMPONENT}
    RUNTIME DESTINATION bin COMPONENT ${SLICK_ORDERBOOK_INSTALL_COMPONENT}
)

install(DIRECTORY include/
    DESTINATION include
    COMPONENT ${SLICK_ORDERBOOK_INSTALL_COMPONENT}
    FILES_MATCHING PATTERN "*.hpp"
)

set(SLICK_ORDERBOOK_CMAKE_INSTALL_DIR lib/cmake/slick-orderbook)

install(EXPORT slick-orderbook-targets
    FILE slick-orderbook-targets.cmake
    NAMESPACE slick::
    DESTINATION ${SLICK_ORDERBOOK_CMAKE_INSTALL_DIR}
    COMPONENT ${SLICK_ORDERBOOK_INSTALL_COMPONENT}
)

# Package configuration
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/slick-orderbook-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/slick-orderbook-config.cmake"
    INSTALL_DESTINATION ${SLICK_ORDERBOOK_CMAKE_INSTALL_DIR}
)
# Compiled library: ABI is only kept within a minor version (matches the MAJOR.MINOR SOVERSION),
# so find_package(slick-orderbook 1.0) must not resolve to a 1.1 install.
# Header-only: no binary ABI and minor releases stay source compatible, so any 1.x satisfies 1.y <= x.
if(SLICK_ORDERBOOK_HEADER_ONLY)
    set(SLICK_ORDERBOOK_VERSION_COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)
else()
    set(SLICK_ORDERBOOK_VERSION_COMPATIBILITY SameMinorVersion)
endif()
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/slick-orderbook-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ${SLICK_ORDERBOOK_VERSION_COMPATIBILITY}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/slick-orderbook-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/slick-orderbook-config-version.cmake"
    DESTINATION ${SLICK_ORDERBOOK_CMAKE_INSTALL_DIR}
    COMPONENT ${SLICK_ORDERBOOK_INSTALL_COMPONENT}
)
