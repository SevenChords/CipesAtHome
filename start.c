#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <libconfig.h>
#include "base.h"
#include "inventory.h"
#include "config.h"
#include "recipes.h"
#include "FTPManagement.h"
#include "start.h"
#include "calculator.h"
#include <time.h>
#include "cJSON.h"
#include <curl/curl.h>
#include "logger.h"
#include "shutdown.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include "absl/base/port.h"

#if _IS_WINDOWS
#include <windows.h>
#endif

#define UNSET_FRAME_RECORD 9999

int current_frame_record;
const char *local_ver;

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
		printf("Got corrupt PB if %d frames. Ignoring\n", frames);
		return;
	}
	current_frame_record = frames;
}

const char *getLocalVersion() {
	return local_ver;
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

// Warn the user if their Username is set as "DefaultUser"
void checkDefaultUsername()
{
	const char* username = getConfigStr("Username");
	if (strncmp(username, "DefaultUser", 19) == 0)
		printf("WARNING: You haven't set your username in config.txt. You will not be identifiable on the leaderboards.\n");
}

int main() {

	int cycle_count = 1;
	current_frame_record = UNSET_FRAME_RECORD;
	initConfig();

	// If select and randomise are both 0, the same roadmap will be calculated on every thread, so set threads = 1
	// The debug setting can only be meaningfully used with one thread as well.
	int workerCount = (getConfigInt("select") || getConfigInt("randomise"))
					  && !getConfigInt("debug") ? getConfigInt("workerCount") : 1;

	local_ver = getConfigStr("Version"); // Recipes@Home version
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

	// Reference the Github for the latest release version
	int update = checkForUpdates(local_ver);
	if (update == -1) {
		printf("Could not check version on Github. Please check your internet connection.\n");
		printf("Otherwise, we can't submit completed roadmaps to the server!\n");
		printf("Alternatively you may have been rate-limited. Please wait a while and try again.\n");
		printf("Press ENTER to quit.\n");
		awaitKeyFromUser();
		return -1;
	}
	else if (update == 1) {
		printf("There is a newer version of Recipes@Home.\nTo continue, please visit https://github.com/SevenChords/CipesAtHome/releases to download the newest version of this program!\n");
		printf("Press ENTER to quit.\n");
		awaitKeyFromUser();
		return -1;
	}

	checkDefaultUsername();

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

	// Create workerCount threads
	omp_set_num_threads(workerCount);
	#pragma omp parallel
	{
		int ID = omp_get_thread_num();

		// Seed each thread's PRNG for the select and randomise config options
		srand(((int)time(NULL)) ^ ID);

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

	return 0;
}
