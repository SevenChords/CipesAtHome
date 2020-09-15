#ifndef START_H
#define START_H

#include "inventory.h"
#include "recipes.h"
#include "config.h"
#include "calculator.h"


struct Result {
	int frames;
	int callNumber;
};

struct Job {
	int callNumber;
	struct Item *startingInventory;
	int current_frame_record;
	struct Result result;
	const char *local_ver;
};

// Defined in calculator.c
struct BranchPath *initializeRoot(struct Job job);

// Defined in calculator.c
void periodicCheckForUpdate(struct Job job);

// Used to test behavior of various functions as roadmaps are calculated and verified
void userDebugSession(struct Job job);

// Main routine
struct Result calculateOrder(struct Job job);

int main(); // Main method for entire algorithm

#endif
