#include "lasterror.h"

namespace Chameleon
{
    namespace d {
        // RAII Helper for automatic thread-local buffer cleanup
        class CErrorHelper
        {
        public:
            ~CErrorHelper() { cerror_cleanup_thread_local_buffer();}
        };
        // Thread-local helper instance
        extern thread_local CErrorHelper g_errorHelper;
        inline void gc() {static thread_local CErrorHelper helper; (void)helper;}
    }

    // C++ Wrapper: Set the thread-local last error code.Ensures cleanup helper is initialized.
    inline void setLastError(const uint64_t ullError) {d::gc(); cerror_set_last(ullError);}

    inline uint64_t getLastError() {return cerror_get_last();}

    // C++ Wrapper: Clear the thread-local last error code
    inline void clearLastError() {cerror_clear_last();}

    // C++ Wrapper: Set thread-local error code with constant info string.Ensures cleanup helper is initialized.
    inline void setLastErrorInfo(const uint64_t ullError, const char* pszErrorInfo) {d::gc(); cerror_set_last_info(ullError, pszErrorInfo);}

    // C++ Wrapper: Set thread-local error code with info string (copy) Ensures cleanup helper is initialized to free the allocated buffer.
    inline void setLastErrorInfoCopy(const uint64_t ullError, const char* pszErrorInfo) {d::gc(); cerror_set_last_info_copy(ullError, pszErrorInfo);}

    // C++ Wrapper: Get the thread-local error info string
    inline const char* getLastErrorInfo() {return cerror_get_last_info();}
}

/* ============================================================================
 * Error Code Construction Macros - LEON Prefix (Wrappers)
 * ============================================================================ */

#define LEON_MAKE_ERROR_CODE_53(reserved, softwareId, componentId, status, errorCode) \
    MAKE_ERROR_CODE_53(reserved, softwareId, componentId, status, errorCode)

#define LEON_MAKE_ERROR_CODE(softwareId, componentId, status, errorCode) \
    MAKE_ERROR_CODE(softwareId, componentId, status, errorCode)

#define LEON_MAKE_ERROR_CODE_32(componentId, status, errorCode) \
    MAKE_ERROR_CODE_32(componentId, status, errorCode)


/* ============================================================================
 * Error Code Field Extraction Macros - LEON Prefix (Wrappers)
 * ============================================================================ */

#define LEON_GET_ERROR_CODE(ullError)     GET_ERROR_CODE(ullError)
#define LEON_GET_STATUS(ullError)         GET_STATUS(ullError)
#define LEON_GET_COMPONENT_ID(ullError)   GET_COMPONENT_ID(ullError)
#define LEON_GET_SOFTWARE_ID(ullError)    GET_SOFTWARE_ID(ullError)


/* ============================================================================
 * Error Code Testing Macros - LEON Prefix (Wrappers)
 * ============================================================================ */

#define LEON_IS_VALID_ERROR_CODE(ullError) IS_VALID_ERROR_CODE(ullError)