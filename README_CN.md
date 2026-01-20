[English](./README.md) | [中文说明](./README_CN.md)

# c-error

基于线程本地存储的错误码库，支持 53 位结构化错误码。

## 特性

- **纯 C 实现** - 无 C++ 依赖，兼容 C89/C99/C11
- **线程安全** - 每个线程拥有独立的错误存储，使用编译器特定的 TLS 实现
- **跨平台** - 支持 Windows (MSVC)、Linux (GCC/Clang)、macOS
- **结构化错误码** - 53 位错误码包含多个字段：
  - 13 位保留字段（用于未来扩展）
  - 8 位软件 ID（产品/软件标识符）
  - 11 位组件 ID（模块/组件标识符）
  - 5 位状态码（通用状态，兼容 gRPC）
  - 16 位错误码（具体错误编号）
- **零依赖** - 仅头文件宏和内联函数（配合单文件存储定义）
- **源码级集成** - 简单的 CMake 集成函数
- **C++ RAII 支持** - C++ 中自动清理线程本地缓冲区

## 错误码结构

```
位布局（共 53 位）：
┌─────────┬────────────┬────────────┬────────┬──────────┐
│ 保留位  │  软件 ID   │  组件 ID   │ 状态码 │ 错误码   │
│ 13 bits │  8 bits    │  11 bits   │ 5 bits │ 16 bits  │
│ [52:40] │  [39:32]   │  [31:21]   │[20:16] │  [15:0]  │
└─────────┴────────────┴────────────┴────────┴──────────┘
```

### 字段说明

| 字段     | 位范围  | 取值范围   | 描述                      |
|:-------- |:------- |:---------- |:------------------------- |
| 保留位   | [52:40] | 0-8191     | C-1 部分：预留未来使用    |
| 软件 ID  | [39:32] | 0-255      | C-2 部分：软件/产品标识符 |
| 组件 ID  | [31:21] | 0-2047     | C-3 部分：模块/组件标识符 |
| 状态码   | [20:16] | 0-31       | B 部分：通用状态码        |
| 错误码   | [15:0]  | 0-65535    | A 部分：具体错误编号      |

## 集成方式

### 方式 1：include()（推荐）

```cmake
# 包含集成模块
include(path/to/c-error/c_error.cmake)

# 添加到目标
add_executable(your_app main.c)
target_add_c_error(your_app)
```

### 方式 2：add_subdirectory()

```cmake
# 将 c-error 作为子目录添加
add_subdirectory(path/to/c-error)

# 添加到目标
add_executable(your_app main.c)
target_add_c_error(your_app)
```

### 方式 3：手动集成

复制以下文件到你的项目：
- `include/c-error/lasterror.h`
- `src/lasterror.c`

```cmake
add_executable(your_app
    main.c
    path/to/lasterror.c
)
target_include_directories(your_app PRIVATE path/to/include)
```

## C++ 集成

对于 C++ 应用程序，库提供了封装头文件 `lasterror.hpp`。该文件包含一个 RAII 辅助类，可以在线程退出时自动清理线程本地缓冲区，无需手动调用 `cerror_cleanup_thread_local_buffer()`。

```cpp
#include <c-error/lasterror.hpp>

void myFunction() {
    // 1. 基本用法
    Chameleon::setLastError(LEON_MAKE_ERROR_CODE(1, 2, 3, 4));

    // 2. 使用 std::string (自动拷贝 COPY)
    std::string dynamicMsg = "Resource " + name + " not found";
    Chameleon::setLastError(LEON_MAKE_ERROR_CODE(1, 2, 3, 5), dynamicMsg);

    // 3. 使用字符数组 (自动拷贝 COPY)
    char buf[64];
    snprintf(buf, sizeof(buf), "Error at %p", ptr);
    Chameleon::setLastError(LEON_MAKE_ERROR_CODE(1, 2, 3, 6), buf);

    // 4. 使用字符串字面量 (自动零拷贝 NO-COPY)
    // 注意：const char(&)[N] 重载假定字符串具有静态生命周期
    Chameleon::setLastError(LEON_MAKE_ERROR_CODE(1, 2, 3, 7), "Static error message");
}
// 线程退出时缓冲区会自动清理
```

**注意：** 您必须使用 `Chameleon::` 封装函数（如 `Chameleon::setLastError`）或手动引用 `Chameleon::g_errorHelper` 才能激活当前线程的自动清理机制。

### 便捷重载接口 (C++)

`Chameleon` 命名空间提供了多个 `setLastError` 重载以提升易用性：

| 重载签名 | 行为 |
|:--------- |:--------- |
| `setLastError(uint64_t)` | 仅设置错误码 |
| `setLastError(uint64_t, const std::string&)` | 设置错误码并**拷贝**字符串内容 |
| `setLastError(uint64_t, char (&)[N])` | 设置错误码并**拷贝**数组内容 |
| `setLastError(uint64_t, const char (&)[N])` | 设置错误码并使用指针（**不拷贝**，仅限静态字面量！） |


C++ 头文件还为所有标准宏提供了 `LEON_` 前缀的别名，以便在需要时提供一致的命名约定：

- `LEON_MAKE_ERROR_CODE`
- `LEON_GET_ERROR_CODE`
- `LEON_IS_VALID_ERROR_CODE`
- ...以及所有位字段定义和常量。

## 快速入门

### 基本用法 (C API)

```c
#include <c-error/lasterror.h>
#include <stdio.h>

int processData(const char* data) {
    if (data == NULL) {
        cerror_set_last(MAKE_ERROR_CODE(0x01, 0x10, 0x03, 0x0001));
        return 0;  /* 失败 */
    }

    /* 处理数据... */
    cerror_clear_last();
    return 1;  /* 成功 */
}

int main(void) {
    if (!processData(NULL)) {
        uint64_t err = cerror_get_last();
        printf("错误: 0x%llX\n", (unsigned long long)err);
        printf("  软件 ID: %u\n", cerror_get_last_software_id());
        printf("  组件 ID: %u\n", cerror_get_last_component_id());
        printf("  状态码: %u\n", cerror_get_last_status());
        printf("  错误码: %u\n", cerror_get_last_code());
    }

    /* 线程退出前清理 */
    cerror_cleanup_thread_local_buffer();
    return 0;
}
```

### 构造错误码

```c
/* 完整的 53 位错误码 */
uint64_t err = MAKE_ERROR_CODE_53(
    0x0,     /* 保留位 */
    0x42,    /* 软件 ID */
    0x123,   /* 组件 ID */
    0x03,    /* 状态码 */
    0x5678   /* 错误码 */
);

/* 不带保留位（最常用） */
uint64_t err = MAKE_ERROR_CODE(0x01, 0x10, 0x05, 0x1234);

/* 32 位风格（无软件 ID） */
uint64_t err = MAKE_ERROR_CODE_32(0x10, 0x05, 0x1234);
```

### 提取错误字段

```c
uint64_t err = cerror_get_last();

uint16_t errorCode = GET_ERROR_CODE(err);
uint8_t status = GET_STATUS(err);
uint16_t componentId = GET_COMPONENT_ID(err);
uint8_t softwareId = GET_SOFTWARE_ID(err);

/* 或使用便捷函数 */
uint16_t errorCode = cerror_get_last_code();
uint8_t status = cerror_get_last_status();
uint16_t componentId = cerror_get_last_component_id();
uint8_t softwareId = cerror_get_last_software_id();
```

### 错误信息字符串

```c
/* 设置错误及常量字符串（不拷贝） */
cerror_set_last_info(MAKE_ERROR_CODE(1, 2, 3, 4), "File not found");

/* 设置错误及拷贝字符串（用于动态字符串） */
char msg[64];
snprintf(msg, sizeof(msg), "Failed at line %d", lineNum);
cerror_set_last_info_copy(MAKE_ERROR_CODE(1, 2, 3, 5), msg);

/* 获取错误信息 */
const char* info = cerror_get_last_info();
printf("错误信息: %s\n", info);
```

## API 参考 (C)

### 函数

#### 错误管理

| 函数 | 描述 |
|:---- |:---- |
| `cerror_set_last(uint64_t)` | 设置错误码 |
| `cerror_get_last()` | 获取错误码 |
| `cerror_clear_last()` | 清除错误码 |
| `cerror_set_last_info(uint64_t, const char*)` | 设置错误及常量字符串 |
| `cerror_set_last_info_copy(uint64_t, const char*)` | 设置错误及拷贝字符串 |
| `cerror_get_last_info()` | 获取错误信息字符串 |
| `cerror_cleanup_thread_local_buffer()` | 线程退出前释放动态缓冲区 |

#### 字段提取

| 函数 | 描述 |
|:---- |:---- |
| `cerror_get_last_code()` | 获取错误码字段（16 位） |
| `cerror_get_last_status()` | 获取状态码字段（5 位） |
| `cerror_get_last_component_id()` | 获取组件 ID（11 位） |
| `cerror_get_last_software_id()` | 获取软件 ID（8 位） |

#### 状态码工具

| 函数 | 描述 |
|:---- |:---- |
| `cerror_get_status_code_string(CErrorStatusCode)` | 获取状态码字符串 |
| `cerror_grpc_status_to_http_status(CErrorStatusCode)` | 将 gRPC 状态转为 HTTP |
| `cerror_code_to_http_status(uint64_t)` | 将错误码转为 HTTP 状态 |

### 宏

#### 构造

| 宏 | 描述 |
|:-- |:---- |
| `MAKE_ERROR_CODE_53(reserved, softwareId, componentId, status, errorCode)` | 完整 53 位 |
| `MAKE_ERROR_CODE(softwareId, componentId, status, errorCode)` | 不带保留位 |
| `MAKE_ERROR_CODE_32(componentId, status, errorCode)` | 32 位风格 |

#### 提取

| 宏 | 描述 |
|:-- |:---- |
| `GET_ERROR_CODE(err)` | 提取错误码（16 位） |
| `GET_STATUS(err)` | 提取状态码（5 位） |
| `GET_COMPONENT_ID(err)` | 提取组件 ID（11 位） |
| `GET_SOFTWARE_ID(err)` | 提取软件 ID（8 位） |

#### 测试

| 宏 | 描述 |
|:-- |:---- |
| `IS_VALID_ERROR_CODE(err)` | 检查是否在有效的 53 位范围内 |

## 平台支持

| 平台          | 编译器 | TLS 关键字           |
|:------------- |:------ |:-------------------- |
| Windows       | MSVC   | `__declspec(thread)` |
| Windows       | MinGW  | `__thread`           |
| Linux         | GCC    | `__thread`           |
| Linux         | Clang  | `__thread`           |
| macOS         | Clang  | `__thread`           |
| Android NDK   | Clang  | `__thread`           |
| iOS           | Clang  | `__thread`           |

## 构建测试/示例

```bash
mkdir build && cd build
cmake .. -DC_ERROR_BUILD_TESTS=ON -DC_ERROR_BUILD_EXAMPLES=ON
cmake --build .
ctest
```

### 构建选项

| 选项                     | 默认值 | 描述               |
|:------------------------ |:------ |:------------------ |
| `C_ERROR_BUILD_TESTS`    | OFF    | 构建测试程序       |
| `C_ERROR_BUILD_EXAMPLES` | OFF    | 构建示例程序       |

## 线程安全

每个线程维护自己独立的错误码和缓冲区。

```c
/* 线程 A */
cerror_set_last(MAKE_ERROR_CODE(1, 2, 3, 0x1234));
printf("线程 A: %llX\n", cerror_get_last());

/* 线程 B */
cerror_set_last(MAKE_ERROR_CODE(3, 4, 5, 0x5678));
printf("线程 B: %llX\n", cerror_get_last());

/* 每个线程必须清理自己的缓冲区 */
cerror_cleanup_thread_local_buffer();
```

## 许可证

MIT License

## 版本历史

- **2.0.0** - 重构 C API
  - 为 C 函数添加命名空间（`cerror_` 前缀）以避免冲突
  - 移除 `lasterror.cpp`（现在大部分为内联函数）
  - 添加 HTTP/gRPC 状态码工具
- **1.0.0** - 初始版本
  - 线程本地错误存储
  - 53 位结构化错误码
  - 跨平台支持（MSVC、GCC、Clang）
  - 源码级 CMake 集成