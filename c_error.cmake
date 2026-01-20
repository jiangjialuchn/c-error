# c-error - Source-level Integration Module
# Usage: include(path/to/c_error.cmake)
#        target_add_c_error(your_target)

# Integration function for source-level inclusion
# Usage: target_add_c_error(your_target)
set(_C_ERROR_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(target_add_c_error target)
    # Define source directory (works from any location)
    set(C_ERROR_SOURCE_DIR "${_C_ERROR_BASE_DIR}")

    # Add source files to target
    target_sources(${target} PRIVATE
        "${C_ERROR_SOURCE_DIR}/src/lasterror.c"
    )

    # Add include directories (BUILD_INTERFACE to avoid source path in install exports)
    target_include_directories(${target} PUBLIC
        "$<BUILD_INTERFACE:${C_ERROR_SOURCE_DIR}/include>"
    )

    # Thread library (required for thread-local storage on some platforms)
    if(NOT WIN32)
        find_package(Threads QUIET)
        if(Threads_FOUND)
            target_link_libraries(${target} PUBLIC Threads::Threads)
        endif()
    endif()

    message(STATUS "[${target}] Added c-error sources")
endfunction()
