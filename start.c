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

int main() {
	int cycle_count			= 1;
	struct Item *startingInventory 	= getStartingInventory();
	int workerCount;
	int current_frame_record	= 9999;
	config_t *config 		= getConfig();
	config_lookup_int(config, "workerCount", &workerCount);
	init_level_cfg();
	const char *local_ver;
	config_lookup_string(config, "Version", &local_ver);
	
	// Seed the RNG for the select config option
	srand(time(0));
	
	// Initialize libcurl
	curl_global_init(CURL_GLOBAL_DEFAULT);
	
	int update = checkForUpdates(local_ver);
	if (update == -1) {
		printf("Please check your internet connection in order to continue.\n");
		printf("Otherwise, we can't submit compelted roadmaps to the server!\n");
		return -1;
	}
	else if (update == 1) {
		printf("Please visit https://github.com/SevenChords/CipesAtHome/releases to download the newest version of this program!");
		return -1;
	}

	current_frame_record = getFastestRecordOnBlob();
	// Initialize global variable invFrames in calculator.c
	initializeInvFrames();
	initializeRecipeList();
	
	// Create workerCount threads
	double threads[workerCount];
	omp_set_num_threads(workerCount);
	#pragma omp parallel
	{
		int ID = omp_get_thread_num();
		struct Job job;
		job.callNumber = ID;
		job.startingInventory = startingInventory;
		job.current_frame_record = malloc(sizeof(int));
		job.local_ver = local_ver;
		
		while (1) {
			*(job.current_frame_record) = current_frame_record;
			job.result.frames = -1; // If these are set to -1, then we know there has not been a result produced
			job.result.callNumber = -1;
			struct Result result = calculateOrder(job);

			if (result.frames < current_frame_record) {
				testRecord(result.frames);
				current_frame_record = result.frames;
				
				// Add log
			}

			cycle_count++;
		}
	}
	
	/*struct Job job;
	job.callNumber = 0;
	job.startingInventory = startingInventory;
	job.recipeList = recipeList;
	job.current_frame_record = malloc(sizeof(int));
	job.local_ver = local_ver;
	
	while (1) {
		*(job.current_frame_record) = current_frame_record;
		job.result.frames = -1; // If these are set to -1, then we know there has not been a result produced
		job.result.callNumber = -1;
		struct Result result = calculateOrder(job);

		if (result.frames < current_frame_record && result.frames > -1) {
			testRecord(result.frames);
			current_frame_record = result.frames;
			
			// Add log
		}

		cycle_count++;
	}
	
	return 0;*/
}
