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
	struct Recipe *recipeList;
	int *current_frame_record;
	struct Result result;
};

// Defined in calculator.c
struct BranchPath *initializeRoot(struct Job job);

// Main routine
struct Result calculateOrder(struct Job job);

int main(); // Main method for entire algorithm

#endif
