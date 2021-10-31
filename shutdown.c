#include "shutdown.h"

bool _askedToShutdownVar = false;

bool requestShutdown() {
	bool oldVal = _askedToShutdownVar;
	_askedToShutdownVar = true;
	return oldVal;
}

