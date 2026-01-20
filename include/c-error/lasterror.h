/** @file lasterror.h
 *  @brief Thread-local Error Code Storage (Pure C, Cross-platform)
 *
 *  Provides cross-platform thread-local error code storage and retrieval
 *  using platform-specific TLS APIs (Windows TLS and POSIX pthread).
 *
 *  Error Code Format (53-bit):
 *  | Bit Range   | Field            | Length  | Description                    |
 *  |:----------- |:---------------- |:------- |:------------------------------ |
 *  | **[52:40]** | **Reserved**     | 13 bit  | **Part C-1**: Reserved / Future|
 *  | **[39:32]** | **Software ID**  | 8 bit   | **Part C-2**: Software/Product |
 *  | **[31:21]** | **Component ID** | 11 bit  | **Part C-3**: Module/Component |
 *  | **[20:16]** | **Status**       | 5 bit   | **Part B**: General Status     |
 *  | **[15:0]**  | **Error Code**   | 16 bit  | **Part A**: Specific Error     |
 *
 *  @author c-error contributors
 *  @date 2026-01-19
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Bit Field Definitions (53-bit Error Code)
 * ============================================================================ */

/** Bit positions for error code fields */
#define ERROR_CODE_BIT_POS      0u   /**< Error Code starts at bit 0 */
#define STATUS_BIT_POS          16u  /**< Status starts at bit 16 */
#define COMPONENT_BIT_POS       21u  /**< Component ID starts at bit 21 */
#define SOFTWARE_ID_BIT_POS     32u  /**< Software ID starts at bit 32 */
#define RESERVED_BIT_POS        40u  /**< Reserved field starts at bit 40 */

/** Bit field widths */
#define ERROR_CODE_WIDTH        16u  /**< Error Code: 16 bits */
#define STATUS_WIDTH            5u   /**< Status: 5 bits */
#define COMPONENT_WIDTH         11u  /**< Component ID: 11 bits */
#define SOFTWARE_ID_WIDTH       8u   /**< Software ID: 8 bits */
#define RESERVED_WIDTH          13u  /**< Reserved: 13 bits */

/** Bit field masks */
#define ERROR_CODE_MASK         0x000000000000FFFFULL  /**< Bits [15:0] */
#define STATUS_MASK             0x00000000001F0000ULL  /**< Bits [20:16] */
#define COMPONENT_MASK          0x00000000FFE00000ULL  /**< Bits [31:21] */
#define SOFTWARE_ID_MASK        0x000000FF00000000ULL  /**< Bits [39:32] */
#define RESERVED_MASK           0x001FFF0000000000ULL  /**< Bits [52:40] */
#define VALID_ERROR_MASK        0x001FFFFFFFFFFFFFULL  /**< All 53 bits */

/** Maximum values for each field */
#define MAX_ERROR_CODE          0xFFFFu      /**< 16-bit max: 65535 */
#define MAX_STATUS              0x1Fu        /**< 5-bit max: 31 */
#define MAX_COMPONENT           0x7FFu       /**< 11-bit max: 2047 */
#define MAX_SOFTWARE_ID         0xFFu        /**< 8-bit max: 255 */
#define MAX_RESERVED            0x1FFFu      /**< 13-bit max: 8191 */


/* ============================================================================
 * Error Code Construction Macros
 * ============================================================================ */

/**
 * @brief Construct a 53-bit error code from individual fields
 *
 * @param reserved Reserved field (13 bits, Part C-1)
 * @param softwareId Software/Product ID (8 bits, Part C-2)
 * @param componentId Module/Component ID (11 bits, Part C-3)
 * @param status General status code (5 bits, Part B)
 * @param errorCode Specific error code (16 bits, Part A)
 * @return 53-bit error code
 */
#define MAKE_ERROR_CODE_53(reserved, softwareId, componentId, status, errorCode) \
    ((((uint64_t)(reserved) & MAX_RESERVED) << RESERVED_BIT_POS) | \
     (((uint64_t)(softwareId) & MAX_SOFTWARE_ID) << SOFTWARE_ID_BIT_POS) | \
     (((uint64_t)(componentId) & MAX_COMPONENT) << COMPONENT_BIT_POS) | \
     (((uint64_t)(status) & MAX_STATUS) << STATUS_BIT_POS) | \
     ((uint64_t)(errorCode) & MAX_ERROR_CODE))

/**
 * @brief Construct error code without reserved field (most common case)
 */
#define MAKE_ERROR_CODE(softwareId, componentId, status, errorCode) \
    MAKE_ERROR_CODE_53(0u, softwareId, componentId, status, errorCode)

/**
 * @brief Construct a 32-bit error code 
 */
#define MAKE_ERROR_CODE_32(componentId, status, errorCode) \
    MAKE_ERROR_CODE_53(0u, 0u, componentId, status, errorCode)


/* ============================================================================
 * Error Code Field Extraction Macros
 * ============================================================================ */

/**
 * @brief Extract error code field (16 bits, Part A)
 */
#define GET_ERROR_CODE(ullError) \
    ((uint16_t)(((ullError) & ERROR_CODE_MASK) >> ERROR_CODE_BIT_POS))

/**
 * @brief Extract status field (5 bits, Part B)
 */
#define GET_STATUS(ullError) \
    ((uint8_t)(((ullError) & STATUS_MASK) >> STATUS_BIT_POS))

/**
 * @brief Extract component ID field (11 bits, Part C-3)
 */
#define GET_COMPONENT_ID(ullError) \
    ((uint16_t)(((ullError) & COMPONENT_MASK) >> COMPONENT_BIT_POS))

/**
 * @brief Extract software ID field (8 bits, Part C-2)
 */
#define GET_SOFTWARE_ID(ullError) \
    ((uint8_t)(((ullError) & SOFTWARE_ID_MASK) >> SOFTWARE_ID_BIT_POS))

/* ============================================================================
 * Error Code Testing Macros
 * ============================================================================ */

/**
 * @brief Check if error code is within valid 53-bit range
 */
#define IS_VALID_ERROR_CODE(ullError) \
    (((ullError) & ~VALID_ERROR_MASK) == 0ULL)

/* ============================================================================
 * Thread-local Storage Structures
 * ============================================================================ */

/** Initial buffer capacity for dynamic allocation (lazy initialization) */
#define ERROR_INFO_INITIAL_CAPACITY 128

/**
 * @brief Error context structure with dynamic error info buffer
 *
 * The buffer is lazily allocated (starts as NULL) and grows by 2x when needed.
 */
typedef struct ErrorContext
{
    uint64_t    ullLastError;           /**< 53-bit error code + flags */
    const char* pszLastErrorInfo;       /**< Pointer to error info string (may point to external, internal static, or internal dynamic buffer) */
    char*       pszLastErrorInfoBuffer; /**< Dynamically allocated buffer for copied strings (NULL initially) */
    size_t      nBufferCapacity;        /**< Current capacity of the dynamic buffer (0 initially) */
} ErrorContext;

/* ============================================================================
 * Thread-local Storage Declaration
 * ============================================================================ */
/**
 * @brief Thread-local error context variable
 *
 * Uses compiler-specific thread-local storage keywords for zero-overhead access.
 * The buffer (pszLastErrorInfoBuffer) is lazily allocated and must be manually
 * freed before thread exit by calling cerror_cleanup_thread_local_buffer().
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__) 
    /* C11 standard thread-local storage */
    extern _Thread_local ErrorContext g_LastErrorCtx;
#elif defined(_MSC_VER)
    /* Microsoft Visual C++ */
    extern __declspec(thread) ErrorContext g_LastErrorCtx;
#elif defined(__GNUC__) || defined(__clang__)
    /* GCC and Clang */
    extern __thread ErrorContext g_LastErrorCtx;
#else
    #error "Thread-local storage not supported on this compiler"
#endif

/**
 * @brief Cleanup the dynamic buffer in thread-local error context
 *
 * Call this function before thread exit to free the dynamically allocated buffer.
 * Failure to call this function will result in memory leak (buffer only, not the context).
 */
void cerror_cleanup_thread_local_buffer(void);

/* ============================================================================
 * Inline Function Implementations (New C-Style API)
 * ============================================================================ */

/**
 * @brief Set the thread-local last error code
 */
static inline void cerror_set_last(const uint64_t ullError)
{
    /* Store only valid 53-bit error code (mask off upper 11 bits) */
    g_LastErrorCtx.ullLastError = ullError & VALID_ERROR_MASK;
}

/**
 * @brief Get the thread-local last error code
 */
static inline uint64_t cerror_get_last(void)
{
    return g_LastErrorCtx.ullLastError;
}

/**
 * @brief Clear the thread-local last error code
 */
static inline void cerror_clear_last(void)
{
    g_LastErrorCtx.ullLastError = 0ULL;
    g_LastErrorCtx.pszLastErrorInfo = NULL;
    /* Clear buffer to prevent info leakage */
    if (NULL != g_LastErrorCtx.pszLastErrorInfoBuffer)
    {
        g_LastErrorCtx.pszLastErrorInfoBuffer[0] = '\0';
    }
}

/**
 * @brief Get the error code field from last error
 */
static inline uint16_t cerror_get_last_code(void)
{
    return GET_ERROR_CODE(g_LastErrorCtx.ullLastError);
}

/**
 * @brief Get the status field from last error
 */
static inline uint8_t cerror_get_last_status(void)
{
    return GET_STATUS(g_LastErrorCtx.ullLastError);
}

/**
 * @brief Get the component ID field from last error
 */
static inline uint16_t cerror_get_last_component_id(void)
{
    return GET_COMPONENT_ID(g_LastErrorCtx.ullLastError);
}

/**
 * @brief Get the software ID field from last error
 */
static inline uint8_t cerror_get_last_software_id(void)
{
    return GET_SOFTWARE_ID(g_LastErrorCtx.ullLastError);
}

/**
 * @brief Set thread-local error code with constant info string (no copy)
 */
static inline void cerror_set_last_info(const uint64_t ullError, const char* pszErrorInfo)
{
    cerror_set_last(ullError);
    /* Store pointer to constant string (no copy, NULL allowed) */
    g_LastErrorCtx.pszLastErrorInfo = pszErrorInfo;
}

/**
 * @brief Set thread-local error code with info string (copy string content)
 *
 * Uses Small String Optimization (SSO) to avoid allocation for short strings.
 * For longer strings, uses lazy-allocated dynamic buffer with 2x growth strategy.
 */
static inline void cerror_set_last_info_copy(const uint64_t ullError, const char* pszErrorInfo)
{
    if (NULL == pszErrorInfo)
    {
        assert(NULL != pszErrorInfo);
        return;
    }

    cerror_set_last(ullError);

    /* Calculate required capacity (including null terminator) */
    const size_t nLength = strlen(pszErrorInfo);

    /* Fallback: Dynamic allocation for longer strings */
    const size_t nRequiredCapacity = nLength + 1;

    /* Lazy allocation or reallocation if needed */
    if (nRequiredCapacity > g_LastErrorCtx.nBufferCapacity)
    {
        size_t n = nRequiredCapacity;
        // 32-bit hack to round up to next power of 2
        n--; 
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n++;

        size_t nNewCapacity = (n > ERROR_INFO_INITIAL_CAPACITY) ? n : ERROR_INFO_INITIAL_CAPACITY;
        
        char* pNewBuffer = (char*)realloc(g_LastErrorCtx.pszLastErrorInfoBuffer, nNewCapacity);
        assert(pNewBuffer != NULL);

        if (pNewBuffer != NULL)
        {
            g_LastErrorCtx.pszLastErrorInfoBuffer = pNewBuffer;
            g_LastErrorCtx.nBufferCapacity = nNewCapacity;
        }
        else
        {
            /* Allocation failed: keep old buffer (if any), but can't store new message */
            /* In a real system, we might set a fallback "Out of memory" error here */
            return;
        }
    }

    /* Copy string to buffer with null termination */
    memcpy(g_LastErrorCtx.pszLastErrorInfoBuffer, pszErrorInfo, nLength);
    g_LastErrorCtx.pszLastErrorInfoBuffer[nLength] = '\0';

    /* Point to the buffer */
    g_LastErrorCtx.pszLastErrorInfo = g_LastErrorCtx.pszLastErrorInfoBuffer;
}

/**
 * @brief Get the thread-local error info string
 */
static inline const char* cerror_get_last_info(void)
{
    /* Return pointer directly (NULL if no info) */ 
    return NULL == g_LastErrorCtx.pszLastErrorInfo ? "" : g_LastErrorCtx.pszLastErrorInfo;
}

// ============================================================================
// Status Code Utilities
// ============================================================================
/**
 * @brief gRPC-compatible status codes (5-bit: 0-31)
 */
typedef enum CErrorStatusCode {
    CERROR_OK                  = 0,
    CERROR_CANCELLED           = 1,
    CERROR_UNKNOWN             = 2,
    CERROR_INVALID_ARGUMENT    = 3,
    CERROR_DEADLINE_EXCEEDED   = 4,
    CERROR_NOT_FOUND           = 5,
    CERROR_ALREADY_EXISTS      = 6,
    CERROR_PERMISSION_DENIED   = 7,
    CERROR_RESOURCE_EXHAUSTED  = 8,
    CERROR_FAILED_PRECONDITION = 9,
    CERROR_ABORTED             = 10,
    CERROR_OUT_OF_RANGE        = 11,
    CERROR_UNIMPLEMENTED       = 12,
    CERROR_INTERNAL            = 13,
    CERROR_UNAVAILABLE         = 14,
    CERROR_DATA_LOSS           = 15,
    CERROR_UNAUTHENTICATED     = 16,
    CERROR_STATUS_MAX          = 16
} CErrorStatusCode;
/**
 * @brief Get string representation of status code
 */
static inline const char* cerror_get_status_code_string(const CErrorStatusCode statusCode)
{
    switch (statusCode) {
        case CERROR_OK:                  return "OK";
        case CERROR_CANCELLED:           return "CANCELLED";
        case CERROR_UNKNOWN:             return "UNKNOWN";
        case CERROR_INVALID_ARGUMENT:    return "INVALID_ARGUMENT";
        case CERROR_DEADLINE_EXCEEDED:   return "DEADLINE_EXCEEDED";
        case CERROR_NOT_FOUND:           return "NOT_FOUND";
        case CERROR_ALREADY_EXISTS:      return "ALREADY_EXISTS";
        case CERROR_PERMISSION_DENIED:   return "PERMISSION_DENIED";
        case CERROR_RESOURCE_EXHAUSTED:  return "RESOURCE_EXHAUSTED";
        case CERROR_FAILED_PRECONDITION: return "FAILED_PRECONDITION";
        case CERROR_ABORTED:             return "ABORTED";
        case CERROR_OUT_OF_RANGE:        return "OUT_OF_RANGE";
        case CERROR_UNIMPLEMENTED:       return "UNIMPLEMENTED";
        case CERROR_INTERNAL:            return "INTERNAL";
        case CERROR_UNAVAILABLE:         return "UNAVAILABLE";
        case CERROR_DATA_LOSS:           return "DATA_LOSS";
        case CERROR_UNAUTHENTICATED:     return "UNAUTHENTICATED";
        default:                         return "UNKNOWN_STATUS";
    }
}

/**
 * @brief Convert gRPC status to HTTP status code
 */
static inline int cerror_grpc_status_to_http_status(const CErrorStatusCode status)
{
    switch (status) {
        case CERROR_OK:                  return 200;
        case CERROR_INVALID_ARGUMENT:    return 400;
        case CERROR_FAILED_PRECONDITION: return 400;
        case CERROR_OUT_OF_RANGE:        return 400;
        case CERROR_UNAUTHENTICATED:     return 401;
        case CERROR_PERMISSION_DENIED:   return 403;
        case CERROR_NOT_FOUND:           return 404;
        case CERROR_ABORTED:             return 409;
        case CERROR_ALREADY_EXISTS:      return 409;
        case CERROR_RESOURCE_EXHAUSTED:  return 429;
        case CERROR_CANCELLED:           return 499;
        case CERROR_UNKNOWN:             return 500;
        case CERROR_INTERNAL:            return 500;
        case CERROR_DATA_LOSS:           return 500;
        case CERROR_UNIMPLEMENTED:       return 501;
        case CERROR_UNAVAILABLE:         return 503;
        case CERROR_DEADLINE_EXCEEDED:   return 504;
        default:                         return 500;
    }
}

/**
 * @brief Convert error code (full or status only) to HTTP status code
 */
static inline int cerror_code_to_http_status(const uint64_t ullError)
{
    if (ullError == 0) return 200;
    /* Extract status (5 bits) using the macro from lasterror.h */
    return cerror_grpc_status_to_http_status((CErrorStatusCode)GET_STATUS(ullError));
}

static inline const char* getStatusCodeString(const CErrorStatusCode statusCode) {
    return cerror_get_status_code_string(statusCode);
}

static inline int grpcStatusToHttpStatus(const CErrorStatusCode status) {
    return cerror_grpc_status_to_http_status(status);
}

static inline int errorCodeToHttpStatus(const uint64_t ullError) {
    return cerror_code_to_http_status(ullError);
}


#ifdef __cplusplus
}
#endif