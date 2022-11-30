#ifndef CRIPES_SHUTDOWN_H
#define CRIPES_SHUTDOWN_H

#include <stdbool.h>
#include "absl/base/port.h"

extern bool _askedToShutdownVar;

void setSignalHandlers();

ABSL_ATTRIBUTE_ALWAYS_INLINE inline bool askedToShutdown() {
	return ABSL_PREDICT_FALSE(_askedToShutdownVar);
}
#endif
