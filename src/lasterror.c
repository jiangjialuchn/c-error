/** @file lasterror.c
 *  @brief Thread-local Error Code Storage Implementation (Pure C, Cross-platform)
 *
 *  Uses compiler-specific thread-local storage keywords for zero-overhead access.
 *
 *  @author c-error contributors
 *  @date 2026-01-19
 */

#include "c-error/lasterror.h"

/* ============================================================================
 * Thread-local Storage Variable Definition
 * ============================================================================ */

/**
 * @brief Thread-local error context variable (zero-initialized by compiler)
 *
 * The compiler automatically initializes this to zero for each thread:
 * - ullLastError = 0
 * - pszLastErrorInfo = NULL
 * - pszLastErrorInfoBuffer = NULL
 * - nBufferCapacity = 0
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
    /* C11 standard thread-local storage */
    _Thread_local ErrorContext g_LastErrorCtx = {0};
#elif defined(_MSC_VER)
    /* Microsoft Visual C++ */
    __declspec(thread) ErrorContext g_LastErrorCtx = {0};
#elif defined(__GNUC__) || defined(__clang__)
    /* GCC and Clang */
    __thread ErrorContext g_LastErrorCtx = {0};
#else
    #error "Thread-local storage not supported on this compiler"
#endif

/* ============================================================================
 * Thread-local Buffer Cleanup
 * ============================================================================ */

/**
 * @brief Cleanup the dynamic buffer in thread-local error context
 *
 * Call this function before thread exit to free the dynamically allocated buffer.
 * This function is safe to call multiple times or when the buffer is not allocated.
 *
 * @note This only frees the buffer (pszLastErrorInfoBuffer), not the context itself.
 *       The context (g_LastErrorCtx) is managed by the compiler and will be
 *       automatically destroyed when the thread exits.
 */
void cleanupThreadLocalErrorBuffer(void)
{
    /* Free the dynamic buffer if allocated */
    if (NULL != g_LastErrorCtx.pszLastErrorInfoBuffer)
    {
        free(g_LastErrorCtx.pszLastErrorInfoBuffer);
        g_LastErrorCtx.pszLastErrorInfoBuffer = NULL;
        g_LastErrorCtx.nBufferCapacity = 0;
    }

    /* Reset error state */
    g_LastErrorCtx.ullLastError = 0ULL;
    g_LastErrorCtx.pszLastErrorInfo = NULL;
}
