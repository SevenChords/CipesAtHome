#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stdbool.h>
#include "absl/base/attributes.h"
#include "inventory.h"
#include "recipes.h"
#include "start.h"

typedef struct Result Result;

// Represent the action at a particular node in the roadmap
typedef enum Action Action;
enum Action {
	EBegin = 0,
	ECook = 1,
	ESort_Alpha_Asc = 2,
	ESort_Alpha_Des = 3,
	ESort_Type_Asc = 4,
	ESort_Type_Des = 5,
	ECh5 = 6
};

// What do we do with the produced item after crafting a recipe?
enum HandleOutput  {
	Toss,		// Toss the recipe itself
	Autoplace,	// The recipe is placed in an empty slot
	TossOther	// Toss a different item to make room
};

// Information pertaining to cooking a recipe
typedef struct Cook Cook;
struct Cook {
	int numItems;
	enum Type_Sort item1;
	int itemIndex1;
	enum Type_Sort item2;
	int itemIndex2;
	enum Type_Sort output;
	enum HandleOutput handleOutput;
	enum Type_Sort toss;
	int indexToss;
};

// Information pertaining to the Chapter 5 intermission
typedef struct CH5 CH5;
struct CH5 {
	int indexDriedBouquet;	// index of item we toss to make room for Dried Bouquet
	int indexCoconut;		// index of item we toss to make room for Coconut
	enum Action ch5Sort;	// The sort type we perform after Coconut
	int indexKeelMango;		// index of item we toss to make room for Keel Mango
	int indexCourageShell;	// index of item we toss to make room for Courage Shell
	int indexThunderRage;	// index of Thunder Rage when we use it during Smorg (if 0-9, TR is removed)
	int lateSort;			// 0 - sort after Coconut, 1 - sort after Keel Mango
};

// Information pertaining to the evaluation of Chapter 5 intermission moves
typedef struct CH5_Eval CH5_Eval;
struct CH5_Eval {
	int frames_at_DB;
	int frames_at_CO;
	int frames_at_KM;
	int frames_at_CS;
	int DB_place_index;
	int CO_place_index;
	int KM_place_index;
	int CS_place_index;
	int TR_use_index;
	int frames_at_HD;
	int frames_at_MC;
	int frames_at_TR;
	int frames_at_sort;
	enum Action sort;
};

// Overall data pertaining to what we did at a particular point in the roadmap
typedef struct MoveDescription MoveDescription;
struct MoveDescription {
	enum Action action;		// Cook, sort, handle CH5,...
	void *data;				// This data may be either of type Cook, CH5, or NULL if we are just sorting
	int framesTaken;		// How many frames were wasted to perform this move
	int totalFramesTaken;	// Cummulative frame loss
};

typedef struct Serial Serial;
struct Serial {
	uint8_t length;
	void *data;
};

typedef struct BranchPath BranchPath;
struct BranchPath {
	int moves;							// Represents how many nodes we've traversed down a particular branch (0 for root, 57 for leaf node)
	struct Inventory inventory;
	struct MoveDescription description;
	struct BranchPath *prev;
	struct BranchPath *next;
	outputCreatedArray_t outputCreated;					// Array of 58 items, true if item was produced, false if not; indexed by recipe ordering
	int numOutputsCreated;				// Number of valid outputCreated entries to eliminate a lengthy for-loop
	struct BranchPath **legalMoves;		// Represents possible next paths to take
	int numLegalMoves;
	int totalSorts;
	struct Serial serial;
};

// Structure to return the head and tail of an optimized roadmap
typedef struct OptimizeResult OptimizeResult;
struct OptimizeResult {
	struct BranchPath *root;
	struct BranchPath *last;
};

// optimizeRoadmap functions
BranchPath* copyAllNodes(BranchPath* newNode, const BranchPath* oldNode);
OptimizeResult optimizeRoadmap(const BranchPath* root);
void reallocateRecipes(BranchPath *newRoot, const enum Type_Sort *rearranged_recipes, int num_rearranged_recipes);
int removeRecipesForReallocation(BranchPath *node, enum Type_Sort *rearranged_recipes);

// Legal move functions

ABSL_MUST_USE_RESULT  // Output is newly allocated and needs to be freed at some point
BranchPath* createLegalMove(BranchPath* node, Inventory inventory, MoveDescription description, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled);
void filterOut2Ingredients(BranchPath* node);
void finalizeChapter5Eval(BranchPath* node, Inventory inventory, CH5* ch5Data, int temp_frame_sum, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled);
void finalizeLegalMove(BranchPath* node, int tempFrames, MoveDescription useDescription, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, enum HandleOutput tossType, enum Type_Sort toss, int tossIndex);
void freeLegalMove(BranchPath* node, int index);
int getInsertionIndex(const BranchPath* node, int frames);
void insertIntoLegalMoves(int insertIndex, BranchPath* newLegalMove, BranchPath* curNode);
void popAllButFirstLegalMove(BranchPath* node);
void shiftDownLegalMoves(BranchPath *node, int lowerBound, int uppderBound);
void shiftUpLegalMoves(BranchPath* node, int startIndex);

// Cooking functions

void createCookDescription2Items(const BranchPath* node, Recipe recipe, ItemCombination combo, Inventory* tempInventory, int* ingredientLoc, int* tempFrames, int viableItems, MoveDescription* useDescription);
void createCookDescription1Item(const BranchPath* node, Recipe recipe, ItemCombination combo, Inventory* tempInventory, int* ingredientLoc, int* tempFrames, int viableItems, MoveDescription* useDescription);
MoveDescription createCookDescription(const BranchPath* node, Recipe recipe, ItemCombination combo, Inventory *tempInventory, int* tempFrames, int viableItems);
void fulfillRecipes(BranchPath* curNode);
void generateCook(MoveDescription* description, const ItemCombination combo, const Recipe recipe, const int* ingredientLoc, int swap);
void handleRecipeOutput(BranchPath* curNode, Inventory tempInventory, int tempFrames, MoveDescription useDescription, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, enum Type_Sort output, int viableItems);
void tryTossInventoryItem(BranchPath* curNode, Inventory tempInventory, MoveDescription useDescription, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, enum Type_Sort output, int tempFrames, int viableItems);

// Chapter 5 functions
void fulfillChapter5(BranchPath* curNode);
void handleChapter5Eval(BranchPath* node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval);
void handleChapter5EarlySortEndItems(BranchPath* node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval);
void handleChapter5Sorts(BranchPath* node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval);
void handleChapter5LateSortEndItems(BranchPath* node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval);
void handleDBCOAllocation0Nulls(BranchPath* curNode, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, CH5_Eval eval);
void handleDBCOAllocation1Null(BranchPath* curNode, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, CH5_Eval eval);
void handleDBCOAllocation2Nulls(BranchPath* curNode, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, CH5_Eval eval);
CH5* createChapter5Struct(CH5_Eval eval, int lateSort);

// Initialization functions
void initializeInvFrames();
void initializeRecipeList();

// File output functions
void printCh5Data(const BranchPath* curNode, const MoveDescription desc, FILE* fp);
void printCh5Sort(const CH5* ch5Data, FILE* fp);
void printCookData(const BranchPath* curNode, const MoveDescription desc, FILE* fp);
void printFileHeader(FILE* fp);
void printInventoryData(const BranchPath* curNode, FILE* fp);
void printOutputsCreated(const BranchPath* curNode, FILE* fp);
void printNodeDescription(const BranchPath * curNode, FILE * fp);
void printResults(const char* filename, const BranchPath* path);
void printSortData(FILE* fp, enum Action curNodeAction);

// Select and random methodology functions
void handleSelectAndRandom(BranchPath* curNode, int select, int randomise);
void shuffleLegalMoves(BranchPath* node);
void softMin(BranchPath *node);

// Sorting functions
int alpha_sort(const void* elem1, const void* elem2);
int alpha_sort_reverse(const void* elem1, const void* elem2);
Inventory getSortedInventory(Inventory inventory, enum Action sort);
int getSortFrames(enum Action action);
void handleSorts(BranchPath* curNode);
int type_sort(const void* elem1, const void* elem2);
int type_sort_reverse(const void* elem1, const void* elem2);

// Frame calculation and optimization functions
void applyJumpStorageFramePenalty(BranchPath *node);
void generateFramesTaken(MoveDescription* description, const BranchPath* node, int framesTaken);
int selectSecondItemFirst(const int* ingredientLoc, int nulls, int viableItems);
void swapItems(int* ingredientLoc);

// General node functions
void freeAllNodes(BranchPath* node);
void freeNode(BranchPath *node);
BranchPath* initializeRoot();

// Serial functions
void serializeNode(BranchPath *node);

// Other
void periodicGithubCheck();
Result calculateOrder(int ID);

#endif
