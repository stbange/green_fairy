#ifndef GF_TEST_HARNESS_H
#define GF_TEST_HARNESS_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int gf_tests_run = 0;
static int gf_tests_failed = 0;

#define GF_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL %s:%d: assertion failed: %s\n", __FILE__, __LINE__, #cond); \
        gf_tests_failed++; \
        return; \
    } \
} while (0)

#define GF_ASSERT_NEAR(a, b, eps) do { \
    double _da = (double)(a); \
    double _db = (double)(b); \
    if (fabs(_da - _db) > (double)(eps)) { \
        fprintf(stderr, "FAIL %s:%d: %.6f != %.6f (eps=%.6f)\n", __FILE__, __LINE__, _da, _db, (double)(eps)); \
        gf_tests_failed++; \
        return; \
    } \
} while (0)

#define GF_ASSERT_STR_EQ(a, b) do { \
    if (!(a) || !(b) || strcmp((a), (b)) != 0) { \
        fprintf(stderr, "FAIL %s:%d: '%s' != '%s'\n", __FILE__, __LINE__, (a) ? (a) : "(null)", (b) ? (b) : "(null)"); \
        gf_tests_failed++; \
        return; \
    } \
} while (0)

#define GF_RUN_TEST(fn) do { \
    int _before = gf_tests_failed; \
    gf_tests_run++; \
    fn(); \
    if (gf_tests_failed == _before) { \
        printf("PASS %s\n", #fn); \
    } \
} while (0)

#define GF_TEST_SUMMARY() do { \
    printf("Executed %d tests, %d failed\n", gf_tests_run, gf_tests_failed); \
    return gf_tests_failed == 0 ? 0 : 1; \
} while (0)

#endif
