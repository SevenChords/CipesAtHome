// Functions and Macros for common logic that doesn't really belong anywhere else.

#include <stddef.h>
#include <stdio.h>
#include "absl/base/port.h"

#if defined(_MSC_FULL_VER) || defined(_MSC_VER) || defined(__MINGW32__)
#define _IS_WINDOWS 1
#else
#define _IS_WINDOWS 0
#endif

// Microsoft standard library provides these as an extension, but we need definitions for the POSIX side.
#ifndef min
// Computes the minimum between two values.
// If the two values compare equal, which argument is returned is unspecified.
// WARNING: This implementation takes NO precautions against
// arguments having side effects or being expensive to compute.
#define min(x, y) ((y) < (x) ? (y) : (x))
#endif

#ifndef max
// Computes the maximum between two value.
// If the two values compare equal, which argument is returned is unspecified.
// WARNING: This implementation takes NO precautions against
// arguments having side effects or being expensive to compute.
#define max(x, y) ((y) > (x) ? (y) : (x))
#endif

 /*-------------------------------------------------------------------
 * Function 	: awaitKeyFromUser
 * Outputs: char the character entered by the user.
 *
 * Maps directly to 'getchar()' from 'stdio.h', but does not come with
 * the warning that the output must be used.
 *
 * This function is for cases where the program wants to pause for user
 * input (usually to read a message), but might not actually care what
 * character the user entered.
 -------------------------------------------------------------------*/
ABSL_ATTRIBUTE_ALWAYS_INLINE
inline char awaitKeyFromUser() {
	return getchar();
}

 /*-------------------------------------------------------------------
 * Function 	: checkMallocFailed
 * Inputs	: void* p
 *
 * This function tests whether malloc failed to allocate (usually due
 * to lack of heap space), checked by giving the just malloc'ed pointer, p,
 * to this function.
 * If it was NULL, then this function will print an error message and
 * exit the program with a failure status.
 -------------------------------------------------------------------*/
inline void checkMallocFailed(const void* const p) {
	if (ABSL_PREDICT_FALSE(p == NULL)) {
		printf("Fatal error! Ran out of heap memory.\n");
		printf("Press enter to quit.\n");
		awaitKeyFromUser();
		exit(1);
	}
}
