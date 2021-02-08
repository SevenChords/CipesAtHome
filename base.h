// Functions and Macros for common logic that doesn't really belong anywhere else.

#include <stddef.h>
#include <stdio.h>
#include "absl/base/port.h"

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
		printf("Press enter to quit.");
		ABSL_ATTRIBUTE_UNUSED char exitChar = getchar();
		exit(1);
	}
}
