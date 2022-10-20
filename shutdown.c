#include "shutdown.h"
#include "base.h"

#if _IS_WINDOWS
#include <windows.h>
#else
#include <signal.h>
#endif

bool _askedToShutdownVar = false;

bool requestShutdown() {
	bool oldVal = _askedToShutdownVar;
	_askedToShutdownVar = true;
	return oldVal;
}

int numTimesExitRequest = 0;
#define NUM_TIMES_EXITED_BEFORE_HARD_QUIT 3

void countAndSetShutdown(bool isSignal) {
	// On Windows, it is undefined behavior trying to use stdio.h functions in a signal handler (which this is called from).
	// So for now, these messages are stifled on Windows. This may be revisited at a later point.
	if (++numTimesExitRequest >= NUM_TIMES_EXITED_BEFORE_HARD_QUIT) {
		if (!_IS_WINDOWS || !isSignal) {
			printf("\nExit reqested %d times; shutting down now.\n", NUM_TIMES_EXITED_BEFORE_HARD_QUIT);
		}
		exit(1);
	}
	else {
		requestShutdown();
		if (!_IS_WINDOWS || !isSignal) {
			printf("\nExit requested, finishing up work. Should shutdown soon (CTRL-C %d times total to force exit)\n", NUM_TIMES_EXITED_BEFORE_HARD_QUIT);
		}
	}
}

#if _IS_WINDOWS
BOOL WINAPI windowsCtrlCHandler(DWORD fdwCtrlType) {
	switch (fdwCtrlType) {
	case CTRL_C_EVENT: ABSL_FALLTHROUGH_INTENDED;
	case CTRL_CLOSE_EVENT:
		countAndSetShutdown(false);
		return TRUE;
	default:
		return FALSE;
	}
}
#else
void handleTermSignal(int signal) {
	countAndSetShutdown(true);
}
#endif

void setSignalHandlers() {
#if _IS_WINDOWS
	if (!SetConsoleCtrlHandler(windowsCtrlCHandler, TRUE)) {
		printf("Unable to set CTRL-C handler. CTRL-C may cause unclean shutdown.\n");
	}
#else
	signal(SIGTERM, handleTermSignal);
	signal(SIGINT, handleTermSignal);
#endif
}
