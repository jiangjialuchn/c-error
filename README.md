[English](./README.md) | [中文说明](./README_CN.md)

# c-error

Thread-local error code storage library with 53-bit structured error codes.

## Features

- **Pure C** - No C++ dependencies, compatible with C89/C99/C11
- **Thread-safe** - Each thread has independent error storage using compiler-specific TLS
- **Cross-platform** - Works on Windows (MSVC), Linux (GCC/Clang), macOS
- **Structured Error Codes** - 53-bit error codes with multiple fields:
  - 13-bit Reserved field (future extensions)
  - 8-bit Software ID (product/software identifier)
  - 11-bit Component ID (module/component identifier)
  - 5-bit Status Code (general status, gRPC-compatible)
  - 16-bit Error Code (specific error number)
- **Zero Dependencies** - Header-only macros and inline functions (with single-file storage definition)
- **Source-level Integration** - Simple CMake integration function
- **C++ RAII Support** - Automatic cleanup of thread-local buffers in C++

## Error Code Structure

```
Bit Layout (53 bits total):
┌─────────┬────────────┬────────────┬────────┬──────────┐
│Reserved │Software ID │Component ID│ Status │Error Code│
│ 13 bits │  8 bits    │  11 bits   │ 5 bits │ 16 bits  │
│ [52:40] │  [39:32]   │  [31:21]   │[20:16] │  [15:0]  │
└─────────┴────────────┴────────────┴────────┴──────────┘
```

### Field Descriptions

| Field        | Bits    | Range      | Description                           |
|:------------- |:------- |:---------- |:------------------------------------- |
| Reserved     | [52:40] | 0-8191     | Part C-1: Reserved for future use     |
| Software ID  | [39:32] | 0-255      | Part C-2: Software/Product identifier |
| Component ID | [31:21] | 0-2047     | Part C-3: Module/Component identifier |
| Status       | [20:16] | 0-31       | Part B: General status code           |
| Error Code   | [15:0]  | 0-65535    | Part A: Specific error number         |

## Integration

### Method 1: include() (Recommended)

```cmake
# Include the integration module
include(path/to/c-error/c_error.cmake)

# Add to your target
add_executable(your_app main.c)
target_add_c_error(your_app)
```

### Method 2: add_subdirectory()

```cmake
# Add c-error as subdirectory
add_subdirectory(path/to/c-error)

# Add to your target
add_executable(your_app main.c)
target_add_c_error(your_app)
```

### Method 3: Manual Integration

Copy the following files to your project:
- `include/c-error/lasterror.h`
- `src/lasterror.c`

```cmake
add_executable(your_app
    main.c
    path/to/lasterror.c
)
target_include_directories(your_app PRIVATE path/to/include)
```

## C++ Integration

For C++ applications, a wrapper header `lasterror.hpp` is provided. It includes an RAII helper that automatically cleans up thread-local buffers when the thread exits, eliminating the need to manually call `cerror_cleanup_thread_local_buffer()`.

```cpp
#include <c-error/lasterror.hpp>

void myFunction() {
    // 1. Basic usage
    Chameleon::setLastError(LEON_MAKE_ERROR_CODE(1, 2, 3, 4));

    // 2. With dynamic string (Automatic COPY)
    std::string dynamicMsg = "Resource " + name + " not found";
    Chameleon::setLastError(LEON_MAKE_ERROR_CODE(1, 2, 3, 5), dynamicMsg);

    // 3. With char array (Automatic COPY)
    char buf[64];
    snprintf(buf, sizeof(buf), "Error at %p", ptr);
    Chameleon::setLastError(LEON_MAKE_ERROR_CODE(1, 2, 3, 6), buf);

    // 4. With string literal (Automatic NO-COPY / Zero-copy)
    // Note: Overload for const char(&)[N] assumes static lifetime
    Chameleon::setLastError(LEON_MAKE_ERROR_CODE(1, 2, 3, 7), "Static error message");
}
// Buffer automatically cleaned up when thread exits
```

**Note:** You must use the `Chameleon::` wrappers (e.g., `Chameleon::setLastError`) or manually reference `Chameleon::g_errorHelper` to activate the automatic cleanup mechanism for the current thread.

### Convenience Overloads (C++)

The `Chameleon` namespace provides several `setLastError` overloads for better ergonomics:

| Overload | Behavior |
|:--------- |:--------- |
| `setLastError(uint64_t)` | Sets error code only |
| `setLastError(uint64_t, const std::string&)` | Sets error code and **copies** string content |
| `setLastError(uint64_t, char (&)[N])` | Sets error code and **copies** array content |
| `setLastError(uint64_t, const char (&)[N])` | Sets error code and uses pointer **without copy** (Static literals only!) |


The C++ header also provides `LEON_` prefixed aliases for all standard macros, offering a consistent naming convention if desired:

- `LEON_MAKE_ERROR_CODE`
- `LEON_GET_ERROR_CODE`
- `LEON_IS_VALID_ERROR_CODE`
- ...and so on for all bit fields and definitions.

## Quick Start

### Basic Usage (C API)

```c
#include <c-error/lasterror.h>
#include <stdio.h>

int processData(const char* data) {
    if (data == NULL) {
        cerror_set_last(MAKE_ERROR_CODE(0x01, 0x10, 0x03, 0x0001));
        return 0;  /* failure */
    }

    /* Process data... */
    cerror_clear_last();
    return 1;  /* success */
}

int main(void) {
    if (!processData(NULL)) {
        uint64_t err = cerror_get_last();
        printf("Error: 0x%llX\n", (unsigned long long)err);
        printf("  Software ID: %u\n", cerror_get_last_software_id());
        printf("  Component ID: %u\n", cerror_get_last_component_id());
        printf("  Status: %u\n", cerror_get_last_status());
        printf("  Error Code: %u\n", cerror_get_last_code());
    }

    /* Cleanup before thread exit */
    cerror_cleanup_thread_local_buffer();
    return 0;
}
```

### Constructing Error Codes

```c
/* Full 53-bit error code */
uint64_t err = MAKE_ERROR_CODE_53(
    0x0,     /* reserved */
    0x42,    /* software ID */
    0x123,   /* component ID */
    0x03,    /* status */
    0x5678   /* error code */
);

/* Without reserved field (most common) */
uint64_t err = MAKE_ERROR_CODE(0x01, 0x10, 0x05, 0x1234);

/* 32-bit style (no software ID) */
uint64_t err = MAKE_ERROR_CODE_32(0x10, 0x05, 0x1234);
```

### Extracting Error Fields

```c
uint64_t err = cerror_get_last();

uint16_t errorCode = GET_ERROR_CODE(err);
uint8_t status = GET_STATUS(err);
uint16_t componentId = GET_COMPONENT_ID(err);
uint8_t softwareId = GET_SOFTWARE_ID(err);

/* Or use convenience functions */
uint16_t errorCode = cerror_get_last_code();
uint8_t status = cerror_get_last_status();
uint16_t componentId = cerror_get_last_component_id();
uint8_t softwareId = cerror_get_last_software_id();
```

### Error Info String

```c
/* Set error with constant string (no copy) */
cerror_set_last_info(MAKE_ERROR_CODE(1, 2, 3, 4), "File not found");

/* Set error with copied string (for dynamic strings) */
char msg[64];
snprintf(msg, sizeof(msg), "Failed at line %d", lineNum);
cerror_set_last_info_copy(MAKE_ERROR_CODE(1, 2, 3, 5), msg);

/* Get error info */
const char* info = cerror_get_last_info();
printf("Error info: %s\n", info);
```

## API Reference (C)

### Functions

#### Error Management

| Function | Description |
|:-------- |:----------- |
| `cerror_set_last(uint64_t)` | Set error code |
| `cerror_get_last()` | Get error code |
| `cerror_clear_last()` | Clear error code |
| `cerror_set_last_info(uint64_t, const char*)` | Set error with constant string |
| `cerror_set_last_info_copy(uint64_t, const char*)` | Set error with copied string |
| `cerror_get_last_info()` | Get error info string |
| `cerror_cleanup_thread_local_buffer()` | Free dynamic buffer before thread exit |

#### Field Extraction

| Function | Description |
|:-------- |:----------- |
| `cerror_get_last_code()` | Get error code field (16 bits) |
| `cerror_get_last_status()` | Get status field (5 bits) |
| `cerror_get_last_component_id()` | Get component ID (11 bits) |
| `cerror_get_last_software_id()` | Get software ID (8 bits) |

#### Status Utilities

| Function | Description |
|:-------- |:----------- |
| `cerror_get_status_code_string(CErrorStatusCode)` | Get string for status code |
| `cerror_grpc_status_to_http_status(CErrorStatusCode)` | Convert gRPC status to HTTP |
| `cerror_code_to_http_status(uint64_t)` | Convert error code to HTTP status |

### Macros

#### Construction

| Macro | Description |
|:----- |:----------- |
| `MAKE_ERROR_CODE_53(reserved, softwareId, componentId, status, errorCode)` | Full 53-bit |
| `MAKE_ERROR_CODE(softwareId, componentId, status, errorCode)` | Without reserved |
| `MAKE_ERROR_CODE_32(componentId, status, errorCode)` | 32-bit style |

#### Extraction

| Macro | Description |
|:----- |:----------- |
| `GET_ERROR_CODE(err)` | Extract error code (16 bits) |
| `GET_STATUS(err)` | Extract status (5 bits) |
| `GET_COMPONENT_ID(err)` | Extract component ID (11 bits) |
| `GET_SOFTWARE_ID(err)` | Extract software ID (8 bits) |

#### Testing

| Macro | Description |
|:----- |:----------- |
| `IS_VALID_ERROR_CODE(err)` | Check if within valid 53-bit range |

## Platform Support

| Platform      | Compiler | TLS Keyword         |
|:------------- |:-------- |:------------------- |
| Windows       | MSVC     | `__declspec(thread)` |
| Windows       | MinGW    | `__thread`          |
| Linux         | GCC      | `__thread`          |
| Linux         | Clang    | `__thread`          |
| macOS         | Clang    | `__thread`          |
| Android NDK   | Clang    | `__thread`          |
| iOS           | Clang    | `__thread`          |

## Building Tests/Examples

```bash
mkdir build && cd build
cmake .. -DC_ERROR_BUILD_TESTS=ON -DC_ERROR_BUILD_EXAMPLES=ON
cmake --build .
ctest
```

### Build Options

| Option                   | Default | Description              |
|:------------------------ |:------- |:------------------------ |
| `C_ERROR_BUILD_TESTS`    | OFF     | Build test programs      |
| `C_ERROR_BUILD_EXAMPLES` | OFF     | Build example programs   |

## Thread Safety

Each thread maintains its own independent error code and buffer.

```c
/* Thread A */
cerror_set_last(MAKE_ERROR_CODE(1, 2, 3, 0x1234));
printf("Thread A: %llX\n", cerror_get_last());

/* Thread B */
cerror_set_last(MAKE_ERROR_CODE(3, 4, 5, 0x5678));
printf("Thread B: %llX\n", cerror_get_last());

/* Each thread must cleanup its own buffer */
cerror_cleanup_thread_local_buffer();
```

## License

MIT License

## Version History

- **2.0.0** - Refactored C API
  - Namespaced C functions (`cerror_` prefix) to avoid collisions
  - Removed `lasterror.cpp` (now mostly header-only with inline functions)
  - Added HTTP/gRPC status code utilities
- **1.0.0** - Initial release
  - Thread-local error storage
  - 53-bit structured error codes
  - Cross-platform support (MSVC, GCC, Clang)
  - Source-level CMake integration