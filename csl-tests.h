#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef CSL_TEST
# define CSL_TEST

# ifndef CSL_TEST_NO_COLORS
#  define CSL_TEST_ANSI_RED   "\033[31m"
#  define CSL_TEST_ANSI_GREEN "\033[32m"
#  define CSL_TEST_ANSI_RESET "\033[0m"
#  define CSL_TEST_ANSI_BOLD "\033[1m"
# else
#  define CSL_TEST_ANSI_RED
#  define CSL_TEST_ANSI_GREEN
#  define CSL_TEST_ANSI_RESET
#  define CSL_TEST_ANSI_BOLD
# endif

#define CSL_TEST_ASSERT(condition, msg) csl_run_test(condition, #condition, __FILE__, __LINE__, __FUNCTION__, msg)

size_t csl_test_pass_counter = 0;
size_t csl_test_fail_counter = 0;

void csl_run_test(bool result, const char* expr, const char* file, unsigned int line, const char* func, const char* msg) {
    if(result) {
        csl_test_pass_counter++;
#ifndef CSL_TEST_ONLY_FAILS
        printf(CSL_TEST_ANSI_BOLD "%s:%u: " CSL_TEST_ANSI_GREEN "pass: " CSL_TEST_ANSI_RESET
            "(%s) in function" CSL_TEST_ANSI_BOLD " \"%s\"" CSL_TEST_ANSI_RESET "\n", file, line, expr, func);
#endif
    } else {
        csl_test_fail_counter++;
        printf(CSL_TEST_ANSI_BOLD "%s:%u: " CSL_TEST_ANSI_RED "fail: " CSL_TEST_ANSI_RESET 
            "(%s) in function" CSL_TEST_ANSI_BOLD " \"%s\"" CSL_TEST_ANSI_RESET " - %s\n", file, line, expr, func, msg);
    }
}

/* TODO: Atexit function for showing test summary */
void csl_test_summary(void) {
    printf("================================================================================\n"
            CSL_TEST_ANSI_BOLD "Summary" CSL_TEST_ANSI_RESET ": %zu tests run, %zu failed, %zu passed.\n"
            , csl_test_pass_counter + csl_test_fail_counter 
            , csl_test_fail_counter
            , csl_test_pass_counter);
}

#define CSL_TEST_INIT if(atexit(csl_test_summary)) fprintf(stderr, "warning: failed to set exit function.\n")

#endif
