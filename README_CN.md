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
- **零依赖** - 仅头文件宏 + 单文件实现
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

对于 C++ 应用程序，库提供了封装头文件 `lasterror.hpp`。该文件包含一个 RAII 辅助类，可以在线程退出时自动清理线程本地缓冲区，无需手动调用 `cleanupThreadLocalErrorBuffer()`。

```cpp
#include <c-error/lasterror.hpp>

void myFunction() {
    // 使用 Chameleon 命名空间下的封装函数以确保自动清理
    Chameleon::setLastErrorInfoCopy(
        LEON_MAKE_ERROR_CODE(1, 2, 3, 4), 
        "Dynamic error message"
    );
}
// 线程退出时缓冲区会自动清理
```

**注意：** 您必须使用 `Chameleon::` 封装函数（如 `Chameleon::setLastErrorInfoCopy`）或手动引用 `Chameleon::g_errorHelper` 才能激活当前线程的自动清理机制。

### 带前缀的宏

C++ 头文件还为所有标准宏提供了 `LEON_` 前缀的别名，以便在需要时提供一致的命名约定：

- `LEON_MAKE_ERROR_CODE`
- `LEON_GET_ERROR_CODE`
- `LEON_IS_VALID_ERROR_CODE`
- ...以及所有位字段定义和常量。

## 快速入门

### 基本用法

```c
#include <c-error/lasterror.h>
#include <stdio.h>

int processData(const char* data) {
    if (data == NULL) {
        setLastError(MAKE_ERROR_CODE(0x01, 0x10, 0x03, 0x0001));
        return 0;  /* 失败 */
    }

    /* 处理数据... */
    clearLastError();
    return 1;  /* 成功 */
}

int main(void) {
    if (!processData(NULL)) {
        uint64_t err = getLastError();
        printf("错误: 0x%llX\n", (unsigned long long)err);
        printf("  软件 ID: %u\n", getLastSoftwareId());
        printf("  组件 ID: %u\n", getLastComponentId());
        printf("  状态码: %u\n", getLastStatus());
        printf("  错误码: %u\n", getLastErrorCode());
    }

    /* 线程退出前清理 */
    cleanupThreadLocalErrorBuffer();
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
uint64_t err = getLastError();

uint16_t errorCode = GET_ERROR_CODE(err);
uint8_t status = GET_STATUS(err);
uint16_t componentId = GET_COMPONENT_ID(err);
uint8_t softwareId = GET_SOFTWARE_ID(err);

/* 或使用便捷函数 */
uint16_t errorCode = getLastErrorCode();
uint8_t status = getLastStatus();
uint16_t componentId = getLastComponentId();
uint8_t softwareId = getLastSoftwareId();
```

### 错误信息字符串

```c
/* 设置错误及常量字符串（不拷贝） */
setLastErrorInfo(MAKE_ERROR_CODE(1, 2, 3, 4), "File not found");

/* 设置错误及拷贝字符串（用于动态字符串） */
char msg[64];
snprintf(msg, sizeof(msg), "Failed at line %d", lineNum);
setLastErrorInfoCopy(MAKE_ERROR_CODE(1, 2, 3, 5), msg);

/* 获取错误信息 */
const char* info = getLastErrorInfo();
printf("错误信息: %s\n", info);
```

## API 参考

### 函数

#### 错误管理

| 函数 | 描述 |
|:---- |:---- |
| `setLastError(uint64_t)` | 设置错误码 |
| `getLastError()` | 获取错误码 |
| `clearLastError()` | 清除错误码 |
| `setLastErrorInfo(uint64_t, const char*)` | 设置错误及常量字符串 |
| `setLastErrorInfoCopy(uint64_t, const char*)` | 设置错误及拷贝字符串 |
| `getLastErrorInfo()` | 获取错误信息字符串 |
| `cleanupThreadLocalErrorBuffer()` | 线程退出前释放动态缓冲区 |

#### 字段提取

| 函数 | 描述 |
|:---- |:---- |
| `getLastErrorCode()` | 获取错误码字段（16 位） |
| `getLastStatus()` | 获取状态码字段（5 位） |
| `getLastComponentId()` | 获取组件 ID（11 位） |
| `getLastSoftwareId()` | 获取软件 ID（8 位） |

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
setLastError(MAKE_ERROR_CODE(1, 2, 3, 0x1234));
printf("线程 A: %llX\n", getLastError());

/* 线程 B */
setLastError(MAKE_ERROR_CODE(3, 4, 5, 0x5678));
printf("线程 B: %llX\n", getLastError());

/* 每个线程必须清理自己的缓冲区 */
cleanupThreadLocalErrorBuffer();
```

## 许可证

MIT License

## 版本历史

- **1.0.0** - 初始版本
  - 线程本地错误存储
  - 53 位结构化错误码
  - 跨平台支持（MSVC、GCC、Clang）
  - 源码级 CMake 集成
