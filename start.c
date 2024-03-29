#include "start.h"

#include <curl/curl.h>
#include <omp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "absl/base/port.h"
#include "pcg_basic.h"
#include "base.h"
#include "calculator.h"
#include "config.h"
#include "FTPManagement.h"
#include "logger.h"
#include "shutdown.h"
#include "types.h"

#if _IS_WINDOWS
#include <windows.h>
#endif

#define UNSET_FRAME_RECORD 9999

int current_frame_record;

// May get a value <0 if local record was corrupt.
int getLocalRecord() {
	int current_frame_record_orig = current_frame_record;
	if (ABSL_PREDICT_FALSE(current_frame_record < 0)) {
		printf("Current frame record is corrupt (less then 0 frames). Resetting (you may get false PBs for a while).\n");
		current_frame_record = UNSET_FRAME_RECORD;
		return current_frame_record_orig;
	}
	return current_frame_record;
}
void setLocalRecord(int frames) {
	if (ABSL_PREDICT_FALSE(frames < 0)) {
		printf("Got corrupt PB of %d frames. Ignoring\n", frames);
		return;
	}
	current_frame_record = frames;
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
	} else {
		requestShutdown();
		if (!_IS_WINDOWS || !isSignal) {
			printf("\nExit requested, finishing up work. Should shutdown soon (CTRL-C %d times total to force exit)\n", NUM_TIMES_EXITED_BEFORE_HARD_QUIT);
		}
	}
}

void handleTermSignal(int signal) {
	countAndSetShutdown(true);
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
#endif

void setSignalHandlers() {
	signal(SIGTERM, handleTermSignal);
	signal(SIGINT, handleTermSignal);
#if _IS_WINDOWS
	if (!SetConsoleCtrlHandler(windowsCtrlCHandler, TRUE)) {
		printf("Unable to set CTRL-C handler. CTRL-C may cause unclean shutdown.\n");
	}
#endif
}

void printAsciiGreeting()
{
	FILE *fp = fopen("img.txt", "r");
	char data[255];
	char *status;

	if (fp == NULL)
		return;

	status = fgets(data, sizeof(data), fp);
	while (status)
	{
		printf("%s", data);
		status = fgets(data, sizeof(data), fp);
	}

	printf("\n");

	fclose(fp);
}

uint64_t getSysRNG() {
	uint64_t ret;

#if _IS_WINDOWS
	HCRYPTPROV hCryptProv = 0;
	const DWORD dwLength = 8;
	BYTE pbData[8];
	bool bSuccess = false;

	if (CryptAcquireContext(&hCryptProv, NULL, (LPCWSTR)L"Microsoft Base Cryptographic Provider v1.0",
		PROV_RSA_FULL,
		CRYPT_VERIFYCONTEXT)) {
		if (CryptGenRandom(hCryptProv, dwLength, pbData))
			bSuccess = true;
	}

	if (!bSuccess) {
		// Log the issue as the user's window will likely close immediately
		char errMsg[70];
		sprintf(errMsg, "Windows random generator failed with error %lu", GetLastError());
		recipeLog(1, "Startup", "PRNG", "Seeding", errMsg);
		exit(1);
	}

	ret = *(uint64_t*)pbData;
#else
	FILE* fp;
	int readRet;
	if ((fp = fopen("/dev/random", "r")) == NULL || (readRet = fread(&ret, sizeof(uint64_t), 1, fp)) == 0) {
		printf("Unable to access /dev/random for RNG seeding. Please submit a GitHub issue and include your OS version.");
		exit(1);
	}
#endif
	return ret;
}

int main() {
	current_frame_record = UNSET_FRAME_RECORD;
	initConfig();
	validateConfig();

	// When performing in-order traversal, the same roadmap will be calculated
	// on every thread, so we should only use 1. When choosing moves manually,
	// we should likewise use only 1, or else multiple threads will be
	// interacting with the user at once.
	enum SelectionMethod method = getConfigInt("selectionMethod");
	int workerCount = (method == InOrder || method == Random) ? 1
	                  : getConfigInt("workerCount");

	init_level_cfg(); // set log level from config
	curl_global_init(CURL_GLOBAL_DEFAULT);	// Initialize libcurl

	// Greeting message to user
	printAsciiGreeting();
	printf("Welcome to Recipes@Home!\n");
	printf("Leave this program running as long as you want to search for new recipe orders.\n");

	// Try to retrieve the record from the Blob server
	int blob_record = getFastestRecordOnBlob();
	if (blob_record == 0) {
		printf("There was an error contacting the server to retrieve the fastest time.\n");
		printf("Please check your internet connection, but we'll continue for now.\n");
	}
	else {
		printf("The current fastest record is %d frames. Happy cooking!\n", blob_record);
	}

	// Quit if new version available
	if (checkGithubVer() == 1)
		return -1;

	// Verify that the results folder exists
	// If not, create the directory
#if _IS_WINDOWS
	CreateDirectoryA("./results", NULL);
#else
	mkdir("./results", 0777);
#endif

	// To avoid generating roadmaps that are slower than the user's record best,
	// use PB.txt to identify the user's current best
	FILE* fp = fopen("results/PB.txt", "r");

	// The PB file may not have been created yet, so ignore the case where it is missing
	if (fp != NULL) {
		int PB_record;
		if (fscanf(fp, "%d", &PB_record) == 1) {
			if (PB_record < 1000) {
				printf("The record stored in PB.txt can't be right... Ignoring.\n");
			} else {
				current_frame_record = PB_record;
				// Submit the user's fastest roadmap to the server for leaderboard purposes
				// in case this was not submitted upon initial discovery
				testRecord(current_frame_record);
			}
		}
		fclose(fp);
	}

	// Initialize global variables in calculator.c
	// This does not need to be done in parallel, as these globals will
	// persist through all parallel calls to calculator.c
	initializeInvFrames();
	initializeRecipeList();

	setSignalHandlers();

	// Seed each thread's PRNG for those selection methods that use it.
	seedThreadRNG(workerCount);

	// Create workerCount threads
	omp_set_num_threads(workerCount);
	#pragma omp parallel
	{
		int ID = omp_get_thread_num();

		while (1) {
			if (askedToShutdown()) {
				break;
			}
			Result result = calculateOrder(ID);

			// result might store -1 frames for errors that might be recoverable
			if (result.frames > -1) {
				testRecord(result.frames);
			}
		}
	}

	shutdownConfig();
	shutdownCalculator();

	return 0;
}
