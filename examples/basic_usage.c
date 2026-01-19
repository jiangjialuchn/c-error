/**
 * @file basic_usage.c
 * @brief Basic usage example for c-error library
 */

#include <c-error/lasterror.h>
#include <stdio.h>

/**
 * @brief Print error code with all fields decoded
 */
static void printErrorCode(const char* strLabel, const uint64_t ullError)
{
    printf("%s: 0x%013llX\n", strLabel, (unsigned long long)ullError);
    printf("  Error Code:   0x%04X (%u)\n", GET_ERROR_CODE(ullError), GET_ERROR_CODE(ullError));
    printf("  Status:       0x%02X (%u)\n", GET_STATUS(ullError), GET_STATUS(ullError));
    printf("  Component ID: 0x%03X (%u)\n", GET_COMPONENT_ID(ullError), GET_COMPONENT_ID(ullError));
    printf("  Software ID:  0x%02X (%u)\n", GET_SOFTWARE_ID(ullError), GET_SOFTWARE_ID(ullError));
    printf("  Reserved:     0x%04X (%u)\n", GET_RESERVED(ullError), GET_RESERVED(ullError));
    printf("\n");
}

/**
 * @brief Simulate a function that may fail
 */
static int processData(const char* pData, int nSize)
{
    if (pData == NULL) {
        setLastError(MAKE_ERROR_CODE(0x01, 0x10, 0x03, 0x0001));  /* Invalid argument */
        return 0;  /* failure */
    }

    if (nSize <= 0) {
        setLastError(MAKE_ERROR_CODE(0x01, 0x10, 0x03, 0x0002));  /* Invalid size */
        return 0;  /* failure */
    }

    /* Success */
    clearLastError();
    return 1;
}

/**
 * @brief Main example entry point
 */
int main(void)
{
    printf("c-error Basic Usage Example\n");
    printf("========================================\n\n");

    /* Note: No initialization needed! The thread-local context is automatically
     * initialized by the compiler. Just start using the error functions. */

    /* Example 1: Success case */
    printf("=== Example 1: Success Case ===\n");
    if (processData("test data", 9)) {
        printf("processData succeeded\n");
        printf("Last error: 0x%llX (should be 0)\n", (unsigned long long)getLastError());
        printf("isLastSuccess: %d\n\n", isLastSuccess());
    }

    /* Example 2: NULL pointer error */
    printf("=== Example 2: NULL Pointer Error ===\n");
    if (!processData(NULL, 10)) {
        printf("processData failed\n");
        uint64_t ullError = getLastError();
        printErrorCode("Error details", ullError);
    }

    /* Example 3: Invalid size error */
    printf("=== Example 3: Invalid Size Error ===\n");
    if (!processData("test", 0)) {
        printf("processData failed\n");
        printf("Error code: 0x%04X\n", getLastErrorCode());
        printf("Status: 0x%02X\n", getLastStatus());
        printf("Component ID: 0x%03X\n", getLastComponentId());
        printf("Software ID: 0x%02X\n\n", getLastSoftwareId());
    }

    /* Example 4: Constructing complex error codes */
    printf("=== Example 4: Complex Error Codes ===\n");

    uint64_t ullComplexError = MAKE_ERROR_CODE_53(
        0x0ABC,  /* reserved */
        0x42,    /* software ID */
        0x567,   /* component ID */
        0x0D,    /* status (internal error) */
        0x8901   /* error code */
    );

    setLastError(ullComplexError);
    printErrorCode("Complex error", getLastError());

    /* Example 5: Simple error code */
    printf("=== Example 5: Simple Error Code ===\n");
    setLastError(MAKE_SIMPLE_ERROR(0x11, 0x222, 0x3333));
    printErrorCode("Simple error", getLastError());

    /* Example 6: Validation */
    printf("=== Example 6: Error Code Validation ===\n");
    uint64_t ullValidError = MAKE_SIMPLE_ERROR(0x01, 0x02, 0x0003);
    uint64_t ullInvalidError = 0xFFFFFFFFFFFFFFFFULL;

    printf("Valid error code: %d (should be 1)\n", IS_VALID_ERROR_CODE(ullValidError));
    printf("Invalid error code: %d (should be 0)\n", IS_VALID_ERROR_CODE(ullInvalidError));
    printf("Success is valid: %d (should be 1)\n", IS_VALID_ERROR_CODE(0ULL));
    printf("\n");

    printf("========================================\n");
    printf("Example completed!\n");

    /* Cleanup the dynamic buffer before thread exit to avoid memory leak */
    cleanupThreadLocalErrorBuffer();

    return 0;
}
