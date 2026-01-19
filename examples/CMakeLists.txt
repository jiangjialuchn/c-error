# c-error Examples

# Basic usage example
add_executable(basic_usage basic_usage.c)
target_link_libraries(basic_usage PRIVATE c-error::c-error)

# Set C standard
set_target_properties(basic_usage PROPERTIES C_STANDARD 11)

message(STATUS "c-error examples configured")
