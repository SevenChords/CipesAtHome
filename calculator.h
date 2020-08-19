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

enum HandleOutput {
	Toss,
	Autoplace
};

struct MoveDescription {
	// Store data pertaining to what we did at a particular point in the roadmap
	enum Action action;		// Cook, sort, handle CH5,...
	int numItems;			// If action == Cook, represents the number of items we are cooking (1 or 2)
	struct Item item1; 		// If action == Cook, represents first item cooked
	int itemIndex1;		// If action == Cook, represents index of first item we are cooking; slot = index + 1
	struct Item item2; 		// If action == Cook, represents first item cooked (optional)
	int itemIndex2;		// If action == Cook, represents index of second item we are cooking (optional); slot = index + 1
	struct Item output;		// if action == Cook, represents the item produced from cooking item1 and item2
	int framesTaken;		// Number of frames wasted to perform this action
	int totalFramesTaken;		// Cummulative number of frames wasted to perform all actioons thus far
	enum HandleOutput handleOutput;// Do we toss something or is the output autoplaced?
	struct Item toss;		// Item that we are tossing
	
	// The following are only for the Chapter 5 case...
	// it seems wasteful to put it in here when it won't be used for the other 57 nodes,
	// but I can't think of a better solution rn
	int indexDriedBouquet;		// The index that we toss to make room for Dried Bouquet
	int indexCoconut;		// The index that we toss to make room for Coconut
	enum Action ch5Sort;		// Sorting method used after picking up the Coconut
	int indexKeelMango;
	int indexCourageShell;
	int indexThunderRage;		// Index of Thunder Rage when we use it during the Smorg battle
};

struct BranchPath {
	int moves; // Represents how many nodes we've traversed down a particular branch (0 for root, 57 for leaf node)
	struct Item *inventory;
	struct MoveDescription *description;
	struct BranchPath *next;
	int *outputCreated;		// Array of 58 items, 1 if item was produced, 0 if not
};

// Print the results of all states observed in the current stack
// filename represents the output roadmap filename
// path represents the array of data that contains info for every single action we've done along the roadmap
int printResults(char *filename, struct BranchPath *path);

int main();
