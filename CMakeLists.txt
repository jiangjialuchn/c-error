# c-error - Thread-local Error Code Storage (Pure C, Cross-platform)
cmake_minimum_required(VERSION 3.16)

project(c-error
    VERSION 1.0.0
    LANGUAGES C
    DESCRIPTION "Thread-local error code storage with 53-bit structured error codes"
)

# ============================================================================
# Build Options
# ============================================================================

option(C_ERROR_BUILD_TESTS "Build c-error tests" OFF)
option(C_ERROR_BUILD_EXAMPLES "Build c-error examples" OFF)

# ============================================================================
# Include Integration Function
# ============================================================================

include(${CMAKE_CURRENT_SOURCE_DIR}/c_error.cmake)

# ============================================================================
# Tests
# ============================================================================

if(C_ERROR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# Examples
# ============================================================================

if(C_ERROR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "c-error configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Integration: Source-level (use target_add_c_error)")
message(STATUS "  Build Tests: ${C_ERROR_BUILD_TESTS}")
message(STATUS "  Build Examples: ${C_ERROR_BUILD_EXAMPLES}")
