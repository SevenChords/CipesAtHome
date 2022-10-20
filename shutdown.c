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
	// For these events, returning TRUE is sufficient to prevent shutdown, so
	// we can simply inform the threads to shut down.
	case CTRL_C_EVENT: ABSL_FALLTHROUGH_INTENDED;
	case CTRL_BREAK_EVENT:
		countAndSetShutdown(false);
		return TRUE;
	// Returning TRUE for a CTRL_CLOSE_EVENT results in instant termination, so
	// we have to wait until the threads are done.
	case CTRL_CLOSE_EVENT:
		countAndSetShutdown(false);
		// Note that this does not cause closing to take 5 seconds. The program
		// will actually shut down when main returns. 5 seconds is just the max
		// time we could delay shutdown.
		Sleep(5000);
		return TRUE;
	// CTRL_LOGOFF_EVENT and CTRL_SHUTDOWN_EVENT are not sent to interactive
	// applications, only to services. Thus, there is no behavior to add in
	// here.
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
