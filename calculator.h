#include "inventory.h"
#include "recipes.h"

enum Action {
	Cook,
	Sort_Alpha_Asc,
	Sort_Alpha_Des,
	Sort_Type_Asc,
	Sort_Type_Des,
	Ch5
};

enum HandleOutput  {
	Toss,
	Autoplace,
	TossOther
};

struct Cook {
	int numItems;			// number of items we are cooking (1 or 2)
	struct Item item1;		// first item we are cooking
	int itemIndex1;			// index of first item we are cooking
	struct Item item2;		// second item we are cooking (optional)
	int itemIndex2;			// index of second item we are cooking
	struct Item output;		// item produced
	enum HandleOutput handleOutput;	// what do we do with this produced item?
	struct Item toss;		// item that we toss; {-1, -1} if we don't toss anything
	int indexToss;			// index of the item we are tossing (-1 if no toss)
};

struct CH5 {
	int indexDriedBouquet;		// index of item we toss to make room for Dried Bouquet
	int indexCoconut;		// index of item we toss to make room for Coconut
	enum Action ch5Sort;		// The sort type we perform after Coconut
	int indexKeelMango;		// index of item we toss to make room for Keel Mango
	int indexCourageShell;		// index of item we toss to make room for Courage Shell
	int indexThunderRage;		// index of Thunder Rage when we use it during Smorg (if 0-9, TR is removed)
};

struct MoveDescription {
	// Store data pertaining to what we did at a particular point in the roadmap
	enum Action action;		// Cook, sort, handle CH5,...
	void *data;			// This data may be either of type Cook, CH5, or NULL if we are just sorting
	int framesTaken;		// How many frames were wasted to perform this move
	int totalFramesTaken;		// Cummulative frame loss
};

struct BranchPath {
	int moves; // Represents how many nodes we've traversed down a particular branch (0 for root, 57 for leaf node)
	struct Item *inventory;
	struct MoveDescription *description;
	struct BranchPath *prev;	// Needed since we'll often be moving back up the tree
	struct BranchPath *next;
	int *outputCreated;		// Array of 58 items, 1 if item was produced, 0 if not
};

// Print the results of all states observed in the current stack
// filename represents the output roadmap filename
// path represents the array of data that contains info for every single action we've done along the roadmap
int printResults(char *filename, struct BranchPath *path);

int main();
