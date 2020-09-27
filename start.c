#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <libconfig.h>
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
#include <sys/stat.h>
#include <sys/types.h>

int current_frame_record;
const char *local_ver;

int getLocalRecord() {
	return current_frame_record;
}
void setLocalRecord(int frames) {
	current_frame_record = frames;
}

const char *getLocalVersion() {
	return local_ver;
}

int main() {
	int cycle_count = 1;
	current_frame_record = 9999;
	initConfig();

	// If select and randomise are both 0, the same roadmap will be calculated on every thread, so set threads = 1
	int workerCount = getConfigInt("select") || getConfigInt("randomise") ? getConfigInt("workerCount") : 1;
	local_ver = getConfigStr("Version");
	init_level_cfg();
	curl_global_init(CURL_GLOBAL_DEFAULT);	// Initialize libcurl
	int update = checkForUpdates(local_ver);
	
	if (update == -1) {
		printf("Could not check version on Github. Please check your internet connection.\n");
		printf("Otherwise, we can't submit compelted roadmaps to the server!\n");
		printf("Alternatively you may have been rate-limited. Please wait a while and try again.\n");
		printf("Press ENTER to quit.\n");
		char exitChar = getchar();
		return -1;
	}
	else if (update == 1) {
		printf("Please visit https://github.com/SevenChords/CipesAtHome/releases to download the newest version of this program!");
		printf("Press ENTER to quit.");
		char exitChar = getchar();
		return -1;
	}

	// Verify that the results folder exists
	// If not, create the directory
	mkdir("./results", 0777);

	// Initialize global variables in calculator.c
	// This does not need to be done in parallel, as these globals will
	// persist through all parallel calls to calculator.c
	initializeInvFrames();
	initializeRecipeList();
	
	// Create workerCount threads
	omp_set_num_threads(workerCount);
	#pragma omp parallel
	{
		int ID = omp_get_thread_num();
		
		// Seed each thread's PRNG for the select and randomise config options
		srand(((int)time(NULL)) ^ ID);
		
		while (1) {
			struct Result result = calculateOrder(ID);
			
			#pragma omp critical
			{
				current_frame_record = result.frames;
			}
			
			testRecord(result.frames);
			cycle_count++;
		}
	}
	
	return 0;
}
