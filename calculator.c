#include "calculator.h"

#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "absl/base/port.h"
#include "pcg_basic.h"
#include "base.h"
#include "config.h"
#include "FTPManagement.h"
#include "logger.h"
#include "node_print.h"
#include "recipes.h"
#include "shutdown.h"
#include "start.h"


// Constants
#define INVENTORY_SIZE 20

// Frame penalty for a particular action
#define CHOOSE_2ND_INGREDIENT_FRAMES 56
#define TOSS_FRAMES 32
#define ALPHA_ASC_SORT_FRAMES 38
#define ALPHA_DESC_SORT_FRAMES 40
#define TYPE_ASC_SORT_FRAMES 39
#define TYPE_DESC_SORT_FRAMES 41
#define JUMP_STORAGE_NO_TOSS_FRAMES 5		// Penalty for not tossing the last item (because we need to get Jump Storage)

// algorithm parameters
#define BUFFER_SEARCH_FRAMES 150 // Threshold to try optimizing a roadmap to attempt to beat the current record
#define DEFAULT_ITERATION_LIMIT 100000 // Cutoff for iterations explored before resetting
#define ITERATION_LIMIT_INCREASE 100000000 // Amount to increase the iteration limit by when finding a new record
#define CHECK_SHUTDOWN_INTERVAL 30000

static const int INT_OUTPUT_ARRAY_SIZE_BYTES = sizeof(outputCreatedArray_t);
static const outputCreatedArray_t EMPTY_RECIPES = {0};
static const Cook EMPTY_COOK = {0};

// globals
int **invFrames;
Recipe *recipeList;
pcg32_random_t *rng;

/*-------------------------------------------------------------------
 * Function 	: checkShutdownOnIndex
 *
 * Called periodically to check for shutdown. Performs modulo operation
 * before the function call to reduce overhead cost from repeated checks.
 -------------------------------------------------------------------*/
ABSL_ATTRIBUTE_ALWAYS_INLINE static inline bool checkShutdownOnIndex(int i) {
	return i % CHECK_SHUTDOWN_INTERVAL == 0 && askedToShutdown();
}

/*-------------------------------------------------------------------
 * Function 	: copyOutputsFulfilled
 *
 * A simple memcpy to duplicate srcOutputsFulfilled into destOutputsFulfilled
 -------------------------------------------------------------------*/
ABSL_ATTRIBUTE_ALWAYS_INLINE static inline void copyOutputsFulfilled(outputCreatedArray_t dest, const outputCreatedArray_t src) {
	memcpy((void*)dest, (const void*)src, INT_OUTPUT_ARRAY_SIZE_BYTES);
}

/*-------------------------------------------------------------------
 * Function 	: initializeInvFrames
 *
 * Initializes global invFrames, which is used to calculate
 * the number of frames it takes to navigate to an item in the menu.
 -------------------------------------------------------------------*/
void initializeInvFrames() {
	invFrames = getInventoryFrames();
}

/*-------------------------------------------------------------------
 * Function 	: initializeRecipeList
 *
 * Initializes global variable recipeList, which stores data pertaining
 * to each of the 57 recipes, plus a representation of Chapter 5. This
 * data consists of recipe outputs, number of different ways to cook the
 * recipe, and the items required for each recipe combination.
 -------------------------------------------------------------------*/
void initializeRecipeList() {
	recipeList = getRecipeList();
}

/*-------------------------------------------------------------------
 * Function : initializeRoot
 *
 * Generate the root of the tree graph
 -------------------------------------------------------------------*/
ABSL_MUST_USE_RESULT BranchPath* initializeRoot() {
	BranchPath* root = malloc(sizeof(BranchPath));

	checkMallocFailed(root);

	root->moves = 0;
	root->inventory = getStartingInventory();
	root->description.action = EBegin;
	root->description.data = NULL;
	root->description.framesTaken = 0;
	root->description.totalFramesTaken = 0;
	root->prev = NULL;
	root->next = NULL;
	// This will also 0 out all the other elements
	copyOutputsFulfilled(root->outputCreated, EMPTY_RECIPES);
	root->numOutputsCreated = 0;
	root->legalMoves = NULL;
	root->numLegalMoves = 0;
	root->totalSorts = 0;
	return root;
}

void seedThreadRNG(int workerCount) {
	rng = malloc(workerCount * sizeof(pcg32_random_t));
	checkMallocFailed(rng);

	uint64_t initstate = getSysRNG();

	for (int i = 0; i < workerCount; i++) {
		// argument 1 determines the starting RNG value
		// argument 2 determines the RNG sequence
		// See https://www.pcg-random.org/using-pcg-c-basic.html
		uint64_t initseq = (intptr_t)(rng) + i;
		
		pcg32_srandom_r(rng + i, initstate, initseq);
	}
		
}

/*-------------------------------------------------------------------
 * Function : applyJumpStorageFramePenalty
 *
 * Looks at the node's Cook data. If the item is autoplaced, then add
 * a penalty for not tossing the item. Adjust framesTaken and
 * totalFramesTaken to reflect this change.
 -------------------------------------------------------------------*/
void applyJumpStorageFramePenalty(BranchPath *node) {
	Cook* pCook = (Cook*)node->description.data;
	if (pCook->handleOutput == Autoplace) {
		node->description.framesTaken += JUMP_STORAGE_NO_TOSS_FRAMES;
		node->description.totalFramesTaken += JUMP_STORAGE_NO_TOSS_FRAMES;
	}
}

/*-------------------------------------------------------------------
 * Function : createChapter5Struct
 *
 * Compartmentalization of setting CH5 attributes
 * lateSort tracks whether we performed the sort before or after the
 * Keel Mango, for printing purposes
 -------------------------------------------------------------------*/
ABSL_MUST_USE_RESULT CH5 *createChapter5Struct(CH5_Eval eval, int lateSort) {
	CH5 *ch5 = malloc(sizeof(CH5));

	checkMallocFailed(ch5);

	ch5->indexDriedBouquet = eval.DB_place_index;
	ch5->indexCoconut = eval.CO_place_index;
	ch5->ch5Sort = eval.sort;
	ch5->indexKeelMango = eval.KM_place_index;
	ch5->indexCourageShell = eval.CS_place_index;
	ch5->indexThunderRage = eval.TR_use_index;
	ch5->lateSort = lateSort;
	return ch5;
}

/*-------------------------------------------------------------------
 * Function : createCookDescription
 *
 * Compartmentalization of generating a MoveDescription struct
 * based on various parameters dependent on what recipe we're cooking
 -------------------------------------------------------------------*/
MoveDescription createCookDescription(const BranchPath *node, Recipe recipe, ItemCombination combo, Inventory *tempInventory, int viableItems) {
	MoveDescription useDescription;
	useDescription.action = ECook;

	int ingredientLoc[2];

	// Determine the locations of both ingredients
	ingredientLoc[0] = indexOfItemInInventory(*tempInventory, combo.item1);

	if (combo.numItems == 1) {
		createCookDescription1Item(node, recipe, combo, tempInventory, ingredientLoc, viableItems, &useDescription);
	}
	else {
		ingredientLoc[1] = indexOfItemInInventory(*tempInventory, combo.item2);
		createCookDescription2Items(node, recipe, combo, tempInventory, ingredientLoc, viableItems, &useDescription);
	}

	return useDescription;
}

/*-------------------------------------------------------------------
 * Function : createCookDescription1Item
 *
 * Handles inventory management and frame calculation for recipes of
 * length 1. Generates Cook structure and points to this structure
 * in useDescription.
 -------------------------------------------------------------------*/
void createCookDescription1Item(const BranchPath *node, Recipe recipe, ItemCombination combo, Inventory *tempInventory, int *ingredientLoc, int viableItems, MoveDescription *useDescription) {
	// This is a potentially viable recipe with 1 ingredient
	// Determine how many frames will be needed to select that item
	int framesTaken = invFrames[viableItems - 1][ingredientLoc[0] - tempInventory->nulls];

	// Modify the inventory if the ingredient was in the first 10 slots
	if (ingredientLoc[0] < 10) {
		// Shift the inventory towards the front of the array to fill the null
		*tempInventory = removeItem(*tempInventory, ingredientLoc[0]);
	}

	generateCook(useDescription, combo, recipe, ingredientLoc);
	generateFramesTaken(useDescription, node, framesTaken);
}

/*-------------------------------------------------------------------
 * Function : createCookDescription2Items
 *
 * Handles inventory management and frame calculation for recipes of
 * length 2. Swaps items if it's faster to choose the second item first.
 * Generates Cook structure and points to this structure in useDescription.
 -------------------------------------------------------------------*/
void createCookDescription2Items(const BranchPath *node, Recipe recipe, ItemCombination combo, Inventory *tempInventory, int *ingredientLoc, int viableItems, MoveDescription *useDescription) {
	// This is a potentially viable recipe with 2 ingredients
	//Baseline frames based on how many times we need to access the menu
	int framesTaken = CHOOSE_2ND_INGREDIENT_FRAMES;

	// Swap ingredient order if necessary. There are some configurations where
	// it is 2 frames faster to pick the ingredients in the reverse order or
	// only one order is possible.
	if (selectSecondItemFirst(ingredientLoc, tempInventory->nulls, viableItems)) {
		swapItems(ingredientLoc, &combo);
	}

	int visibleLoc0 = ingredientLoc[0] - tempInventory->nulls;
	int visibleLoc1 = ingredientLoc[1] - tempInventory->nulls;

	// Add the frames to choose the first ingredient.
	framesTaken += invFrames[viableItems - 1][visibleLoc0];

	// Depending on the first ingredient's position and the state of the
	// inventory, it or another item could be hidden during the second
	// ingredient selection.
	if (ingredientLoc[0] < 10) {
		--viableItems;
		// If the second ingredient comes after the first, its position will be
		// 1 less.
		if (ingredientLoc[0] < ingredientLoc[1]) {
			--visibleLoc1;
		}
	}
	else if (visibleLoc0 >= 10) {
		--viableItems;
		// If there are nulls in this case, the wrong item is hidden. If the
		// second ingredient comes after the hidden item, its position will be
		// 1 less.
		if (visibleLoc0 < ingredientLoc[1]) {
			--visibleLoc1;
		}
	}
	// Add the frames to choose the second ingredient.
	framesTaken += invFrames[viableItems - 1][visibleLoc1];

	// Set each inventory index to null if the item was in the first 10 slots
	// To reduce complexity, remove the items in ascending order of index
	if (ingredientLoc[0] < ingredientLoc[1]) {
		if (ingredientLoc[0] < 10) {
			*tempInventory = removeItem(*tempInventory, ingredientLoc[0]);
		}
		if (ingredientLoc[1] < 10) {
			*tempInventory = removeItem(*tempInventory, ingredientLoc[1]);
		}
	}
	else {
		if (ingredientLoc[1] < 10) {
			*tempInventory = removeItem(*tempInventory, ingredientLoc[1]);
		}
		if (ingredientLoc[0] < 10) {
			*tempInventory = removeItem(*tempInventory, ingredientLoc[0]);
		}
	}

	// Describe what items were used
	generateCook(useDescription, combo, recipe, ingredientLoc);
	generateFramesTaken(useDescription, node, framesTaken);
}

/*-------------------------------------------------------------------
 * Function : createLegalMove
 *
 * Given the input parameters, allocate and set attributes for a
 * legalMove node, as well as add it to the parent's list of legal moves.
 -------------------------------------------------------------------*/
BranchPath *createLegalMove(BranchPath *node, Inventory inventory, MoveDescription description, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled) {
	BranchPath *newLegalMove = malloc(sizeof(BranchPath));

	checkMallocFailed(newLegalMove);

	newLegalMove->moves = node->moves + 1;
	newLegalMove->inventory = inventory;
	newLegalMove->description = description;
	newLegalMove->prev = node;
	newLegalMove->next = NULL;
	copyOutputsFulfilled(newLegalMove->outputCreated, outputsFulfilled);
	newLegalMove->numOutputsCreated = numOutputsFulfilled;
	newLegalMove->legalMoves = NULL;
	newLegalMove->numLegalMoves = 0;
	if (description.action >= ESort_Alpha_Asc && description.action <= ESort_Type_Des) {
		newLegalMove->totalSorts = node->totalSorts + 1;
	}
	else {
		newLegalMove->totalSorts = node->totalSorts;
	}

	return newLegalMove;
}

/*-------------------------------------------------------------------
 * Function : filterOut2Ingredients
 *
 * For the first node's legal moves, we cannot cook a recipe which
 * contains two items. Thus, we need to remove any legal moves
 * which require two ingredients
 -------------------------------------------------------------------*/
void filterOut2Ingredients(BranchPath *node) {
	for (int i = 0; i < node->numLegalMoves; i++) {
		if (node->legalMoves[i]->description.action == ECook) {
			Cook *cook = node->legalMoves[i]->description.data;
			if (cook->numItems == 2)
				freeAndShiftLegalMove(node, i--);
		}
	}
}

/*-------------------------------------------------------------------
 * Function : finalizeChapter5Eval
 *
 * Given input parameters, construct a new legal move to represent CH5
 -------------------------------------------------------------------*/
void finalizeChapter5Eval(BranchPath *node, Inventory inventory, CH5 *ch5Data, int temp_frame_sum, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled) {
	// Get the index of where to insert this legal move to
	int insertIndex = getInsertionIndex(node, temp_frame_sum);

	MoveDescription description;
	description.action = ECh5;
	description.data = ch5Data;
	description.framesTaken = temp_frame_sum;
	description.totalFramesTaken = node->description.totalFramesTaken + temp_frame_sum;

	// Create the legalMove node
	BranchPath *legalMove = createLegalMove(node, inventory, description, outputsFulfilled, numOutputsFulfilled);

	// Apend the legal move
	insertIntoLegalMoves(insertIndex, legalMove, node);
}

/*-------------------------------------------------------------------
 * Function : finalizeLegalMove
 *
 * Given input parameters, construct a new legal move to represent
 * a valid recipe move. Also checks to see if the legal move exceeds
 * the frame limit
 -------------------------------------------------------------------*/
void finalizeLegalMove(BranchPath *node, MoveDescription useDescription, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, enum HandleOutput tossType, enum Type_Sort toss, int tossIndex) {
	// Determine if the legal move exceeds the frame limit. If so, return out
	if (useDescription.totalFramesTaken > getLocalRecord() + BUFFER_SEARCH_FRAMES)
		return;

	// Determine where to insert this legal move into the list of legal moves (sorted by frames taken)
	int insertIndex = getInsertionIndex(node, useDescription.framesTaken);

	Cook *cookNew = malloc(sizeof(Cook));
	checkMallocFailed(cookNew);

	*cookNew = *((Cook*)useDescription.data);
	cookNew->handleOutput = tossType;
	cookNew->toss = toss;
	cookNew->indexToss = tossIndex;
	useDescription.data = cookNew;

	// Create the legalMove node
	BranchPath *newLegalMove = createLegalMove(node, tempInventory, useDescription, tempOutputsFulfilled, numOutputsFulfilled);

	// Insert this new move into the current node's legalMove array
	insertIntoLegalMoves(insertIndex, newLegalMove, node);
}

/*-------------------------------------------------------------------
 * Function : freeAllNodes
 *
 * We've reached the iteration limit, so free all nodes in the roadmap
 * We additionally need to delete node from the previous node's list of
 * legalMoves to prevent a double-free. Don't cache serials, as we haven't
 * traversed all legal moves.
 -------------------------------------------------------------------*/
void freeAllNodes(BranchPath *node) {
	BranchPath *prevNode = NULL;

	do {
		prevNode = node->prev;

		freeNode(node);

		// Delete node in nextNode's list of legal moves to prevent a double free
		if (prevNode != NULL && prevNode->legalMoves != NULL) {
			prevNode->legalMoves[0] = NULL;
			prevNode->numLegalMoves--;
			shiftUpLegalMoves(prevNode, 1);
		}

		// Traverse to the previous node
		node = prevNode;
	} while (node != NULL);
}

/*-------------------------------------------------------------------
 * Function : freeLegalMove
 *
 * Free the legal move at index in the node's array of legal moves,
 * but unlike freeAndShiftLegalMove(), this does NOT shift the existing
 * legal moves to fill the gap. This is useful in cases where the
 * caller can assure such consistency is not needed (For example,
 * freeing the last legal move or freeing all legal moves).
 -------------------------------------------------------------------*/
static void freeLegalMove(BranchPath *node, int index) {
	freeNode(node->legalMoves[index]);
	node->legalMoves[index] = NULL;
	node->numLegalMoves--;
	node->next = NULL;
}

/*-------------------------------------------------------------------
 * Function : freeAndShiftLegalMove
 *
 * Free the legal move at index in the node's array of legal moves,
 * and shift the existing legal moves after the index to fill the gap.
 -------------------------------------------------------------------*/

void freeAndShiftLegalMove(BranchPath *node, int index) {
	freeLegalMove(node, index);
	shiftUpLegalMoves(node, index + 1);
}

/*-------------------------------------------------------------------
 * Function : freeNode
 *
 * Free the current node and all legal moves within the node
 -------------------------------------------------------------------*/
void freeNode(BranchPath *node) {
	if (node->description.data != NULL) {
		free(node->description.data);
	}
	if (node->legalMoves != NULL) {
		const int max = node->numLegalMoves;
		int i = 0;
		while (i < max) {
			// Don't need to worry about shifting up when we do this.
			// Or resetting slots to NULL.
			// We are blowing it all away anyways.
			freeLegalMove(node, i++);
		}
		free(node->legalMoves);
	}
	
	free(node);
}

/*-------------------------------------------------------------------
 * Function : fulfillChapter5
 *
 * Count up frames from Hot Dog and Mousse Cake, then determine how
 * to continue evaluating Chapter 5 based on null count.
 -------------------------------------------------------------------*/
void fulfillChapter5(BranchPath *curNode) {
	// Create an outputs chart but with the Dried Bouquet collected
	// to ensure that the produced inventory can fulfill all remaining recipes
	outputCreatedArray_t tempOutputsFulfilled;
	copyOutputsFulfilled(tempOutputsFulfilled, curNode->outputCreated);
	tempOutputsFulfilled[getOutputIndex(Dried_Bouquet)] = true;
	int numOutputsFulfilled = curNode->numOutputsCreated + 1;

	Inventory newInventory = curNode->inventory;

	int mousse_cake_index = indexOfItemInInventory(newInventory, Mousse_Cake);

	// Create the CH5 eval struct
	CH5_Eval eval;

	// Explicit int casts are to silence warnings.
	int viableItems = (int)newInventory.length - newInventory.nulls - min((int)newInventory.length - 10, newInventory.nulls);

	// Calculate frames it takes the navigate to the Mousse Cake and the Hot Dog for the trade
	eval.frames_at_HD = 2 * invFrames[viableItems - 1][indexOfItemInInventory(newInventory, Hot_Dog) - newInventory.nulls];
	eval.frames_at_MC = eval.frames_at_HD
		+ invFrames[viableItems - 1][mousse_cake_index - newInventory.nulls];

	// If the Mousse Cake is in the first 10 slots, change it to NULL
	if (mousse_cake_index < 10) {
		newInventory = removeItem(newInventory, mousse_cake_index);
	}

	// Handle allocation of the first 2 CH5 items (Dried Bouquet and Coconut)
	switch (newInventory.nulls) {
		case 0 :
			handleDBCOAllocation0Nulls(curNode, newInventory, tempOutputsFulfilled, numOutputsFulfilled, eval);
			break;
		case 1 :
			handleDBCOAllocation1Null(curNode, newInventory, tempOutputsFulfilled, numOutputsFulfilled, eval);
			break;
		default :
			handleDBCOAllocation2Nulls(curNode, newInventory, tempOutputsFulfilled, numOutputsFulfilled, eval);
	}
}

/*-------------------------------------------------------------------
 * Function : fulfillRecipes
 *
 * Iterate through all possible combinations of cooking different
 * recipes and create legal moves for them
 -------------------------------------------------------------------*/
void fulfillRecipes(BranchPath *curNode) {
	// Only evaluate the 57th recipe (Mistake) when it's the last recipe to fulfill
	// This is because it is relatively easy to craft this output with many of the previous outputs, and will take minimal frames
	int upperOutputLimit = (curNode->numOutputsCreated == NUM_RECIPES - 1) ? NUM_RECIPES : (NUM_RECIPES - 1);

	// Iterate through all recipe ingredient combos
	for (int recipeIndex = 0; recipeIndex < upperOutputLimit; recipeIndex++) {
		// Only want recipes that haven't been fulfilled
		if (curNode->outputCreated[recipeIndex]) {
			continue;
		}

		// Dried Bouquet (Recipe index 56) represents the Chapter 5 intermission
		// Don't actually use the specified recipe, as it is handled later
		if (recipeIndex == getOutputIndex(Dried_Bouquet)) {
			continue;
		}

		// Only want ingredient combos that can be fulfilled right now!
		Recipe recipe = recipeList[recipeIndex];
		ItemCombination *combos = recipe.combos;
		for (int comboIndex = 0; comboIndex < recipe.countCombos; comboIndex++) {
			ItemCombination combo = combos[comboIndex];
			if (!itemComboInInventory(combo, curNode->inventory)) {
				continue;
			}

			// This is a recipe that can be fulfilled right now!

			// Copy the inventory
			Inventory newInventory = curNode->inventory;

			// Mark that this output has been fulfilled for viability determination
			outputCreatedArray_t tempOutputsFulfilled;
			copyOutputsFulfilled(tempOutputsFulfilled, curNode->outputCreated);
			tempOutputsFulfilled[recipeIndex] = 1;
			int numOutputsFulfilled = curNode->numOutputsCreated + 1;

			// How many items there are to choose from (Not NULL or hidden)
			int viableItems = newInventory.length - newInventory.nulls - min(newInventory.length - 10, newInventory.nulls);

			MoveDescription useDescription = createCookDescription(curNode, recipe, combo, &newInventory, viableItems);

			// Store the base useDescription's cook pointer to be freed later
			Cook *cookBase = (Cook *)useDescription.data;

			// Handle allocation of the output
			handleRecipeOutput(curNode, newInventory, useDescription, tempOutputsFulfilled, numOutputsFulfilled, recipe.output, viableItems);

			free(cookBase);
			// We know tempOutputsFulfilled does not escape this scope, so safe to be unallocated on return.
		}
	}
}

/*-------------------------------------------------------------------
 * Function : generateCook
 *
 * Given input parameters, generate Cook structure to represent
 * what was cooked and how.
 -------------------------------------------------------------------*/
void generateCook(MoveDescription *description, const ItemCombination combo, const Recipe recipe, const int *ingredientLoc) {
	Cook *cook = malloc(sizeof(Cook));

	checkMallocFailed(cook);

	description->action = ECook;
	cook->numItems = combo.numItems;
	cook->item1 = combo.item1;
	cook->item2 = combo.item2;

	cook->itemIndex1 = ingredientLoc[0];
	cook->itemIndex2 = ingredientLoc[1];
	cook->output = recipe.output;
	description->data = cook;
}

/*-------------------------------------------------------------------
 * Function : generateFramesTaken
 *
 * Assign frame duration to description structure and reference the
 * previous node to find the total frame duration for the roadmap thus far
 -------------------------------------------------------------------*/
void generateFramesTaken(MoveDescription *description, const BranchPath *node, int framesTaken) {
	description->framesTaken = framesTaken;
	description->totalFramesTaken = node->description.totalFramesTaken + framesTaken;
}

/*-------------------------------------------------------------------
 * Function : getInsertionIndex
 *
 * Based on the frames it takes to complete a new legal move, find out
 * where to insert it in the current node's array of legal moves, which
 * is ordered based on frame count ascending
 -------------------------------------------------------------------*/
int getInsertionIndex(const BranchPath *curNode, int frames) {
	if (curNode->legalMoves == NULL) {
		return 0;
	}
	int tempIndex = 0;
	while (tempIndex < curNode->numLegalMoves && frames > curNode->legalMoves[tempIndex]->description.framesTaken) {
		tempIndex++;
	}

	return tempIndex;
}

/*-------------------------------------------------------------------
 * Function : getSortFrames
 *
 * Depending on the type of sort, return the corresponding frame cost.
 -------------------------------------------------------------------*/
int getSortFrames(enum Action action) {
	switch (action) {
		case ESort_Alpha_Asc:
			return ALPHA_ASC_SORT_FRAMES;
		case ESort_Alpha_Des:
			return ALPHA_DESC_SORT_FRAMES;
		case ESort_Type_Asc:
			return TYPE_ASC_SORT_FRAMES;
		case ESort_Type_Des:
			return TYPE_DESC_SORT_FRAMES;
		default:
			// Critical error if we reach this point...
			// action should be some type of sort
			exit(-2);
	}
}

/*-------------------------------------------------------------------
 * Function : outputOrderIsSlower
 *
 * In a case where an output is manually placed, if the item it
 * replaces has not been used since the placement of a previous
 * output, then the items the outputs replaced can be swapped.
 * As outputs are placed at the beginning, the lower index would be
 * changed by 1, so there can be a time difference of 2 frames.
 * This function determines whether the current order is slower and
 * the outputs should be swapped. If the orders take the same time
 * (possible when the inventory length is even and less than 20),
 * this will still return 1 for one order and 0 for the other.
 -------------------------------------------------------------------*/
int outputOrderIsSlower(int location_1, int location_2, int inventoryLength) {
	int middle = inventoryLength / 2;
	if (location_1 < middle) {
		// The first location will be selected by going down, so we want
		// to minimize its position.
		return location_1 >= location_2;
	}
	if (location_2 > middle) {
		// The second location will be selected by going up, so we want
		// to maximize its position.
		return location_2 > location_1;
	}
	return 1;
}

/*-------------------------------------------------------------------
 * Function : handleChapter5EarlySortEndItems
 *
 * A sort already occurred right after placing the Coconut. Evaluate
 * each combination of Keel Mango and Courage Shell placements,
 * handle using the Thunder Rage on Smorg,, and if everything is valid,
 * create the move and insert it.
 -------------------------------------------------------------------*/
void handleChapter5EarlySortEndItems(BranchPath *node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	for (eval.KM_place_index = 0; eval.KM_place_index < 10; eval.KM_place_index++) {
		// Don't allow a move that will be invalid.
		if (inventory.inventory[eval.KM_place_index] == Thunder_Rage
			|| inventory.inventory[eval.KM_place_index] == Dried_Bouquet) {
			continue;
		}

		// Replace the chosen item with the Keel Mango
		Inventory km_temp_inventory = replaceItem(inventory, eval.KM_place_index, Keel_Mango);
		// Calculate the frames for this action
		eval.frames_at_KM = eval.frames_at_sort + TOSS_FRAMES
			+ invFrames[inventory.length][eval.KM_place_index + 1];

		for (eval.CS_place_index = 1; eval.CS_place_index < 10; eval.CS_place_index++) {
			// Don't allow a move that will be invalid or needlessly slower.
			if (km_temp_inventory.inventory[eval.CS_place_index] == Thunder_Rage
				|| km_temp_inventory.inventory[eval.CS_place_index] == Dried_Bouquet
				|| outputOrderIsSlower(eval.KM_place_index, eval.CS_place_index, km_temp_inventory.length)) {
				continue;
			}

			// Replace the chosen item with the Courage Shell
			Inventory kmcs_temp_inventory = replaceItem(km_temp_inventory, eval.CS_place_index, Courage_Shell);
			// Calculate the frames for this action
			eval.frames_at_CS = eval.frames_at_KM + TOSS_FRAMES
				+ invFrames[kmcs_temp_inventory.length][eval.CS_place_index + 1];

			// The next event is using the Thunder Rage item before resuming the 2nd session of recipe fulfillment
			eval.TR_use_index = indexOfItemInInventory(kmcs_temp_inventory, Thunder_Rage);
			if (eval.TR_use_index < 10) {
				kmcs_temp_inventory = removeItem(kmcs_temp_inventory, eval.TR_use_index);
			}
			// Calculate the frames for this action
			eval.frames_at_TR = eval.frames_at_CS
				+ invFrames[kmcs_temp_inventory.length - 1][eval.TR_use_index];

			// Determine if the remaining inventory is sufficient to fulfill all remaining recipes
			if (stateOK(kmcs_temp_inventory, outputsFulfilled, recipeList)) {
				CH5 *ch5Data = createChapter5Struct(eval, 0);
				finalizeChapter5Eval(node, kmcs_temp_inventory, ch5Data, eval.frames_at_TR, outputsFulfilled, numOutputsFulfilled);
			}
		}
	}
}

/*-------------------------------------------------------------------
 * Function : handleChapter5Eval
 *
 * Branch into early and late sort scenarios. For late sort, handle
 * Keel Mango placement here. Otherwise, defer to other functions.
 -------------------------------------------------------------------*/
void handleChapter5Eval(BranchPath *node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// Evaluate sorting before the Keel Mango
	// Use -1 to identify that we are not collecting the Keel Mango until after the sort
	eval.KM_place_index = -1;
	handleChapter5Sorts(node, inventory, outputsFulfilled, numOutputsFulfilled, eval);

	// Place the Keel Mango in a null spot if one is available.
	if (inventory.nulls >= 1) {
		// Making a copy of the temp inventory for what it looks like after the allocation of the KM
		Inventory km_temp_inventory = addItem(inventory, Keel_Mango);
		eval.frames_at_KM = eval.frames_at_CO;
		eval.KM_place_index = 0;

		// Perform all sorts
		handleChapter5Sorts(node, km_temp_inventory, outputsFulfilled, numOutputsFulfilled, eval);

	}
	else {
		// Place the Keel Mango starting after the other placed items.
		for (eval.KM_place_index = 2; eval.KM_place_index < 10; eval.KM_place_index++) {
			// Don't allow a move that will be invalid.
			if (inventory.inventory[eval.KM_place_index] == Thunder_Rage) {
				continue;
			}

			// Making a copy of the temp inventory for what it looks like after the allocation of the KM
			Inventory km_temp_inventory = replaceItem(inventory, eval.KM_place_index, Keel_Mango);
			// Calculate the frames for this action
			eval.frames_at_KM = eval.frames_at_CO + TOSS_FRAMES
				+ invFrames[inventory.length][eval.KM_place_index + 1];

			// Perform all sorts
			handleChapter5Sorts(node, km_temp_inventory, outputsFulfilled, numOutputsFulfilled, eval);
		}
	}
}

/*-------------------------------------------------------------------
 * Function : handleChapter5LateSortEndItems
 *
 * A sort already occurred right after placing the Keel Mango.
 * Evaluate each Courage Shell placement, handle using the Thunder
 * Rage, and if everything is valid, create the move and insert it.
 -------------------------------------------------------------------*/
void handleChapter5LateSortEndItems(BranchPath *node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// Place the Courage Shell
	for (eval.CS_place_index = 0; eval.CS_place_index < 10; eval.CS_place_index++) {
		// Don't allow a move that will be invalid.
		if (inventory.inventory[eval.CS_place_index] == Dried_Bouquet
			|| inventory.inventory[eval.CS_place_index] == Keel_Mango
			|| inventory.inventory[eval.CS_place_index] == Thunder_Rage) {
			continue;
		}

		// Replace the chosen item with the Courage Shell
		Inventory cs_temp_inventory = replaceItem(inventory, eval.CS_place_index, Courage_Shell);
		// Calculate the frames for this action
		eval.frames_at_CS = eval.frames_at_sort + TOSS_FRAMES
			+ invFrames[cs_temp_inventory.length][eval.CS_place_index + 1];

		// The next event is using the Thunder Rage
		eval.TR_use_index = indexOfItemInInventory(cs_temp_inventory, Thunder_Rage);
		// Using the Thunder Rage in slots 1-10 will remove it
		if (eval.TR_use_index < 10) {
			cs_temp_inventory = removeItem(cs_temp_inventory, eval.TR_use_index);
		}
		// Calculate the frames for this action
		eval.frames_at_TR = eval.frames_at_CS
			+ invFrames[cs_temp_inventory.length - 1][eval.TR_use_index];

		if (stateOK(cs_temp_inventory, outputsFulfilled, recipeList)) {
			CH5 *ch5Data = createChapter5Struct(eval, 1);
			finalizeChapter5Eval(node, cs_temp_inventory, ch5Data, eval.frames_at_TR, outputsFulfilled, numOutputsFulfilled);
		}
	}
}

/*-------------------------------------------------------------------
 * Function : handleChapter5Sorts
 *
 * Evaluate each sorting method to make sure Coconut will be
 * duplicated, and then branch to the appropriate end items function
 * based on whether Keel Mango has already been placed or not.
 -------------------------------------------------------------------*/
void handleChapter5Sorts(BranchPath *node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	for (eval.sort = ESort_Alpha_Asc; eval.sort <= ESort_Type_Des; eval.sort++) {
		Inventory sorted_inventory = getSortedInventory(inventory, eval.sort);

		// Only bother with further evaluation if the sort placed the Coconut in the latter half of the inventory
		// because the Coconut is needed for duplication
		if (indexOfItemInInventory(sorted_inventory, Coconut) < 10) {
			continue;
		}

		// Count the frames this sort takes.
		int sort_frames = getSortFrames(eval.sort);

		// Evaluate final items, including Keel Mango if necessary.
		if (eval.KM_place_index == -1) {
			eval.frames_at_sort = eval.frames_at_CO + sort_frames;
			handleChapter5EarlySortEndItems(node, sorted_inventory, outputsFulfilled, numOutputsFulfilled, eval);
			continue;
		}

		eval.frames_at_sort = eval.frames_at_KM + sort_frames;
		handleChapter5LateSortEndItems(node, sorted_inventory, outputsFulfilled, numOutputsFulfilled, eval);
	}
}

/*-------------------------------------------------------------------
 * Function : handleDBCOAllocation0Nulls
 *
 * There are no nulls, so neither Dried Bouquet nor Coconut will be
 * autoplaced. Evaluate each combination of placements for them, and
 * then proceed to the early/late sort decision.
 -------------------------------------------------------------------*/
void handleDBCOAllocation0Nulls(BranchPath *curNode, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// Place the Dried Bouquet.
	for (eval.DB_place_index = 0; eval.DB_place_index < 10; eval.DB_place_index++) {
		// Don't allow a move that will be invalid.
		if (tempInventory.inventory[eval.DB_place_index] == Thunder_Rage) {
			continue;
		}

		// Replace the chosen item with the Dried Bouquet
		Inventory db_temp_inventory = replaceItem(tempInventory, eval.DB_place_index, Dried_Bouquet);
		// Calculate the frames for this action
		eval.frames_at_DB = eval.frames_at_MC + TOSS_FRAMES
			+ invFrames[tempInventory.length][eval.DB_place_index + 1];

		// Place the Coconut after the Dried Bouquet.
		for (eval.CO_place_index = 1; eval.CO_place_index < 10; eval.CO_place_index++) {
			// Don't allow a move that will be invalid or needlessly slower.
			if (db_temp_inventory.inventory[eval.CO_place_index] == Thunder_Rage
				|| outputOrderIsSlower(eval.DB_place_index, eval.CO_place_index, db_temp_inventory.length)) {
				continue;
			}

			// Replace the chosen item with the Coconut
			Inventory dbco_temp_inventory = replaceItem(db_temp_inventory, eval.CO_place_index, Coconut);

			// Calculate the frames of this action
			eval.frames_at_CO = eval.frames_at_DB + TOSS_FRAMES
				+ invFrames[tempInventory.length][eval.CO_place_index + 1];

			// Handle the allocation of the Coconut sort, Keel Mango, and Courage Shell
			handleChapter5Eval(curNode, dbco_temp_inventory, tempOutputsFulfilled, numOutputsFulfilled, eval);
		}
	}
}

/*-------------------------------------------------------------------
 * Function : handleDBCOAllocation1Null
 *
 * There is 1 null, so Dried Bouquet will be autoplaced. Evaluate
 * each Coconut placement, and then proceed to the early/late sort
 * decision.
 -------------------------------------------------------------------*/
void handleDBCOAllocation1Null(BranchPath *curNode, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// The Dried Bouquet gets auto-placed in the 1st slot,
	// and everything else gets shifted down one to fill the first NULL
	tempInventory = addItem(tempInventory, Dried_Bouquet);
	eval.DB_place_index = 0;
	eval.frames_at_DB = eval.frames_at_MC;

	// Dried Bouquet will always be in the first slot
	for (eval.CO_place_index = 1; eval.CO_place_index < 10; eval.CO_place_index++) {
		// Don't allow a move that will be invalid.
		if (tempInventory.inventory[eval.CO_place_index] == Thunder_Rage) {
			continue;
		}

		// Replace the item with the Coconut
		Inventory co_temp_inventory = replaceItem(tempInventory, eval.CO_place_index, Coconut);
		// Calculate the number of frames needed to pick this slot for replacement
		eval.frames_at_CO = eval.frames_at_DB + TOSS_FRAMES
			+ invFrames[tempInventory.length][eval.CO_place_index + 1];

		// Handle the allocation of the Coconut sort, Keel Mango, and Courage Shell
		handleChapter5Eval(curNode, co_temp_inventory, tempOutputsFulfilled, numOutputsFulfilled, eval);
	}
}

/*-------------------------------------------------------------------
 * Function : handleDBCOAllocation2Nulls
 *
 * There are at least 2 nulls, so both Dried Bouquet and Coconut
 * will be autoplaced. Proceed to the early/late sort decision.
 -------------------------------------------------------------------*/
void handleDBCOAllocation2Nulls(BranchPath *curNode, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// The Dried Bouquet gets auto-placed due to having nulls
	tempInventory = addItem(tempInventory, Dried_Bouquet);
	eval.DB_place_index = 0;
	eval.frames_at_DB = eval.frames_at_MC;

	// The Coconut gets auto-placed due to having nulls
	tempInventory = addItem(tempInventory, Coconut);
	eval.CO_place_index = 0;
	eval.frames_at_CO = eval.frames_at_DB;

	// Handle the allocation of the Coconut, Sort, Keel Mango, and Courage Shell
	handleChapter5Eval(curNode, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, eval);
}

/*-------------------------------------------------------------------
 * Function : handleRecipeOutput
 *
 * After detecting that a recipe can be satisfied, see how we can handle
 * the output (either tossing the output, auto-placing it if there is a
 * null slot, or tossing a different item in the inventory)
 -------------------------------------------------------------------*/
void handleRecipeOutput(BranchPath *curNode, Inventory tempInventory, MoveDescription useDescription, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, enum Type_Sort output, int viableItems) {
	// Options vary by whether there are NULLs within the inventory
	if (tempInventory.nulls >= 1) {
		tempInventory = addItem(tempInventory, ((Cook*)useDescription.data)->output);

		// Check to see if this state is viable
		if(stateOK(tempInventory, tempOutputsFulfilled, recipeList)) {
			finalizeLegalMove(curNode, useDescription, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, Autoplace, -1, -1);
		}
	}
	else {
		// There are no NULLs in the inventory. Something must be tossed
		// Total number of frames increased by forcing to toss something
		useDescription.framesTaken += TOSS_FRAMES;
		useDescription.totalFramesTaken += TOSS_FRAMES;

		// Evaluate the viability of tossing all current inventory items
		// Assumed that it is impossible to toss and replace any items in the last 10 positions
		tryTossInventoryItem(curNode, tempInventory, useDescription, tempOutputsFulfilled, numOutputsFulfilled, output, viableItems);

		// Evaluate viability of tossing the output item itself
		if (stateOK(tempInventory, tempOutputsFulfilled, recipeList)) {
			finalizeLegalMove(curNode, useDescription, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, Toss, output, -1);
		}
	}
}

/*-------------------------------------------------------------------
 * Function : handleLegalMoveSelection
 *
 * Using the given method, select a legal move to explore next and
 * place it at the beginning of the array.
 -------------------------------------------------------------------*/
void handleLegalMoveSelection(BranchPath *curNode, enum SelectionMethod method) {
	// Somewhat random process of picking the quicker moves to recurse down
	// Arbitrarily skip over the fastest legal move with a given probability
	if (method == Exponential && curNode->moves < 55 && curNode->numLegalMoves > 0) {
		int nextMoveIndex = 0;
		while (nextMoveIndex < curNode->numLegalMoves - 1 && pcg32_random_r(&rng[omp_get_thread_num()]) % 2) {
			nextMoveIndex++;
		}

		// Take the legal move at nextMoveIndex and move it to the front of the array
		BranchPath *nextMove = curNode->legalMoves[nextMoveIndex];
		curNode->legalMoves[nextMoveIndex] = NULL;
		shiftDownLegalMoves(curNode, 0, nextMoveIndex);
		curNode->legalMoves[0] = nextMove;
	}
	// When opting for randomization, shuffle the entire list of legal moves
	else if (method == Random) {
		shuffleLegalMoves(curNode);
	}
}

/*-------------------------------------------------------------------
 * Function : handleSorts
 *
 * Perform the 4 different sorts, determine if they changed the inventory,
 * and if so, generate a legal move to represent the sort.
 -------------------------------------------------------------------*/
void handleSorts(BranchPath *curNode) {
	// Limit the number of sorts allowed in a roadmap
	if (curNode->totalSorts < 10) {
		// Perform the 4 different sorts
		for (enum Action sort = ESort_Alpha_Asc; sort <= ESort_Type_Des; sort++) {
			Inventory sorted_inventory = getSortedInventory(curNode->inventory, sort);

			// Only add the legal move if the sort actually changes the inventory
			if (compareInventories(sorted_inventory, curNode->inventory) == 0) {
				MoveDescription description;
				description.action = sort;
				description.data = NULL;
				int sortFrames = getSortFrames(sort);
				generateFramesTaken(&description, curNode, sortFrames);
				description.framesTaken = sortFrames;
				// Create the legalMove node
				BranchPath *newLegalMove = createLegalMove(curNode, sorted_inventory, description, curNode->outputCreated, curNode->numOutputsCreated);

				// Insert this new move into the current node's legalMove array
				insertIntoLegalMoves(curNode->numLegalMoves, newLegalMove, curNode);
			}
		}
	}
}

/*-------------------------------------------------------------------
 * Function : insertIntoLegalMoves
 *
 * Determine where in curNode's legalmove array the new legal move should
 * be inserted. This is necessary because our current implementation
 * arranges legal moves in ascending order based on the number of frames
 * it takes to complete the legal move.
 -------------------------------------------------------------------*/
void insertIntoLegalMoves(int insertIndex, BranchPath *newLegalMove, BranchPath *curNode) {
	// Reallocate the legalMove array to make room for a new legal move
	BranchPath **temp = realloc(curNode->legalMoves, sizeof(BranchPath*) * ((size_t)curNode->numLegalMoves + 1));

	checkMallocFailed(temp);

	curNode->legalMoves = temp;

	// Shift all legal moves further down the array to make room for a new legalMove
	shiftDownLegalMoves(curNode, insertIndex, curNode->numLegalMoves);
	/*for (int i = curNode->numLegalMoves - 1; i >= insertIndex; i--) {
		curNode->legalMoves[i+1] = curNode->legalMoves[i];
	}*/

	// Place newLegalMove in index insertIndex
	curNode->legalMoves[insertIndex] = newLegalMove;

	// Increase numLegalMoves
	curNode->numLegalMoves++;
}

/*-------------------------------------------------------------------
 * Function : copyAllNodes
 *
 * Duplicate all the contents of a roadmap to a new memory region.
 * This is used for optimizeRoadmap.
 -------------------------------------------------------------------*/
BranchPath *copyAllNodes(BranchPath *newNode, const BranchPath *oldNode) {
	do {
		newNode->moves = oldNode->moves;
		newNode->inventory = oldNode->inventory;
		newNode->description = oldNode->description;
		switch (newNode->description.action) {
			case EBegin:
			case ESort_Alpha_Asc:
			case ESort_Alpha_Des:
			case ESort_Type_Asc:
			case ESort_Type_Des:
				newNode->description.data = NULL;
				break;
			case ECook:
				newNode->description.data = malloc(sizeof(Cook));

				checkMallocFailed(newNode->description.data);

				*((Cook*) newNode->description.data) = *((Cook*) oldNode->description.data);
				break;
			case ECh5:
				newNode->description.data = malloc(sizeof(CH5));

				checkMallocFailed(newNode->description.data);

				CH5 *newData = (CH5 *)newNode->description.data;
				CH5 *oldData = (CH5 *)oldNode->description.data;
				newData->indexDriedBouquet = oldData->indexDriedBouquet;
				newData->indexCoconut =  oldData->indexCoconut;
				newData->ch5Sort = oldData->ch5Sort;
				newData->indexKeelMango = oldData->indexKeelMango;
				newData->indexCourageShell = oldData->indexCourageShell;
				newData->indexThunderRage = oldData->indexThunderRage;
				newData->lateSort = oldData->lateSort;
				break;
			default:
				break;
		}

		copyOutputsFulfilled(newNode->outputCreated, oldNode->outputCreated);
		newNode->numOutputsCreated = oldNode->numOutputsCreated;
		newNode->legalMoves = NULL;
		newNode->numLegalMoves = 0;
		if (newNode->numOutputsCreated < NUM_RECIPES) {
			newNode->next = malloc(sizeof(BranchPath));

			checkMallocFailed(newNode->next);

			newNode->next->prev = newNode;
			newNode = newNode->next;
		}
		else {
			newNode->next = NULL;
		}

		oldNode = oldNode->next;
	} while (oldNode != NULL);

	// Returns the deepest node in the tree after copying everything
	return newNode;
}

/*-------------------------------------------------------------------
 * Function : optimizeRoadmap
 *
 * Given a complete roadmap, attempt to rearrange recipes such that they
 * are placed in more efficient locations in the roadmap. This is effective
 * in shaving off upwards of 100 frames off of a roadmap.
 -------------------------------------------------------------------*/
OptimizeResult optimizeRoadmap(const BranchPath *root) {
	// First copy all nodes to new memory locations so we can begin rearranging nodes
	const BranchPath *curNode = root;
	BranchPath *newRoot = malloc(sizeof(BranchPath));
	checkMallocFailed(newRoot);

	newRoot->prev = NULL;

	// newNode is now the leaf of the tree (for easy list manipulation later)
	BranchPath *newNode = copyAllNodes(newRoot, curNode);

	// List of recipes that can be potentially rearranged into a better location within the roadmap
	enum Type_Sort rearranged_recipes[NUM_RECIPES];

	// Ignore the last recipe as the mistake can almost always be cooked last
	newNode = newNode->prev;

	// Determine which steps can be rearranged
	int num_rearranged_recipes = removeRecipesForReallocation(newNode, rearranged_recipes);

	// Now that all rearranged items have been removed,
	// find the optimal place they can be inserted again, such that they don't affect the inventory
	reallocateRecipes(newRoot, rearranged_recipes, num_rearranged_recipes);

	// All items have been rearranged and placed into a new roadmap
	// Recalculate the total frame count
	for (newNode = newRoot; newNode->next != NULL; newNode = newNode->next) {
		newNode->next->description.totalFramesTaken = newNode->next->description.framesTaken + newNode->description.totalFramesTaken;
	}

	struct OptimizeResult result;
	result.root = newRoot;
	result.last = newNode;
	return result;
}

/*-------------------------------------------------------------------
 * Function : popAllButFirstLegalMove
 *
 * In the event we are within the last few nodes of the roadmap, get rid
 * of all but the fastest legal move.
 -------------------------------------------------------------------*/
void popAllButFirstLegalMove(struct BranchPath *node) {
	while (node->numLegalMoves > 1)
		freeLegalMove(node, node->numLegalMoves - 1);
}

/*-------------------------------------------------------------------
 * Function : reallocateRecipes
 *
 * Given a set of recipes, find alternative places in the roadmap to
 * cook these recipes such that we minimize the frame cost.
 * Note: Even though newRoot is never modified in this function,
 * it may be set as the pointer to newly allocated nodes, and thus is not const
 -------------------------------------------------------------------*/
void reallocateRecipes(BranchPath* newRoot, const enum Type_Sort* rearranged_recipes, int num_rearranged_recipes) {
	for (int recipe_offset = 0; recipe_offset < num_rearranged_recipes; recipe_offset++) {
		// Establish a default bound for the optimal place for this item
		int record_frames = 9999;
		BranchPath *record_placement_node = NULL;
		Cook *record_description = NULL;
		Cook temp_description = {0};

		// Evaluate all recipes and determine the optimal recipe and location
		int recipe_index = getOutputIndex(rearranged_recipes[recipe_offset]);
		Recipe recipe = recipeList[recipe_index];
		for (int recipe_combo_index = 0; recipe_combo_index < recipe.countCombos; recipe_combo_index++) {
			ItemCombination combo = recipe.combos[recipe_combo_index];

			// Evaluate placing after each node where it can be placed
			for (struct BranchPath *mutablePlacement = combo.numItems == 2 ? newRoot->next : newRoot;
			     mutablePlacement != NULL; mutablePlacement = mutablePlacement->next) {
				// Prefer to work with the const version when possible to ensure we really don't modify it.
				// It only has to be non-const because of assignment into
				const struct BranchPath *placement = mutablePlacement;
				// Only want moments when there are no NULLs in the inventory
				if (placement->inventory.nulls) {
					continue;
				}

				// Only want recipes where all ingredients are in the last 10 slots of the evaluated inventory
				int indexItem1 = indexOfItemInInventory(placement->inventory, combo.item1);
				int indexItem2 = -1;
				if (indexItem1 < 10) {
					continue;
				}
				if (combo.numItems == 2) {
					indexItem2 = indexOfItemInInventory(placement->inventory, combo.item2);
					if (indexItem2 < 10) {
						continue;
					}
				}

				// This is a valid recipe and location to fulfill (and toss) the output
				// Calculate the frames needed to produce this step
				int temp_frames = TOSS_FRAMES;
				temp_description = EMPTY_COOK;
				temp_description.output = recipe.output;
				temp_description.handleOutput = Toss;

				if (combo.numItems == 1) {
					// Only one ingredient to navigate to
					temp_frames += invFrames[placement->inventory.length - 1][indexItem1];
					temp_description.numItems = 1;
					temp_description.item1 = combo.item1;
					temp_description.itemIndex1 = indexItem1;
					temp_description.item2 = -1;
					temp_description.itemIndex2 = -1;
				}
				else {
					if (indexItem2 < 0) {
						exitWithUserAcknowledgement("Fatal error! indexItem2 was not set in a branch where it should have.\n");
					}
					// Two ingredients to navigate to, but order matters
					// Pick the larger-index number ingredient first, as it will reduce
					// the frames needed to reach the other ingredient
					temp_frames += CHOOSE_2ND_INGREDIENT_FRAMES;
					temp_description.numItems = 2;

					if (indexItem1 > indexItem2) {
						temp_frames += invFrames[placement->inventory.length - 1][indexItem1];
						temp_frames += invFrames[placement->inventory.length - 2][indexItem2];
						temp_description.item1 = combo.item1;
						temp_description.itemIndex1 = indexItem1;
						temp_description.item2 = combo.item2;
						temp_description.itemIndex2 = indexItem2;
					}
					else {
						temp_frames += invFrames[placement->inventory.length - 1][indexItem2];
						temp_frames += invFrames[placement->inventory.length - 2][indexItem1];
						temp_description.item1 = combo.item2;
						temp_description.itemIndex1 = indexItem2;
						temp_description.item2 = combo.item1;
						temp_description.itemIndex2 = indexItem1;
					}
				}

				// Compare the current placement to the current record
				if (temp_frames < record_frames) {
					// Update the record information
					record_frames = temp_frames;
					record_placement_node = mutablePlacement;

					if (record_description == NULL) {
						record_description = malloc(sizeof(Cook));
						checkMallocFailed(record_description);
					}
					*record_description = temp_description;
				}
			}
		}

		// All recipe combos and intervals have been evaluated
		// Insert the optimized output in the designated interval
		if (record_placement_node == NULL) {
			// This is an error
			recipeLog(7, "Calculator", "Roadmap", "Optimize", "OptimizeRoadmap couldn't find a valid placement...");
			exit(1);
			return;  // Never reached; here to let compiler know the function does not continue after this.
		}

		BranchPath *insertNode = malloc(sizeof(BranchPath));
		checkMallocFailed(insertNode);

		// Set pointers to and from surrounding structs
		insertNode->prev = record_placement_node;
		record_placement_node->next->prev = insertNode;
		insertNode->next = record_placement_node->next;
		record_placement_node->next = insertNode;

		// Initialize the new node
		insertNode->moves = record_placement_node->moves + 1;
		insertNode->inventory = record_placement_node->inventory;
		insertNode->description.action = ECook;
		insertNode->description.data = (void *)record_description;
		insertNode->description.framesTaken = record_frames;
		copyOutputsFulfilled(insertNode->outputCreated, record_placement_node->outputCreated);
		insertNode->outputCreated[recipe_index] = true;
		insertNode->numOutputsCreated = record_placement_node->numOutputsCreated + 1;
		insertNode->legalMoves = NULL;
		insertNode->numLegalMoves = 0;

		// Update all subsequent nodes with
		for (BranchPath *node = insertNode->next; node!= NULL; node = node->next) {
			node->outputCreated[recipe_index] = true;
			++node->numOutputsCreated;
			++node->moves;
		}
	}
}

/*-------------------------------------------------------------------
 * Function : removeRecipesForReallocation
 *
 * Look through a completed roadmap to find recipes which can be
 * performed elsewhere in the roadmap without affecting the inventory.
 * Store these recipes in rearranged_recipes for later.
 -------------------------------------------------------------------*/
int removeRecipesForReallocation(BranchPath* node, enum Type_Sort *rearranged_recipes) {
	int num_rearranged_recipes = 0;
	while (node->moves > 1) {
		// Ignore sorts/CH5
		if (node->description.action != ECook) {
			node = node->prev;
			continue;
		}

		// Ignore recipes which do not toss the output
		Cook* cookData = (Cook*)node->description.data;
		if (cookData->handleOutput != Toss) {
			node = node->prev;
			continue;
		}

		// At this point, we have a recipe which tosses the output
		// This output can potentially be relocated to a quicker time
		enum Type_Sort tossed_item = cookData->output;
		rearranged_recipes[num_rearranged_recipes] = tossed_item;
		num_rearranged_recipes++;

		// First update subsequent nodes to remove this item from outputCreated
		BranchPath* newNode = node->next;
		while (newNode != NULL) {
			newNode->outputCreated[getOutputIndex(tossed_item)] = 0;
			newNode->numOutputsCreated--;
			newNode = newNode->next;
		}

		// Now, get rid of this current node
		checkMallocFailed(node->next);
		node->prev->next = node->next;
		node->next->prev = node->prev;
		newNode = node->prev;
		freeNode(node);
		node = newNode;
	}

	return num_rearranged_recipes;
}

/*-------------------------------------------------------------------
 * Function : selectSecondItemFirst
 *
 * Determines whether it is either required or faster to select the
 * second item before the first item originally listed in the recipe
 * combo. Returns 1 if we should swap, returns 0 otherwise.
 -------------------------------------------------------------------*/
int selectSecondItemFirst(const int *ingredientLoc, int nulls, int viableItems) {
	int visibleLoc0 = ingredientLoc[0] - nulls;
	int visibleLoc1 = ingredientLoc[1] - nulls;

	if (ingredientLoc[0] > ingredientLoc[1]) {
		// When swapped, the first ingredient will be between the other
		// ingredient and the beginning. This will also swap the first two when
		// they are in descending order, which is not necessary but not
		// incorrect.
		if (visibleLoc1 <= viableItems / 2) {
			return 1;
		}
		// In this case, the given order is not possible because the second item
		// will be hidden.
		if (ingredientLoc[1] >= 10 && visibleLoc0 == ingredientLoc[1]) {
			return 1;
		}
	}
	else if (visibleLoc0 >= (viableItems + 1) / 2) {
		// When swapped, the hidden item will be between the other ingredient
		// and the end. The second condition is not necessary but helps reduce
		// extraneous swaps.
		if (visibleLoc1 > ingredientLoc[0] && visibleLoc1 >= 10) {
			return 1;
		}
		// When swapped, the hidden ingredient will be between the other
		// ingredient and the end.
		if (ingredientLoc[1] < 10) {
			return 1;
		}
	}
	return 0;
}

/*-------------------------------------------------------------------
 * Function : shiftDownLegalMoves
 *
 * If this function is called, we want to make room in the legal moves
 * array to place a new legal move. Shift all legal moves starting at
 * lowerBound one index towards the end of the list, ending at upperBound
 -------------------------------------------------------------------*/
void shiftDownLegalMoves(struct BranchPath *node, int lowerBound, int upperBound) {
	if (upperBound == lowerBound) return;
	struct BranchPath **legalMoves = node->legalMoves;
	memmove(&legalMoves[lowerBound + 1], &legalMoves[lowerBound], sizeof(&legalMoves[0])*(upperBound - lowerBound));
	/* The above memmove is equivalent to this for loop.
 	for (int i = uppderBound - 1; i >= lowerBound; i--) {
 		legalMoves[i+1] = legalMoves[i];
 	}*/
}

/*-------------------------------------------------------------------
 * Function : shiftUpLegalMoves
 *
 * There is a NULL in the array of legal moves. The first valid legal
 * move AFTER the null is startIndex. Iterate starting at the index of the
 * NULL legal moves and shift all subsequent legal moves towards the
 * front of the array.
 * WARNING: node->numLegalMove MUST already be decremented before
 * calling this function. Thus this function will actually edit
 * node->legalModes[node->numLegalMoves] on the assumption that
 * numLegalMoves is actually now below the allocated region.
 * NOTE: the destination actually starts with index - 1. If index == 0,
 * then you are asking to shift down index 0, which is already at the top,
 * so this function will do nothing.
 *
 * This function does NOT update node->numLegalMoves. Caller should
 * do it themselves.
 -------------------------------------------------------------------*/
void shiftUpLegalMoves(struct BranchPath *node, int startIndex) {
	if (startIndex == 0) return;
	BranchPath **legalMoves = node->legalMoves;
	memmove(&legalMoves[startIndex - 1], &legalMoves[startIndex], sizeof(&legalMoves[0])*(node->numLegalMoves - startIndex + 1));
	/* The above memmove is equivalent to this for loop.
	for (int i = startIndex; i <= node->numLegalMoves; i++) {
		node->legalMoves[i-1] = node->legalMoves[i];
	}*/
	// Null where the last entry was before shifting
	legalMoves[node->numLegalMoves] = NULL;
}

/*-------------------------------------------------------------------
 * Function : shuffleLegalMoves
 *
 * Randomize the order of legal moves by switching two legal moves
 * numlegalMoves times.
 -------------------------------------------------------------------*/
void shuffleLegalMoves(BranchPath *node) {
	// Swap 2 legal moves a variable number of times
	for (int i = 0; i < node->numLegalMoves; i++) {
		int index1 = pcg32_boundedrand_r(&rng[omp_get_thread_num()], node->numLegalMoves);
		int index2 = pcg32_boundedrand_r(&rng[omp_get_thread_num()], node->numLegalMoves);
		BranchPath *temp = node->legalMoves[index1];
		node->legalMoves[index1] = node->legalMoves[index2];
		node->legalMoves[index2] = temp;
	}
}

/*-------------------------------------------------------------------
 * Function : swapItems
 *
 * After determining that it is faster to pick the second item before
 * the first for a given recipe (based on the state of our inventory),
 * swap the item slot locations and offsets (for printing purposes).
 -------------------------------------------------------------------*/
void swapItems(int *ingredientLoc, ItemCombination* combo) {
	int locTemp = ingredientLoc[0];
	ingredientLoc[0] = ingredientLoc[1];
	ingredientLoc[1] = locTemp;

	Type_Sort temp = combo->item1;
	combo->item1 = combo->item2;
	combo->item2 = temp;
}

/*-------------------------------------------------------------------
 * Function 	: tryTossInventoryItem
 *
 * For the given recipe, try to toss items in the inventory in order
 * to make room for the recipe output.
 -------------------------------------------------------------------*/
void tryTossInventoryItem(BranchPath *curNode, Inventory tempInventory, MoveDescription useDescription, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, enum Type_Sort output, int viableItems) {
	for (int tossedIndex = 0; tossedIndex < 10; tossedIndex++) {
		enum Type_Sort tossedItem = tempInventory.inventory[tossedIndex];

		// Make a copy of the tempInventory with the replaced item
		Inventory replacedInventory = replaceItem(tempInventory, tossedIndex, output);

		if (!stateOK(replacedInventory, tempOutputsFulfilled, recipeList)) {
			continue;
		}

		MoveDescription tempUseDescription = useDescription;

		// Calculate the additional tossed frames.
		int tossFrames = invFrames[viableItems][tossedIndex + 1];
		tempUseDescription.framesTaken += tossFrames;
		tempUseDescription.totalFramesTaken += tossFrames;

		finalizeLegalMove(curNode, tempUseDescription, replacedInventory, tempOutputsFulfilled, numOutputsFulfilled, TossOther, tossedItem, tossedIndex);
	}
}

/*-------------------------------------------------------------------
 * Function : alpha_sort
 *
 * Evaluate an alphabetical ascending sort. This is a comparator function
 * called by stdlib's qsort.
 -------------------------------------------------------------------*/
int alpha_sort(const void *elem1, const void *elem2) {
	enum Type_Sort item1 = *((enum Type_Sort*)elem1);
	enum Type_Sort item2 = *((enum Type_Sort*)elem2);

	return getAlphaKey(item1) - getAlphaKey(item2);
}

/*-------------------------------------------------------------------
 * Function 	: alpha_sort_reverse
 *
 * Evaluate an alphabetical descending sort. This is a comparator function
 * called by stdlib's qsort.
 -------------------------------------------------------------------*/
int alpha_sort_reverse(const void *elem1, const void *elem2) {
	enum Type_Sort item1 = *((enum Type_Sort*)elem1);
	enum Type_Sort item2 = *((enum Type_Sort*)elem2);

	return getAlphaKey(item2) - getAlphaKey(item1);
}

/*-------------------------------------------------------------------
 * Function 	: type_sort
 *
 * Evaluate an type ascending sort. This is a comparator function
 * called by stdlib's qsort.
 -------------------------------------------------------------------*/
int type_sort(const void *elem1, const void *elem2) {
	enum Type_Sort item1 = *((enum Type_Sort*)elem1);
	enum Type_Sort item2 = *((enum Type_Sort*)elem2);

	return item1 - item2;
}

/*-------------------------------------------------------------------
 * Function 	: type_sort_reverse
 *
 * Evaluate an type descending sort. This is a comparator function
 * called by stdlib's qsort.
 -------------------------------------------------------------------*/
int type_sort_reverse(const void *elem1, const void *elem2) {
	enum Type_Sort item1 = *((enum Type_Sort*)elem1);
	enum Type_Sort item2 = *((enum Type_Sort*)elem2);

	return item2 - item1;
}

/*-------------------------------------------------------------------
 * Function 	: getSortedInventory
 *
 * Given the type of sort, use qsort to create a new sorted inventory.
 -------------------------------------------------------------------*/
Inventory getSortedInventory(Inventory inventory, enum Action sort) {
	// Set up the inventory for sorting
	memmove(inventory.inventory, inventory.inventory + inventory.nulls,
		(inventory.length - inventory.nulls) * sizeof(enum Type_Sort));
	inventory.length -= inventory.nulls;
	inventory.nulls = 0;

	// Use qsort and execute sort function depending on sort type
	int (*cmpfunc)(const void*, const void*) = NULL;
	switch(sort) {
		case ESort_Alpha_Asc:
			cmpfunc = &alpha_sort;
			break;
		case ESort_Alpha_Des:
			cmpfunc = &alpha_sort_reverse;
			break;
		case ESort_Type_Asc:
			cmpfunc = &type_sort;
			break;
		case ESort_Type_Des:
			cmpfunc = &type_sort_reverse;
			break;
		default:
			printf("Error in sorting inventory.\n");
			exit(2);
	}

	qsort((void*)inventory.inventory, inventory.length, sizeof(Type_Sort), cmpfunc);
	return inventory;
}

/*-------------------------------------------------------------------
 * Function 	: logIterations
 *
 * Print out information to the user about how deep we are and
 * the frame cost at this point after a certain number of iterations.
 -------------------------------------------------------------------*/
void logIterations(int ID, int stepIndex, const BranchPath * curNode, int iterationCount, int level)
{
	char callString[30];
	char iterationString[100];
	sprintf(callString, "Thread %d", ID+1);
	sprintf(iterationString, "%d steps currently taken, %d frames accumulated so far; %dk iterations",
		stepIndex, curNode->description.totalFramesTaken, iterationCount / 1000);
	recipeLog(level, "Calculator", "Info", callString, iterationString);
}

/*-------------------------------------------------------------------
 * Function 	: calculateOrder
 *
 * This is the main roadmap evaluation function. This calls various
 * child functions to generate a sequence of legal moves. It then
 * uses parameters to determine which legal move to traverse to. Once
 * a roadmap is found, the data is printed to a .txt file, and the result
 * is passed back to start.c to try submitting to the server.
 -------------------------------------------------------------------*/
Result calculateOrder(const int ID) {
	enum SelectionMethod selectionMethod = getConfigInt("selectionMethod");
	// When performing in-order or manual traversal, we never want to restart.
	bool noRestart = selectionMethod == InOrder || selectionMethod == Manual;
	bool freeRunning = false;
	int branchInterval = getConfigInt("branchLogInterval");
	int total_dives = 0;
	BranchPath *curNode = NULL; // Deepest node at any particular point
	BranchPath *root;

	Result result_cache = (Result) {-1, -1};

	// Initialize array of visited nodes
	// THIS IS A GLOBAL THAT CAN BE ACCESSED BY OTHER THREADS

	//Start main loop
	while (1) {
		if (askedToShutdown()) {
			break;
		}
		int stepIndex = 0;
		int iterationCount = 0;
		int iterationLimit = DEFAULT_ITERATION_LIMIT;

		// Create root of tree path
		curNode = initializeRoot();
		root = curNode; // Necessary when printing results starting from root

		if (total_dives % branchInterval == 0 && !noRestart) {
			char temp1[30];
			char temp2[75];
			sprintf(temp1, "Thread %d", ID+1);
			sprintf(temp2, "Searching new random branch (%d searched so far)", total_dives);
			recipeLog(3, "Calculator", "Info", temp1, temp2);
		}
		total_dives++;

		// If the user is not exploring only one branch, reset when it is time
		// Start iteration loop
		while (iterationCount < iterationLimit || noRestart) {
			if (checkShutdownOnIndex(iterationCount)) {
				break;
			}
			
			// In the rare occassion that the root node runs out of legal moves due to "select",
			// exit out of the while loop to restart
			if (curNode == NULL) {
				break;
			}

			// Check for end condition (57 recipes + the Chapter 5 intermission)
			if(curNode->numOutputsCreated == NUM_RECIPES) {
				// All recipes have been fulfilled!
				// Check that the total time taken is strictly less than the current observed record.
				// Apply a frame penalty if the final move did not toss an item.
				applyJumpStorageFramePenalty(curNode);

				if (curNode->description.totalFramesTaken < getLocalRecord() + BUFFER_SEARCH_FRAMES) {
					// A finished roadmap has been generated
					// Rearrange the roadmap to save frames
					struct OptimizeResult optimizeResult = optimizeRoadmap(root);
					#pragma omp critical(optimize)
					{
						if (optimizeResult.last->description.totalFramesTaken < getLocalRecord()) {
							setLocalRecord(optimizeResult.last->description.totalFramesTaken);
							char *filename = malloc(sizeof(char) * 17);
							sprintf(filename, "results/%d.txt", optimizeResult.last->description.totalFramesTaken);
							printResults(filename, optimizeResult.root);
							char tmp[100];
							sprintf(tmp, "New local fastest roadmap found! %d frames, saved %d after rearranging", optimizeResult.last->description.totalFramesTaken, curNode->description.totalFramesTaken - optimizeResult.last->description.totalFramesTaken);
							recipeLog(1, "Calculator", "Info", "Roadmap", tmp);
							free(filename);
							if (selectionMethod == Manual) {
								testRecord(result_cache.frames);
							}
							result_cache = (Result){ optimizeResult.last->description.totalFramesTaken, ID };

							writePersonalBest(&result_cache);

							// Reset the iteration count so we continue to explore near this record
							iterationLimit = iterationCount + ITERATION_LIMIT_INCREASE;
						}
					}

					freeAllNodes(optimizeResult.last);
				}

				// Regardless of record status, it's time to go back up and find new endstates
				curNode = curNode->prev;
				freeAndShiftLegalMove(curNode, 0);
				curNode->next = NULL;
				stepIndex--;
				continue;
			}
			// End condition not met. Check if this current level has something in the event queue
			else if (curNode->legalMoves == NULL) {
				// This node has not yet been assigned an array of legal moves.
				// Generate the list of all possible recipes
				fulfillRecipes(curNode);

				// Special handling of the 56th recipe, which is representative of the Chapter 5 intermission

				// The first item is trading the Mousse Cake and 2 Hot Dogs for a Dried Bouquet
				// Inventory must contain both items, and Hot Dog must be in a slot such that it can be duplicated
				// The Mousse Cake and Hot Dog cannot be in a slot such that it is "hidden" due to NULLs in the inventory
				if (!curNode->outputCreated[getOutputIndex(Dried_Bouquet)]
					&& indexOfItemInInventory(curNode->inventory, Mousse_Cake) != -1
					&& indexOfItemInInventory(curNode->inventory, Hot_Dog) >= 10) {
					fulfillChapter5(curNode);
				}

				// Special handling of inventory sorting
				// Avoid redundant searches
				if (curNode->description.action == ECook || curNode->description.action == ECh5) {
					handleSorts(curNode);
				}

				// All legal moves evaluated and listed!

				if (curNode->numOutputsCreated == 0) {
					// Filter out all legal moves that use 2 ingredients in the very first legal move
					filterOut2Ingredients(curNode);
				}

				// Special filtering if we only had one recipe left to fulfill
				if (curNode->numOutputsCreated == NUM_RECIPES-1 && curNode->numLegalMoves > 0 && curNode->legalMoves != NULL && curNode->legalMoves[0]->description.action == ECook) {
					// If there are any legal moves that satisfy this final recipe,
					// strip out everything besides the fastest legal move
					// This saves on recursing down pointless states
					popAllButFirstLegalMove(curNode);
				}
				// Now that all moves are added, bring the one we will explore
				// next to the front.
				else {
					handleLegalMoveSelection(curNode, selectionMethod);
				}

				if (curNode->numLegalMoves == 0) {
					// There are no legal moves to iterate on
					// Go back up!

					// Handle the case where the root node runs out of legal moves
					if (curNode->prev == NULL) {
						freeNode(curNode);
						return (Result) {-1, -1};
					}

					curNode = curNode->prev;
					freeAndShiftLegalMove(curNode, 0);
					curNode->next = NULL;
					stepIndex--;
					continue;
				}

				// Allow the user to choose their path
				else if (selectionMethod == Manual && !freeRunning) {
					FILE *fp = stdout;
					for (int move = 0; move < curNode->numLegalMoves; ++move) {
						fprintf(fp, "%d - ", move);
						printNodeDescription(curNode->legalMoves[move], fp);
						fprintf(fp, "\n");
					}

					fprintf(fp, "%d - Run freely\n", curNode->numLegalMoves);

					printf("Which move would you like to perform? ");
					int moveToExplore;
					ABSL_ATTRIBUTE_UNUSED int ignored = scanf("%d", &moveToExplore);  // For now, we are going to blindly assume it was written.
					fprintf(fp, "\n");

					if (moveToExplore == curNode->numLegalMoves) {
						freeRunning = 1;
						handleLegalMoveSelection(curNode, selectionMethod);
					}
					else {
						// Take the legal move at nextMoveIndex and move it to the front of the array
						BranchPath *nextMove = curNode->legalMoves[0];
						curNode->legalMoves[0] = curNode->legalMoves[moveToExplore];
						curNode->legalMoves[moveToExplore] = nextMove;
					}
				}

				// Once the list is generated choose the top-most path and iterate downward

				checkMallocFailed(curNode->legalMoves);

				curNode->next = curNode->legalMoves[0];
				curNode = curNode->next;
				stepIndex++;

			}
			else {
				if (curNode->numLegalMoves == 0) {
					// No legal moves are left to evaluate, go back up...
					// Wipe away the current node

					// Handle the case where the root node runs out of legal moves
					if (curNode->prev == NULL) {
						freeNode(curNode);
						return (Result) {-1, -1};
					}

					curNode = curNode->prev;
					freeAndShiftLegalMove(curNode, 0);
					curNode->next = NULL;
					stepIndex--;
					continue;
				}

				// Moves are already shuffled if using Random, but Exponential
				// needs to choose a new move every time we go back up.
				if (selectionMethod == Exponential)
					handleLegalMoveSelection(curNode, selectionMethod);

				// Once the list is generated, choose the top-most (quickest) path and iterate downward
				curNode->next = curNode->legalMoves[0];
				curNode = curNode->legalMoves[0];
				stepIndex++;

				// Logging for progress display
				iterationCount++;
				if (iterationCount % (branchInterval * DEFAULT_ITERATION_LIMIT) == 0
					&& (noRestart || iterationLimit != DEFAULT_ITERATION_LIMIT)) {
					logIterations(ID, stepIndex, curNode, iterationCount, 3);
				}
				else if (iterationCount % 10000 == 0) {
					logIterations(ID, stepIndex, curNode, iterationCount, 6);
				}
			}
		}

		// We have passed the iteration maximum
		// Free everything before reinitializing
		freeAllNodes(curNode);

		// Check the cache to see if a result was generated
		if (result_cache.frames > -1)
			return result_cache;

		// Period check for Github update (only perform on thread 0)
		if (total_dives % 10000 == 0 && omp_get_thread_num() == 0) {
			if (checkGithubVer() == 1)
				exit(1);
		}
		
		// For profiling
		/*if (total_dives == 100) {
			exit(1);
		}*/

	}

	// Unexpected break out of loop. Return the nothing results.
	return (Result) { -1, -1 };
}

/*-------------------------------------------------------------------
 * Function 	: writePersonalBest
 *
 * Write the user's current best roadmap time to PB.txt.
 -------------------------------------------------------------------*/
void writePersonalBest(Result *result)
{
	// Prevent slower threads from overwriting a faster record in PB.txt
	// by first checking the current record
	FILE* fp = NULL;
	fp = fopen("results/PB.txt", "w");

	if (fp == NULL)
	{
		recipeLog(1, "Calculator", "Roadmap", "Error", "Failed to open results/PB.txt for writing");
		return;
	}

	// Modify PB.txt
	if (ABSL_PREDICT_FALSE(result->frames < 0)) {
		// Somehow got invalid number of frames.
		// Fetch the current known max from the global.
		int localRecord = getLocalRecord();
		if (ABSL_PREDICT_FALSE(localRecord < 0)) {
			recipeLog(1, "Calculator", "Roadmap", "Error", "Current cached local record is corrupt (less then 0 frames). Not writing invalid PB file but your PB may be lost.");
		}
		else {
			fprintf(fp, "%d", localRecord);
		}
	}
	else {
		recipeLog(3, "Calculator", "Roadmap", "PB", "Wrote to PB.txt");
		fprintf(fp, "%d", result->frames);
	}
	fclose(fp);
}

void shutdownCalculator()
{
	free(recipeList);
	for (int i = 0; i < INVENTORY_MAX_SIZE; i++)
		free(invFrames[i]);
}
