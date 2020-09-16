#ifndef CALCULATOR_H
#define CALCULATOR_H
#include "inventory.h"
#include "recipes.h"
#include "start.h"

enum Action {
	Begin,
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
	int itemIndex1;		// index of first item we are cooking
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
	int lateSort;			// 0 - sort after Coconut, 1 - sort after Keel Mango
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
	struct MoveDescription description;
	struct BranchPath *prev;	// Needed since we'll often be moving back up the tree
	struct BranchPath *next;
	int *outputCreated;		// Array of 58 items, 1 if item was produced, 0 if not; indexed by recipe ordering
	int numOutputsCreated;		// Number of valid outputCreated entries to eliminate a lengthy for-loop
	struct BranchPath **legalMoves;// Represents possible next paths to take
	int numLegalMoves;
};

struct OptimizeResult {
	struct BranchPath *root;
	struct BranchPath *last;
};

void handleRecipeOutput(struct BranchPath *curNode, struct Item *tempInventory, int tempFrames, struct MoveDescription useDescription, int *tempOutputsFulfilled, int numOutputsFulfilled, struct Item output, int viableItems);

void createCookDescription2Items(struct BranchPath *node, struct Recipe recipe, struct ItemCombination combo, struct Item *tempInventory, int *ingredientLoc, int *ingredientOffset, int *tempFrames, int viableItems, struct MoveDescription *useDescription);

void createCookDescription1Item(struct BranchPath *node, struct Recipe recipe, struct ItemCombination combo, struct Item *tempInventory, int *ingredientLoc, int *ingredientOffset, int *tempFrames, int viableItems, struct MoveDescription *useDescription);

struct MoveDescription createCookDescription(struct BranchPath *node, struct Recipe recipe, struct ItemCombination combo, struct Item *tempInventory, int *tempFrames, int viableItems);

int itemComboInInventory(struct ItemCombination combo, struct Item *inventory);

void handleDBCOAllocation0Nulls(struct BranchPath *curNode, struct Item *tempInventory, int *tempOutputsFulfilled, int numOutputsFulfilled, int viableItems);

void handleDBCOAllocation1Null(struct BranchPath *curNode, struct Item *tempInventory, int *tempOutputsFulfilled, int numOutputsFulfilled, int viableItems);

void handleDBCOAllocation2Nulls(struct BranchPath *curNode, struct Item *tempInventory, int *tempOutputsFulfilled, int numOutputsFulfilled, int viableItems);

struct CH5 *createChapter5Struct(int DB_place_index, int CO_place_index, int KM_place_index, int CS_place_index, int TR_use_index, enum Action sort, int lateSort);

void initializeRecipeList();

void initializeInvFrames();

struct BranchPath *copyAllNodes(struct BranchPath *newNode, struct BranchPath *oldNode);

struct OptimizeResult optimizeRoadmap(struct BranchPath *root); 

void printCh5Sort(struct CH5 *ch5Data, FILE *fp);

void printOutputsCreated(struct BranchPath *curNode, FILE *fp);

void printInventoryData(struct BranchPath *curNode, FILE *fp);

void printSortData(FILE *fp, enum Action curNodeAction);

void printFileHeader(FILE *fp);

void printCh5Data(struct BranchPath *curNode, struct MoveDescription desc, FILE *fp);

void printCookData(struct BranchPath *curNode, struct MoveDescription desc, FILE *fp);

void handleSelectAndRandom(struct BranchPath *curNode, int select, int randomise);

void handleSorts(struct BranchPath *curNode);

// Compartmentalized handling of Chapter 5 prior to handleChapter5Eval
void fulfillChapter5(struct BranchPath *curNode);

void fulfillRecipes(struct BranchPath *curNode, int recipeIndex);

void freeInvFrames(int **invFrames);

void tryTossInventoryItem(struct BranchPath *curNode, struct Item *tempInventory, struct MoveDescription useDescription, int *tempOutputsFulfilled, int numOutputsFulfilled, int tossedIndex, struct Item output, int tempFrames, int viableItems);

void finalizeLegalMove(struct BranchPath *node, int tempFrames, struct MoveDescription useDescription, struct Item *tempInventory, int *tempOutputsFulfilled, int numOutputsFulfilled, enum HandleOutput tossType, struct Item toss, int tossIndex);

void finalizeChapter5Eval(struct BranchPath *node, struct Item *inventory, enum Action sort, struct CH5 *ch5Data, int temp_frame_sum, int *outputsFulfilled, int numOutputsFulfilled);

void applyJumpStorageFramePenalty(struct BranchPath *node);

void freeAllNodes(struct BranchPath *node);

void freeLegalMove(struct BranchPath *node, int index);

void copyCook(struct Cook *cookNew, struct Cook *cookOld);

struct BranchPath *createLegalMove(struct BranchPath *node, struct Item *inventory, struct MoveDescription description, int *outputsFulfilled, int numOutputsFulfilled);

int selectSecondItemFirst(struct BranchPath *node, struct ItemCombination combo, int *ingredientLoc, int *ingredientOffset, int viableItems);

void generateFramesTaken(struct MoveDescription *description, struct BranchPath *node, int framesTaken);

void generateCook(struct MoveDescription *description, struct ItemCombination combo, struct Recipe recipe, int *ingredientLoc, int swap);

void shuffleLegalMoves(struct BranchPath *node);

void popAllButFirstLegalMove(struct BranchPath *node);

void filterOut2Ingredients(struct BranchPath *node);

void shiftUpLegalMoves(struct BranchPath *node, int startIndex);

// Remove node's legal moves which exceed the current best frame record
void filterLegalMovesExceedFrameLimit(struct BranchPath *node, int frames);

// Traverse back to previous nodes to count the total number of sorts
int countTotalSorts(struct BranchPath *node);

// Evaluates all possible placements of the Keel Mango and Courage Shell
// and all possible locations and types of sorting that can place the Coconut into a position where it can be duplicated
void handleChapter5Eval(struct BranchPath *node, struct Item *inventory, int *outputsFulfilled, int numOutputsFulfilled, int frames_DB, int frames_CO, int DB_place_index, int CO_place_index);

// Handles allocation of the Keel Mango and Courage Shell, both of which happen after the sort to correctly place
// the Coconut in a location where it can be duplicated
void handleChapter5EarlySortEndItems(struct BranchPath *node, struct Item *inventory, int *outputsFulfilled, int numOutputsFulfilled, int sort_frames, enum Action sort, int frames_DB, int frames_CO, int DB_place_index, int CO_place_index);

// Performs all sorts, verifies Coconut is in slots 11-20, and checks to see if all other recipes can be fulfilled
void handleChapter5Sorts(struct BranchPath *node, struct Item *inventory, int *outputsFulfilled, int numOutputsFulfilled, int frames_DB, int frames_CO, int frames_KM, int DB_place_index, int CO_place_index, int KM_place_index);

// Handles allocation of the Courage Shell
// The inventory has already had the Keel Mango placed,
// and a sort has occurred to place the Coconut into a location
// where it can be duplicated
void handleChapter5LateSortEndItems(struct BranchPath *node, struct Item *inventory, int *outputsFulfilled, int numOutputsFulfilled, int sort_frames, enum Action sort, int frames_DB, int frames_CO, int frames_KM, int DB_place_index, int CO_place_index, int KM_place_index);

void insertIntoLegalMoves(int insertIndex, struct BranchPath *newLegalMove, struct BranchPath *curNode);

// Find out where to place a new legalMove in the array of legalMoves
int getInsertionIndex(struct BranchPath *node, int frames);

// Moves all items down one position to fill up the first null item
void shiftDownToFillNull(struct Item *inventory);

// Swaps a set of item indices when determining that it's faster to choose two ingredients in reverse order
void swapItems(int *ingredientLoc, int *ingredientOffset);

int *copyOutputsFulfilled(int *oldOutputsFulfilled);

// Iterate through nodes to find the current node
void getCurNode(struct BranchPath *node, int stepIndex);

// Frees all pointers when we go back up a node
int freeNode(struct BranchPath *node);

// Print the results of all states observed in the current stack
// filename represents the output roadmap filename
// path represents the array of data that contains info for every single action we've done along the roadmap
int printResults(char *filename, struct BranchPath *path);

int alpha_sort(const void *elem1, const void *elem2);
int alpha_sort_reverse(const void *elem1, const void *elem2);
int type_sort(const void *elem1, const void *elem2);
int type_sort_reverse(const void *elem1, const void *elem2);

// Return a sorted inventory
// Written in calculator.c instead of inventory.c because it is dependent on enum Action
struct Item *getSortedInventory(struct Item *inventory, enum Action sort);

int main();
#endif
