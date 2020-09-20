#include <stdio.h>
#include <stdlib.h>
#include "calculator.h"
#include "FTPManagement.h"
#include "recipes.h"
#include "start.h"
#include <libconfig.h>
#include "logger.h"
#include <assert.h>
#include <time.h>
#include <string.h>

// TODO: Eliminate the need to filter legal moves which exceed frame limit by not appending them in the first place

#define NUM_RECIPES 58 			// Including Chapter 5 representation
#define CHOOSE_2ND_INGREDIENT_FRAMES 56 	// Penalty for choosing a 2nd item
#define TOSS_FRAMES 32				// Penalty for having to toss an item
#define ALPHA_SORT_FRAMES 38			// Penalty to perform alphabetical ascending sort
#define REVERSE_ALPHA_SORT_FRAMES 40		// Penalty to perform alphabetical descending sort
#define TYPE_SORT_FRAMES 39			// Penalty to perform type ascending sort
#define REVERSE_TYPE_SORT_FRAMES 41		// Penalty to perform type descending sort
#define JUMP_STORAGE_NO_TOSS_FRAMES 5		// Penalty for not tossing the last item (because we need to get Jump Storage)
#define BUFFER_SEARCH_FRAMES 150		// Threshold to try optimizing a roadmap to attempt to beat the current record

int **invFrames;
struct Recipe *recipeList;

/*-------------------------------------------------------------------
 * Function 	: initializeInvFrames
 * 
 * Initializes global variable invFrames, which is used to calculate
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
 * Function 	: applyJumpStorageFramePenalty
 * Inputs	: struct BranchPath *node
 * 
 * Looks at the node's Cook data. If the item is autoplaced, then add
 * a penalty for not tossing the item. Adjust framesTaken and
 * totalFramesTaken to reflect this change.
 -------------------------------------------------------------------*/
void applyJumpStorageFramePenalty(struct BranchPath *node) {
	if (((struct Cook *) node->description.data)->handleOutput == Autoplace) {
		node->description.framesTaken += JUMP_STORAGE_NO_TOSS_FRAMES;
		node->description.totalFramesTaken += JUMP_STORAGE_NO_TOSS_FRAMES;
	}
	
	return;
}

/*-------------------------------------------------------------------
 * Function 	: copyCook
 * Inputs	: struct Cook *cookNew
 *		  struct Cook *cookOld
 * 
 * A simple function to copy the data within cookOld to cookNew.
 -------------------------------------------------------------------*/
void copyCook(struct Cook *cookNew, struct Cook *cookOld) {
	*cookNew = *cookOld;
	return;
}

/*-------------------------------------------------------------------
 * Function 	: copyOutputsFulfilled
 * Inputs	: struct BranchPath *node
 *		  int *oldOutputsFulfilled
 * Outputs	: 
 * A simple memcpy to duplicate oldOutputsFulfilled to a new array
 -------------------------------------------------------------------*/
int *copyOutputsFulfilled(int *oldOutputsFulfilled) {
	int *newOutputsFulfilled = malloc(sizeof(int) * NUM_RECIPES);
	memcpy((void *)newOutputsFulfilled, (void *)oldOutputsFulfilled, sizeof(int) * NUM_RECIPES);
	return newOutputsFulfilled;
}

/*-------------------------------------------------------------------
 * Function 	: countTotalSorts
 * Inputs	: struct BranchPath *node
 * 
 * Traverse through the current roadmap to count the total number of sorts
 * TODO: This function can be obsoleted by adding a totalSorts attribute
 *	 to struct BranchPath to negate this lengthy while loop
 -------------------------------------------------------------------*/
int countTotalSorts(struct BranchPath *node) {
	int sorts = 0;
	do {
		if (node->description.action >= Sort_Alpha_Asc && node->description.action <= Sort_Type_Des) {
			sorts++;
		}
		node = node->prev;
	} while (node != NULL);
	
	return sorts;
}

/*-------------------------------------------------------------------
 * Function 	: createLegalMove
 * Inputs	: int				DB_place_index
 *		  int				CO_place_index
 *		  int				KM_place_index
 *		  int				CS_place_index
 *		  int				TR_use_index
 *		  int				lateSort
 * Outputs	: struct CH5			*ch5
 * 
 * Compartmentalization of setting struct CH5 attributes
 * lateSort tracks whether we performed the sort before or after the
 * Keel Mango, for printing purposes
 -------------------------------------------------------------------*/
struct CH5 *createChapter5Struct(int DB_place_index, int CO_place_index, int KM_place_index, int CS_place_index, int TR_use_index, enum Action sort, int lateSort) {
	struct CH5 *ch5 = malloc(sizeof(struct CH5));
	ch5->indexDriedBouquet = DB_place_index;
	ch5->indexCoconut = CO_place_index;
	ch5->ch5Sort = sort;
	ch5->indexKeelMango = KM_place_index;
	ch5->indexCourageShell = CS_place_index;
	ch5->indexThunderRage = TR_use_index;
	ch5->lateSort = lateSort;
	return ch5;
}

/*-------------------------------------------------------------------
 * Function 	: createCookDescription
 * Inputs	: struct BranchPath 	  *node
 *		  struct Recipe 	  recipe
 *		  struct ItemCombination combo
 *		  enum Type_Sort	  *tempInventory
 *		  int 			  *tempFrames
 *		  int 			  viableItems
 * Outputs	: struct MoveDescription useDescription
 * 
 * Compartmentalization of generating a MoveDescription struct
 * based on various parameters dependent on what recip we're cooking
 -------------------------------------------------------------------*/
struct MoveDescription createCookDescription(struct BranchPath *node, struct Recipe recipe, struct ItemCombination combo, enum Type_Sort *tempInventory, int *tempFrames, int viableItems) {
	struct MoveDescription useDescription;
	useDescription.action = Cook;
	
	int ingredientLoc[2];
	int ingredientOffset[2];
	
	// Determine the locations of both ingredients
	ingredientLoc[0] = indexOfItemInInventory(tempInventory, combo.item1);
	ingredientLoc[1] = indexOfItemInInventory(tempInventory, combo.item2);
	
	// Determine the offset by NULLs before the desired items, as NULLs do not appear during the inventory navigation
	ingredientOffset[0] = countNullsInInventory(tempInventory, 0, ingredientLoc[0]);
	ingredientOffset[1] = countNullsInInventory(tempInventory, 0, ingredientLoc[1]);
	
	if (combo.numItems == 1) {
		createCookDescription1Item(node, recipe, combo, tempInventory, ingredientLoc, ingredientOffset, tempFrames, viableItems, &useDescription);
	}
	else {
		createCookDescription2Items(node, recipe, combo, tempInventory, ingredientLoc, ingredientOffset, tempFrames, viableItems, &useDescription);
	}
	
	return useDescription;
}

/*-------------------------------------------------------------------
 * Function 	: createCookDescription1Item
 * Inputs	: struct BranchPath 		*node
 *		  struct Recipe 		recipe
 *		  struct ItemCombination 	combo
 *		  enum Type_Sort		*tempInventory
 *		  int 				*ingredientLoc
 *		  int 				*ingredientOffset
 *		  int 				*tempFrames
 *		  int 				viableItems
 		  struct MoveDescription	*useDescription
 * 
 * Handles inventory management and frame calculation for recipes of
 * length 1. Generates Cook structure and points to this structure
 * in useDescription.
 -------------------------------------------------------------------*/
void createCookDescription1Item(struct BranchPath *node, struct Recipe recipe, struct ItemCombination combo, enum Type_Sort *tempInventory, int *ingredientLoc, int *ingredientOffset, int *tempFrames, int viableItems, struct MoveDescription *useDescription) {
	// This is a potentially viable recipe with 1 ingredient
	// Modify the inventory if the ingredient was in the first 10 slots
	if (ingredientLoc[0] < 10) {
		tempInventory[ingredientLoc[0]] = -1;
	}
	
	// Determine how many frames will be needed to select that item
	*tempFrames = invFrames[viableItems][ingredientLoc[0]-ingredientOffset[0]];
		
	// Describe what items were used
	
	generateCook(useDescription, combo, recipe, ingredientLoc, 0);
	generateFramesTaken(useDescription, node, *tempFrames);
}

/*-------------------------------------------------------------------
 * Function 	: createCookDescription2Items
 * Inputs	: struct BranchPath 		*node
 *		  struct Recipe 		recipe
 *		  struct ItemCombination 	combo
 *		  enum Type_Sort		*tempInventory
 *		  int 				*ingredientLoc
 *		  int 				*ingredientOffset
 *		  int 				*tempFrames
 *		  int 				viableItems
 		  struct MoveDescription	*useDescription
 * 
 * Handles inventory management and frame calculation for recipes of
 * length 2. Swaps items if it's faster to choose the second item first.
 * Generates Cook structure and points to this structure in useDescription.
 -------------------------------------------------------------------*/
void createCookDescription2Items(struct BranchPath *node, struct Recipe recipe, struct ItemCombination combo, enum Type_Sort *tempInventory, int *ingredientLoc, int *ingredientOffset, int *tempFrames, int viableItems, struct MoveDescription *useDescription) {
	// This is a potentially viable recipe with 2 ingredients
	//Baseline frames based on how many times we need to access the menu
	*tempFrames = CHOOSE_2ND_INGREDIENT_FRAMES;
	
	// Determine which order of ingredients to take
	// The first picked item always vanishes from the list of ingredients when picking the 2nd ingredient
	// There are some configurations where it is 2 frames faster to pick the ingredients in the reverse order
	int swap = 0;
	if (selectSecondItemFirst(node, combo, ingredientLoc, ingredientOffset, viableItems)) {
		// It's faster to select the 2nd item, so make it the priority and switch the order
		swapItems(ingredientLoc, ingredientOffset);
		swap = 1;
	}
	
	// Calculate the number of frames needed to grab the first item
	*tempFrames += invFrames[viableItems][ingredientLoc[0]-ingredientOffset[0]];
	
	// Set each inventory index to null if the item was in the first 10 slots
	if (ingredientLoc[0] < 10) {
		tempInventory[ingredientLoc[0]] = -1;
	}
	if (ingredientLoc[1] < 10) {
		tempInventory[ingredientLoc[1]] = -1;
	}
	
	// Determine the frames needed for the 2nd ingredient
	// First ingredient is always removed from the menu, so there is always 1 less viable item
	if (ingredientLoc[1] > ingredientLoc[0]) {
		// In this case, the 2nd ingredient has "moved up" one slot since the 1st ingredient vanishes
		*tempFrames += invFrames[viableItems-1][ingredientLoc[1]-ingredientOffset[1]-1];
	}
	else {
		// In this case, the 2nd ingredient was found earlier on than the 1st ingredient, so no change to index
		*tempFrames += invFrames[viableItems-1][ingredientLoc[1]-ingredientOffset[1]];
	}
	
	// Describe what items were used
	generateCook(useDescription, combo, recipe, ingredientLoc, swap);
	generateFramesTaken(useDescription, node, *tempFrames);
}

/*-------------------------------------------------------------------
 * Function 	: createLegalMove
 * Inputs	: struct BranchPath		*node
 *		  enum Type_Sort		*inventory
 *		  struct MoveDescription	description
 *		  int				*outputsFulfilled
 *		  int				numOutputsFulfilled
 * Outputs	: struct BranchPath		*newLegalMove
 * 
 * Given the input parameters, allocate and set attributes for a legalMove node
 -------------------------------------------------------------------*/
struct BranchPath *createLegalMove(struct BranchPath *node, enum Type_Sort *inventory, struct MoveDescription description, int *outputsFulfilled, int numOutputsFulfilled) {
	struct BranchPath *newLegalMove = malloc(sizeof(struct BranchPath));
	newLegalMove->moves = node->moves + 1;
	newLegalMove->inventory = inventory;
	newLegalMove->description = description;
	newLegalMove->prev = node;
	newLegalMove->next = NULL;
	newLegalMove->outputCreated = outputsFulfilled;
	newLegalMove->numOutputsCreated = numOutputsFulfilled;
	newLegalMove->legalMoves = NULL;
	newLegalMove->numLegalMoves = 0;
	return newLegalMove;
}

/*-------------------------------------------------------------------
 * Function 	: filterLegalMovesExceedFrameLimit
 * Inputs	: struct BranchPath		*node
 *		  int				frames
 * 
 * Pop any legal moves which exceed frames (the current server best)
 -------------------------------------------------------------------*/
void filterLegalMovesExceedFrameLimit(struct BranchPath *node, int frames) {
	for (int i = 0; i < node->numLegalMoves; i++) {
		if (node->legalMoves[i]->description.totalFramesTaken > frames) {
			freeLegalMove(node, i);
			i--; // Update i so we don't skip over the newly moved legalMoves
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: filterOut2Ingredients
 * Inputs	: struct BranchPath		*node
 * 
 * For the first node's legal moves, we cannot cook a recipe which
 * contains two items. Thus, we need to remove any legal moves
 * which require two ingredients
 -------------------------------------------------------------------*/
void filterOut2Ingredients(struct BranchPath *node) {
	for (int i = 0; i < node->numLegalMoves; i++) {
		if (node->legalMoves[i]->description.action == Cook) {
			struct Cook *cook = node->legalMoves[i]->description.data;
			if (cook->numItems == 2) {
				freeLegalMove(node, i);
				i--; // Update i so we don't skip over the newly moved legalMoves
			}
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: finalizeChapter5Eval
 * Inputs	: struct BranchPath		*node
 *		  enum Type_Sort		*inventory
 *		  enum Action			sort
 *		  struct CH5			*ch5Data
 *		  int 				temp_frame_sum
 *		  int				*outputsFulfilled
 *		  int				numOutputsFulfilled
 * 
 * Given input parameters, construct a new legal move to represent CH5
 -------------------------------------------------------------------*/
void finalizeChapter5Eval(struct BranchPath *node, enum Type_Sort *inventory, enum Action sort, struct CH5 *ch5Data, int temp_frame_sum, int *outputsFulfilled, int numOutputsFulfilled) {
	// Get the index of where to insert this legal move to
	int insertIndex = getInsertionIndex(node, temp_frame_sum);
	
	// Copy the inventory
	enum Type_Sort *legalInventory = copyInventory(inventory);
	struct MoveDescription description;
	description.action = Ch5;
	description.data = ch5Data;
	description.framesTaken = temp_frame_sum;
	description.totalFramesTaken = node->description.totalFramesTaken + temp_frame_sum;
	int *copyOfOutputsFulfilled = copyOutputsFulfilled(outputsFulfilled);
	
	// Create the legalMove node
	struct BranchPath *legalMove = createLegalMove(node, legalInventory, description, copyOfOutputsFulfilled, numOutputsFulfilled);
	
	// Apend the legal move
	insertIntoLegalMoves(insertIndex, legalMove, node);
	assert(node->legalMoves[node->numLegalMoves-1] != NULL);
}

/*-------------------------------------------------------------------
 * Function 	: finalizeLegalMove
 * Inputs	: struct BranchPath		*node
 *		  int				tempFrames
 *		  struct MoveDescription	useDescription
 *		  enum Type_Sort		*tempInventory
 *		  int				*tempOutputsFulfilled
 *		  int				numOutputsFulfilled
 *		  enum HandleOutput		tossType
 *		  enum Type_Sort		toss
 *		  int				tossIndex
 * 
 * Given input parameters, construct a new legal move to represent
 * a valid recipe move
 -------------------------------------------------------------------*/
void finalizeLegalMove(struct BranchPath *node, int tempFrames, struct MoveDescription useDescription, enum Type_Sort *tempInventory, int *tempOutputsFulfilled, int numOutputsFulfilled, enum HandleOutput tossType, enum Type_Sort toss, int tossIndex) {
	// This is a viable state that doesn't increase frames at all (output was auto-placed)
	// Determine where to insert this legal move into the list of legal moves (sorted by frames taken)
	int insertIndex = getInsertionIndex(node, tempFrames);
	
	enum Type_Sort *legalInventory = copyInventory(tempInventory);
	struct Cook *cookNew = malloc(sizeof(struct Cook));
	copyCook(cookNew, (struct Cook *)useDescription.data);
	cookNew->handleOutput = tossType;
	cookNew->toss = toss;
	cookNew->indexToss = tossIndex;
	useDescription.data = cookNew;
	int *copyOfOutputsFulfilled = copyOutputsFulfilled(tempOutputsFulfilled);
	
	// Create the legalMove node
	struct BranchPath *newLegalMove = createLegalMove(node, legalInventory, useDescription, copyOfOutputsFulfilled, numOutputsFulfilled);
	
	// Insert this new move into the current node's legalMove array
	insertIntoLegalMoves(insertIndex, newLegalMove, node);
}

/*-------------------------------------------------------------------
 * Function 	: freeAllNodes
 * Inputs	: struct BranchPath	*node
 * 
 * We've reached the iteration limit, so free all nodes in the roadmap
 * We additionally need to delete node from the previous node's list of
 * legalMoves to prevent a double-free
 -------------------------------------------------------------------*/
void freeAllNodes(struct BranchPath *node) {
	struct BranchPath *prevNode = NULL;
	
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
 * Function 	: freeLegalMove
 * Inputs	: struct BranchPath	*node
 *		  int			index
 * 
 * Free the legal move at index in the node's array of legal moves
 -------------------------------------------------------------------*/
void freeLegalMove(struct BranchPath *node, int index) {
	freeNode(node->legalMoves[index]);
	node->legalMoves[index] = NULL;
	node->numLegalMoves--;
	node->next = NULL;
	
	// Shift up the rest of the legal moves
	shiftUpLegalMoves(node, index + 1);
}

/*-------------------------------------------------------------------
 * Function 	: freeNode
 * Inputs	: struct BranchPath	*node
 * 
 * Free the current node and all legal moves within the node
 -------------------------------------------------------------------*/
void freeNode(struct BranchPath *node) {
	if (node->description.data != NULL) {
		free(node->description.data);
	}
	// Root node accesses starting inventory from passed-in parameter... Do not free it
	if (node->moves > 0) {
		free(node->inventory);
	}
	free(node->outputCreated);
	if (node->legalMoves != NULL) {
		while (node->numLegalMoves > 0) {
			freeLegalMove(node, 0);
		}
		free(node->legalMoves);
	}
	free(node);
}

/*-------------------------------------------------------------------
 * Function 	: fulfillChapter5
 * Inputs	: struct BranchPath	*curNode
 * 
 * A preliminary step to determine Dried Bouquet and Coconut placement
 * before calling handleChapter5Eval
 -------------------------------------------------------------------*/
void fulfillChapter5(struct BranchPath *curNode) {
	// Create an outputs chart but with the Dried Bouquet collected
	// to ensure that the produced inventory can fulfill all remaining recipes
	int *tempOutputsFulfilled = copyOutputsFulfilled(curNode->outputCreated);
	tempOutputsFulfilled[getIndexOfRecipe(Dried_Bouquet)] = 1;
	int numOutputsFulfilled = curNode->numOutputsCreated + 1;
	
	// Create a temp inventory
	enum Type_Sort *tempInventory = copyInventory(curNode->inventory);
	
	// Determine how many spaces are available in the inventory for frame calculation purposes
	int viableItems = countItemsInInventory(tempInventory);
	
	// If the Mousse Cake is in the first 10 slots, change it to NULL
	int mousse_cake_index = indexOfItemInInventory(tempInventory, Mousse_Cake);
	if (mousse_cake_index < 10) {
		tempInventory[mousse_cake_index] = -1;
		viableItems--;
	}
	
	// Handle allocation of the first 2 CH5 items (Dried Bouquet and Coconut)
	int nullsInInventory = countNullsInInventory(tempInventory, 0, 10);
	switch (nullsInInventory) {
		case 0 :
			handleDBCOAllocation0Nulls(curNode, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, viableItems);
			break;
		case 1 :
			handleDBCOAllocation1Null(curNode, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, viableItems);
			break;
		default :
			handleDBCOAllocation2Nulls(curNode, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, viableItems);
	}
	
	free(tempInventory);
	free(tempOutputsFulfilled);
}

/*-------------------------------------------------------------------
 * Function 	: fulfillRecipes
 * Inputs	: struct BranchPath	*curNode
 * 		  int			recipeIndex
 *
 * Iterate through all possible combinations of cooking different
 * recipes and create legal moves for them
 -------------------------------------------------------------------*/
void fulfillRecipes(struct BranchPath *curNode, int recipeIndex) {
	// Only want ingredient combos that can be fulfilled right now!
	struct Recipe recipe = recipeList[recipeIndex];
	struct ItemCombination *combos = recipe.combos;
	for (int comboIndex = 0; comboIndex < recipe.countCombos; comboIndex++) {
		struct ItemCombination combo = combos[comboIndex];
		if (!itemComboInInventory(combo, curNode->inventory)) {
			continue;
		}
			
		// This is a recipe that can be fulfilled right now!
		
		// Copy the inventory
		enum Type_Sort *tempInventory = copyInventory(curNode->inventory);
		
		// Mark that this output has been fulfilled for viability determination
		int *tempOutputsFulfilled = copyOutputsFulfilled(curNode->outputCreated);
		tempOutputsFulfilled[recipeIndex] = 1;
		int numOutputsFulfilled = curNode->numOutputsCreated + 1;
		
		// Determine how many viable items are in the list (No NULLS or BLOCKED)
		int viableItems = countItemsInInventory(tempInventory);
		
		int tempFrames;
		
		struct MoveDescription useDescription = createCookDescription(curNode, recipe, combo, tempInventory, &tempFrames, viableItems);
		
		// Store the base useDescription's cook pointer to be freed later
		struct Cook *cookBase = (struct Cook *)useDescription.data;
		
		// Handle allocation of the output
		handleRecipeOutput(curNode, tempInventory, tempFrames, useDescription, tempOutputsFulfilled, numOutputsFulfilled, recipe.output, viableItems);
		
		free(cookBase);
		free(tempInventory);
		free(tempOutputsFulfilled);
	}
}

/*-------------------------------------------------------------------
 * Function 	: generateCook
 * Inputs	: struct MoveDescription	*description
 * 		  struct ItemCombination	combo
 *		  struct Recipe		recipe
 *		  int				*ingredientLoc
 *		  int				swap
 *
 * Given input parameters, generate Cook structure
 -------------------------------------------------------------------*/
void generateCook(struct MoveDescription *description, struct ItemCombination combo, struct Recipe recipe, int *ingredientLoc, int swap) {
	struct Cook *cook = malloc(sizeof(struct Cook));
	description->action = Cook;
	cook->numItems = combo.numItems;
	if (swap) {
		cook->item1 = combo.item2;
		cook->item2 = combo.item1;
	}
	else {
		cook->item1 = combo.item1;
		cook->item2 = combo.item2;
	}
	
	cook->itemIndex1 = ingredientLoc[0];
	cook->itemIndex2 = ingredientLoc[1];
	cook->output = recipe.output;
	description->data = cook;
}

/*-------------------------------------------------------------------
 * Function 	: generateFramesTaken
 * Inputs	: struct MoveDescription	*description
 * 		  struct BranchPath		*node
 *		  int				framesTaken
 *
 * Assign frame duration to description structure and reference the
 * previous node to find the total frame duration for the roadmap thus far
 -------------------------------------------------------------------*/
void generateFramesTaken(struct MoveDescription *description, struct BranchPath *node, int framesTaken) {
	description->framesTaken = framesTaken;
	description->totalFramesTaken = node->description.totalFramesTaken + framesTaken;
}

/*-------------------------------------------------------------------
 * Function 	: getInsertionIndex
 * Inputs	: struct BranchPath	*curNode
 *		  int			frames
 *
 * Based on the frames it takes to complete a new legal move, find out
 * where to insert it in the current node's array of legal moves, which
 * is ordered based on frame count ascending
 -------------------------------------------------------------------*/
int getInsertionIndex(struct BranchPath *curNode, int frames) {
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
 * Function 	: handleChapter5EarlySortEndItems
 * Inputs	: struct BranchPath	*node
 *		  enum Type_Sort	*inventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			sort_frames
 *		  enum Action		sort
 *		  int			frames_DB
 *		  int			frames_CO
 *		  int			DB_place_index
 *		  int			CO_place_index
 *
 * Evaluate Chapter 5 such that a sort occurs between grabbing the
 * Coconut and the Keel Mango. Place the Keel Mango and Courage Shell
 * in various inventory locations. Determine if the move is legal.
 -------------------------------------------------------------------*/
void handleChapter5EarlySortEndItems(struct BranchPath *node, enum Type_Sort *inventory, int *outputsFulfilled, int numOutputsFulfilled, int sort_frames, enum Action sort, int frames_DB, int frames_CO, int DB_place_index, int CO_place_index) {
	// Place the Keel Mango and Courage Shell
	for (int KM_place_index = 0; KM_place_index < 10; KM_place_index++) {
		for (int CS_place_index = KM_place_index + 1; CS_place_index < 10; CS_place_index++) {
			// Replace the 1st chosen item with the Keel Mango
			enum Type_Sort *kmcs_temp_inventory = copyInventory(inventory);
			kmcs_temp_inventory[KM_place_index] = -1;
			shiftUpToFillNull(kmcs_temp_inventory);
			kmcs_temp_inventory[0] = Keel_Mango;
			
			// Replace the 2nd chosen item with the Courage Shell
			kmcs_temp_inventory[CS_place_index] = -1;
			shiftUpToFillNull(kmcs_temp_inventory);
			kmcs_temp_inventory[0] = Courage_Shell;
			
			// Ensure the Thunder Rage is still in the inventory
			int TR_use_index = indexOfItemInInventory(kmcs_temp_inventory, Thunder_Rage);
			if (TR_use_index == -1) {
				// Thunder Rage is no longer in the inventory.
				free(kmcs_temp_inventory);
				continue;
			}
			
			// The Thunder Rage is still in the inventory.
			// The next event is using the Thunder Rage item before resuming the 2nd session of recipe fulfillment
			if (TR_use_index < 10) {
				// Using the Thunder Rage will cause a NULL to appear in that slot
				kmcs_temp_inventory[TR_use_index] = -1;
			}
			
			// Calculate the frames of these actions
			int temp_frames_KM = TOSS_FRAMES + invFrames[20-countNullsInInventory(kmcs_temp_inventory, 10, 20) -1][KM_place_index];
			int temp_frames_CS = TOSS_FRAMES + invFrames[20-countNullsInInventory(kmcs_temp_inventory, 10, 20) -1][CS_place_index];
			int temp_frames_TR = 		    invFrames[20-countNullsInInventory(kmcs_temp_inventory, 10, 20) -1][TR_use_index];
			int temp_frame_sum = frames_DB + frames_CO + temp_frames_KM + temp_frames_CS + temp_frames_TR + sort_frames;
			
			// Determine if the remaining inventory is sufficient to fulfill all remaining recipes
			if (stateOK(kmcs_temp_inventory, outputsFulfilled, recipeList)) {
				struct CH5 *ch5Data = createChapter5Struct(DB_place_index, CO_place_index, KM_place_index, CS_place_index, TR_use_index, sort, 0);
				finalizeChapter5Eval(node, kmcs_temp_inventory, sort, ch5Data, temp_frame_sum, outputsFulfilled, numOutputsFulfilled);
			}
			
			free(kmcs_temp_inventory);
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleChapter5Eval
 * Inputs	: struct BranchPath	*node
 *		  enum Type_Sort	*inventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			frames_DB
 *		  int			frames_CO
 *		  int			DB_place_index
 *		  int			CO_place_index
 *
 * Main Chapter 5 evaluation function. After allocating Dried Bouquet
 * and Coconut in the caller function, try performing a sort before
 * grabbing the Keel Mango and evaluate legal moves. Afterwards, try
 * placing the Keel Mango by tossing various inventory items and
 * evaluate legal moves.
 -------------------------------------------------------------------*/
void handleChapter5Eval(struct BranchPath *node, enum Type_Sort *inventory, int *outputsFulfilled, int numOutputsFulfilled, int frames_DB, int frames_CO, int DB_place_index, int CO_place_index) {
	// Evaluate sorting before the Keel Mango
	handleChapter5Sorts(node, inventory, outputsFulfilled, numOutputsFulfilled, frames_DB, frames_CO, -1, DB_place_index, CO_place_index, -1);
	
	// Evaluate sorting after the Keel Mango
	// Default Keel Mango placement bounds
	int KM_upper_bound = 10;
	
	// Restrict the bounds if there is still a NULL in the inventory
	// because the Keel Mango can only go in the first slot
	if (countNullsInInventory(inventory, 0, 10) >= 1) {
		KM_upper_bound = 1;
	}
	
	// Place the Keel Mango
	for (int KM_place_index = 0; KM_place_index < KM_upper_bound; KM_place_index++) {
		// Making a copy of the temp inventory for what it looks like after the allocation of the KM
		enum Type_Sort *km_temp_inventory = copyInventory(inventory);
		int temp_frames_KM;
		
		// Calculate the needed frames
		if (KM_upper_bound == 10) {
			temp_frames_KM = TOSS_FRAMES + invFrames[20-countNullsInInventory(inventory, 10, 20) -1][KM_place_index];
			km_temp_inventory[KM_place_index] = -1;
		}
		else {
			temp_frames_KM = 0;
		}
		
		// Update the inventory such that all items above the null are moved down one place
		shiftUpToFillNull(km_temp_inventory);

		// The vacancy at the start of the inventory is now occupied with the new item
		km_temp_inventory[0] = Keel_Mango;
		
		// Ensure the Coconut is in the remaining inventory
		if (indexOfItemInInventory(km_temp_inventory, Coconut) == -1) {
			free(km_temp_inventory);
			continue;
		}
		
		// Perform all sorts
		handleChapter5Sorts(node, km_temp_inventory, outputsFulfilled, numOutputsFulfilled, frames_DB, frames_CO, temp_frames_KM, DB_place_index, CO_place_index, KM_place_index);
		free(km_temp_inventory);
	}		
}

/*-------------------------------------------------------------------
 * Function 	: handleChapter5LateSortEndItems
 * Inputs	: struct BranchPath	*node
 *		  enum Type_Sort	*inventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			sort_frames
 *		  enum Action		sort
 *		  int			frames_DB
 *		  int			frames_CO
 *		  int			frames_KM
 *		  int			DB_place_index
 *		  int			CO_place_index
 *		  int			KM_place_index
 *
 * Evaluate Chapter 5 such that a sort occurs after grabbing the
 * Keel Mango. Place the Courage Shell in various inventory locations.
 * Determine if a move is legal.
 -------------------------------------------------------------------*/
void handleChapter5LateSortEndItems(struct BranchPath *node, enum Type_Sort *inventory, int *outputsFulfilled, int numOutputsFulfilled, int sort_frames, enum Action sort, int frames_DB, int frames_CO, int frames_KM, int DB_place_index, int CO_place_index, int KM_place_index) {
	// Place the Courage Shell
	for (int CS_place_index = 0; CS_place_index < 10; CS_place_index++) {
		// Replace the chosen item with the Courage Shell
		enum Type_Sort *cs_temp_inventory = copyInventory(inventory);
		
		cs_temp_inventory[CS_place_index] = -1;
		shiftUpToFillNull(cs_temp_inventory);
		cs_temp_inventory[0] = Courage_Shell;
		
		// Ensure that the Thunder Rage is still in the inventory
		int TR_use_index = indexOfItemInInventory(cs_temp_inventory, Thunder_Rage);
		if (TR_use_index == -1) {
			free(cs_temp_inventory);
			continue;
		}
		
		// The next event is using the Thunder Rage item before resuming the 2nd session of recipe fulfillment
		if (TR_use_index < 10) {
			// Using the  Thunder Rage will cause a NULL to appear in that slot
			cs_temp_inventory[TR_use_index] = -1;
		}
		// Calculate the frames of these actions
		int temp_frames_CS = TOSS_FRAMES + invFrames[20-countNullsInInventory(cs_temp_inventory, 10, 20) -1][CS_place_index];
		int temp_frames_TR = 		    invFrames[20-countNullsInInventory(cs_temp_inventory, 10, 20) -1][TR_use_index];
		int temp_frame_sum = frames_DB + frames_CO + frames_KM + temp_frames_CS + temp_frames_TR + sort_frames;
		
		if (stateOK(cs_temp_inventory, outputsFulfilled, recipeList)) {
			struct CH5 *ch5Data = createChapter5Struct(DB_place_index, CO_place_index, KM_place_index, CS_place_index, TR_use_index, sort, 1);
			finalizeChapter5Eval(node, cs_temp_inventory, sort, ch5Data, temp_frame_sum, outputsFulfilled, numOutputsFulfilled);
		}
			
		free(cs_temp_inventory);
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleChapter5Sorts
 * Inputs	: struct BranchPath	*node
 *		  enum Type_Sort	*inventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			sort_frames
 *		  enum Action		sort
 *		  int			frames_DB
 *		  int			frames_CO
 *		  int			frames_KM
 *		  int			DB_place_index
 *		  int			CO_place_index
 *		  int			KM_place_index
 *
 * Perform various sorts on the inventory during Chapter 5 evaluation.
 * Only continue if a sort places the Coconut in slots 11-20.
 * Then, call an EndItems function to finalize the CH5 evaluation.
 -------------------------------------------------------------------*/
void handleChapter5Sorts(struct BranchPath *node, enum Type_Sort *inventory, int *outputsFulfilled, int numOutputsFulfilled, int frames_DB, int frames_CO, int frames_KM, int DB_place_index, int CO_place_index, int KM_place_index) {
	for (enum Action action = Sort_Alpha_Asc; action <= Sort_Type_Des; action++) {
		enum Type_Sort *sorted_inventory = getSortedInventory(inventory, action);
		
		// Only bother with further evaluation if the sort placed the Coconut in the latter half of the inventory
		// because the Coconut is needed for duplication
		if (indexOfItemInInventory(sorted_inventory, Coconut) < 10) {
			free(sorted_inventory);
			continue;
		}
		
		// Handle all placements of the Keel Mango, Courage Shell, and usage of the Thunder Rage
		int sortFrames;
		switch (action) {
			case Sort_Alpha_Asc :
				sortFrames = ALPHA_SORT_FRAMES;
				break;
			case Sort_Alpha_Des :
				sortFrames = REVERSE_ALPHA_SORT_FRAMES;
				break;
			case Sort_Type_Asc :
				sortFrames = TYPE_SORT_FRAMES;
				break;
			case Sort_Type_Des :
				sortFrames = REVERSE_TYPE_SORT_FRAMES;
				break;
			default :
				// Critical error if we reach this point...
				// action should be some type of sort
				exit(-2);
		}
		
		if (frames_KM == -1) {
			handleChapter5EarlySortEndItems(node, sorted_inventory, outputsFulfilled, numOutputsFulfilled, sortFrames, action, frames_DB, frames_CO, DB_place_index, CO_place_index);
			free(sorted_inventory);
			continue;
		}
		
		handleChapter5LateSortEndItems(node, sorted_inventory, outputsFulfilled, numOutputsFulfilled, sortFrames, action, frames_DB, frames_CO, frames_KM, DB_place_index, CO_place_index, KM_place_index);
		free(sorted_inventory);
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleDBCOAllocation0Nulls
 * Inputs	: struct BranchPath	*curNode
 *		  enum Type_Sort	*tempInventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			viableItems
 *
 * Preliminary function to allocate Dried Bouquet and Coconut before
 * evaluating the rest of Chapter 5. There are no nulls in the inventory.
 -------------------------------------------------------------------*/
void handleDBCOAllocation0Nulls(struct BranchPath *curNode, enum Type_Sort *tempInventory, int *tempOutputsFulfilled, int numOutputsFulfilled, int viableItems) {
	// No nulls to utilize for Chapter 5 intermission
	// Both the DB and CO can only replace items in the first 10 slots
	// The remaining items always slide down to fill the vacancy
	// The DB will eventually end up in slot #2 and
	// the CO will eventually end up in slot #1
	for (int temp_index_DB = 0; temp_index_DB < 10; temp_index_DB++) {
		for (int temp_index_CO = temp_index_DB + 1; temp_index_CO < 10; temp_index_CO++) {
			// Replace the 1st chosen item with the Dried Bouquet
			enum Type_Sort *dbco_temp_inventory = copyInventory(tempInventory);
			dbco_temp_inventory[temp_index_DB] = -1;
			shiftUpToFillNull(dbco_temp_inventory);
			dbco_temp_inventory[0] = Dried_Bouquet;
			
			// Replace the 2nd chosen item with the Coconut
			dbco_temp_inventory[temp_index_CO] = -1;
			shiftUpToFillNull(dbco_temp_inventory);
			dbco_temp_inventory[0] = Coconut;
			
			// Calculate the frames of these actions
			int temp_frames_DB = TOSS_FRAMES + invFrames[viableItems-1][temp_index_DB];
			int temp_frames_CO = TOSS_FRAMES + invFrames[viableItems-1][temp_index_CO];
			
			// Only evaulate the remainder of the CH5 intermission if the Thunder Rage is still present in the inventory
			if (indexOfItemInInventory(dbco_temp_inventory, Thunder_Rage) > -1) {
				// Handle the allocation of the Coconut sort, Keel Mango, and Courage Shell
				handleChapter5Eval(curNode, dbco_temp_inventory, tempOutputsFulfilled, numOutputsFulfilled, temp_frames_DB, temp_frames_CO, temp_index_DB, temp_index_CO);
			}
			
			free(dbco_temp_inventory);
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleDBCOAllocation1Null
 * Inputs	: struct BranchPath	*curNode
 *		  enum Type_Sort		*tempInventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			viableItems
 *
 * Preliminary function to allocate Dried Bouquet and Coconut before
 * evaluating the rest of Chapter 5. There is 1 null in the inventory.
 -------------------------------------------------------------------*/
void handleDBCOAllocation1Null(struct BranchPath *curNode, enum Type_Sort *tempInventory, int *tempOutputsFulfilled, int numOutputsFulfilled, int viableItems) {
	// The Dried Bouquet gets auto-placed in the 1st slot,
	// and everything else gets shifted down one to fill the first NULL
	shiftUpToFillNull(tempInventory);
	
	// The vacancy at the start of the inventory is now occupied with the new item
	tempInventory[0] = Dried_Bouquet;
	/*int tempSlotDB = 0;*/
	viableItems++;
	
	// Auto-placing takes zero frames
	int temp_frames_DB = 0;
	
	// The Coconut can only be placed in the first 10 slots
	// Dried Bouquet will always be in the first slot
	for (int temp_index_CO = 1; temp_index_CO < 10; temp_index_CO++) {
		// Don't waste time replacing the Dried Bouquet or Thunder Rage with the Coconut
		if (tempInventory[temp_index_CO] == Thunder_Rage) {
			continue;
		}
		// Replace the item with the Coconut
		// All items above the replaced item float down one slot
		// and the Coconut is always placed in slot 1
		enum Type_Sort *co_temp_inventory = copyInventory(tempInventory);
		co_temp_inventory[temp_index_CO] = -1;
		shiftUpToFillNull(co_temp_inventory);
		co_temp_inventory[0] = Coconut;
		
		// Calculate the number of frames needed to pick this slot for replacement
		int temp_frames_CO = TOSS_FRAMES + invFrames[viableItems-1][temp_index_CO];
		
		// Handle the allocation of the Coconut sort, Keel Mango, and Courage Shell
		handleChapter5Eval(curNode, co_temp_inventory, tempOutputsFulfilled, numOutputsFulfilled, temp_frames_DB, temp_frames_CO, 0, temp_index_CO);
		
		free(co_temp_inventory);
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleDBCOAllocation2Nulls
 * Inputs	: struct BranchPath	*curNode
 *		  enum Type_Sort	*tempInventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			viableItems
 *
 * Preliminary function to allocate Dried Bouquet and Coconut before
 * evaluating the rest of Chapter 5. There are >=2 nulls in the inventory.
 -------------------------------------------------------------------*/
void handleDBCOAllocation2Nulls(struct BranchPath *curNode, enum Type_Sort *tempInventory, int *tempOutputsFulfilled, int numOutputsFulfilled, int viableItems) {
	// The Dried Bouquet gets auto-placed in the 1st slot,
	// and everything else gets shifted down one slot to fill the first NULL
	shiftUpToFillNull(tempInventory);
	
	// The vacancy at the start of the inventory is now occupied with the new item
	tempInventory[0] = Dried_Bouquet;
	viableItems++;
	
	// The Coconut gets auto-placed in the 1st slot
	// and everything else gets shifted down one slot to fill the first NULL
	shiftUpToFillNull(tempInventory);
	
	// The vacancy at the start of the inventory is now occupied with the new item
	tempInventory[0] = Coconut;
	viableItems++;
	
	// Auto-placing takes 0 frames
	int tempFramesDB = 0;
	int tempFramesCO = 0;
	
	// Handle the allocation of the Coconut, Sort, Keel Mango, and Courage Shell
	handleChapter5Eval(curNode, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, tempFramesDB, tempFramesCO, 0, 0);

}

/*-------------------------------------------------------------------
 * Function 	: handleRecipeOutput
 * Inputs	: struct BranchPath		*curNode
 *		  enum Type_Sort		*tempInventory
 *		  int				tempFrames
 *		  struct MoveDescription	useDescription
 *		  int				*tempOutputsFulfilled
 *		  int				numOutputsFulfilled
 *		  enum Type_Sortf		output
 *		  int				viableItems
 *
 * After detecting that a recipe can be satisfied, see how we can handle
 * the output (either tossing the output, auto-placing it if there is a
 * null slot, or tossing a different item in the inventory)
 -------------------------------------------------------------------*/
void handleRecipeOutput(struct BranchPath *curNode, enum Type_Sort *tempInventory, int tempFrames, struct MoveDescription useDescription, int *tempOutputsFulfilled, int numOutputsFulfilled, enum Type_Sort output, int viableItems) {
	// Options vary by whether there are NULLs within the inventory
	if (countNullsInInventory(tempInventory, 0, 10) >= 1) {
		// If there are NULLs in the inventory, all items before the 1st NULL get shifted down 1 position
		shiftUpToFillNull(tempInventory);
	
		// The vacancy at the start of the inventory is now occupied with the new item
		tempInventory[0] = ((struct Cook *) useDescription.data)->output;
	
		// Check to see if this state is viable
		if(stateOK(tempInventory, tempOutputsFulfilled, recipeList) == 1) {
			finalizeLegalMove(curNode, tempFrames, useDescription, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, Autoplace, -1, -1);
		}
	}
	else {
		// There are no NULLs in the inventory. Something must be tossed
		// Total number of frames increased by forcing to toss something
		tempFrames += TOSS_FRAMES;
		useDescription.framesTaken += TOSS_FRAMES;
		useDescription.totalFramesTaken += TOSS_FRAMES;
		
		// Evaluate viability of tossing the output item itself
		if (stateOK(tempInventory, tempOutputsFulfilled, recipeList) == 1) {
			finalizeLegalMove(curNode, tempFrames, useDescription, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, Toss, output, -1);
		}
		
		// Evaluate the viability of tossing all current inventory items
		// Assumed that it is impossible to toss and replace any items in the last 10 positions
		tryTossInventoryItem(curNode, tempInventory, useDescription, tempOutputsFulfilled, numOutputsFulfilled, output, tempFrames, viableItems);
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleSelectAndRandom
 * Inputs	: struct BranchPath	*curNode
 *		  int 			select
 *		  int 			randomise
 *
 * Based on configuration parameters select and randomise within config.txt,
 * manage the array of legal moves based on the designated behavior of the parameters.
 -------------------------------------------------------------------*/
void handleSelectAndRandom(struct BranchPath *curNode, int select, int randomise) {
	/*if (select && curNode->moves < 55 && curNode->numLegalMoves > 0) {
		softMin(curNode);
	}*/
	
	// Old method of handling select
	// Somewhat random process of picking the quicker moves to recurse down
	// Arbitrarily skip over the fastest legal move with a given probability
	if (select && curNode->moves < 55 && curNode->numLegalMoves > 0) {
		int nextMoveIndex = 0;
		while (nextMoveIndex < curNode->numLegalMoves - 1 && rand() % 100 < 50) {
			nextMoveIndex++;
		}

		// Take the legal move at nextMoveIndex and move it to the front of the array
		struct BranchPath *nextMove = curNode->legalMoves[nextMoveIndex];
		curNode->legalMoves[nextMoveIndex] = NULL;
		shiftDownLegalMoves(curNode, 0, nextMoveIndex);
		curNode->legalMoves[0] = nextMove;
	}
	
	// When not doing the select methodology, and opting for randomize
	// just shuffle the entire list of legal moves and pick the new first item
	else if (randomise) {
		shuffleLegalMoves(curNode);
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleSorts
 * Inputs	: struct BranchPath	*curNode
 *
 * Perform the 4 different sorts, determine if they changed the inventory,
 * and if so, generate a legal move to represent the sort.
 -------------------------------------------------------------------*/
void handleSorts(struct BranchPath *curNode) {
	// Count the number of sorts for capping purposes
	// Limit the number of sorts allowed in a roadmap
	// NOTE: Reduced from 10 to 6 based off of the current set of fastest roadmaps
	if (countTotalSorts(curNode) < 6) {
		// Perform the 4 different sorts
		for (enum Action sort = Sort_Alpha_Asc; sort <= Sort_Type_Des; sort++) {
			enum Type_Sort *sorted_inventory = getSortedInventory(curNode->inventory, sort);
		
			// Only add the legal move if the sort actually changes the inventory
			if (compareInventories(sorted_inventory, curNode->inventory) == 0) {
				struct MoveDescription description;
				description.action = sort;
				description.data = NULL;
				int sortFrames;
				switch (sort) {
					case Sort_Alpha_Asc :
						sortFrames = ALPHA_SORT_FRAMES;
						break;
					case Sort_Alpha_Des :
						sortFrames = REVERSE_ALPHA_SORT_FRAMES;
						break;
					case Sort_Type_Asc :
						sortFrames = TYPE_SORT_FRAMES;
						break;
					case Sort_Type_Des :
						sortFrames = REVERSE_TYPE_SORT_FRAMES;
						break;
					default :
						// Something went wrong if we reach this point...
						exit(-2);
				}
				generateFramesTaken(&description, curNode, sortFrames);
				description.framesTaken = sortFrames;
				int *copyOfOutputsFulfilled = copyOutputsFulfilled(curNode->outputCreated);
				
				// Create the legalMove node
				struct BranchPath *newLegalMove = createLegalMove(curNode, sorted_inventory, description, copyOfOutputsFulfilled, curNode->numOutputsCreated);
				
				// Insert this new move into the current node's legalMove array
				insertIntoLegalMoves(curNode->numLegalMoves, newLegalMove, curNode);
				assert(curNode->legalMoves[curNode->numLegalMoves-1] != NULL);
			}
			else {
				free(sorted_inventory);
			}
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: initializeRoot
 * Inputs	: struct Job		job
 * Outputs	: struct BranchPath	*root
 *
 * Generate the root of the tree graph
 -------------------------------------------------------------------*/
struct BranchPath *initializeRoot(struct Job job) {
	struct BranchPath *root = malloc(sizeof(struct BranchPath));
	root->moves = 0;
	root->inventory = job.startingInventory;
	root->description.action = Begin;
	root->description.data = NULL;
	root->description.framesTaken = 0;
	root->description.totalFramesTaken = 0;
	root->prev = NULL;
	root->next = NULL;
	root->outputCreated = calloc(NUM_RECIPES, sizeof(int));
	root->numOutputsCreated = 0;
	root->legalMoves = NULL;
	root->numLegalMoves = 0;
	return root;
}

/*-------------------------------------------------------------------
 * Function 	: insertIntoLegalMoves
 * Inputs	: int			insertIndex
 *		  struct BranchPath	*newLegalMove
 *		  struct BranchPath	*curNode
 *
 * Determine where in curNode's legalmove array the new legal move should
 * be inserted. This is necessary because our current implementation
 * arranges legal moves in ascending order based on the number of frames
 * it takes to complete the legal move.
 -------------------------------------------------------------------*/
void insertIntoLegalMoves(int insertIndex, struct BranchPath *newLegalMove, struct BranchPath *curNode) {
	// Reallocate the legalMove array to make room for a new legal move
	curNode->legalMoves = realloc(curNode->legalMoves, sizeof(struct BranchPath *) * (curNode->numLegalMoves + 1));
	
	// Shift all legal moves further down the array to make room for a new legalMove
	shiftDownLegalMoves(curNode, insertIndex, curNode->numLegalMoves);
	/*for (int i = curNode->numLegalMoves - 1; i >= insertIndex; i--) {
		curNode->legalMoves[i+1] = curNode->legalMoves[i];
	}*/
	
	// Place newLegalMove in index insertIndex
	curNode->legalMoves[insertIndex] = newLegalMove;
	
	// Increase numLegalMoves
	curNode->numLegalMoves++;
	
	return;
}

/*-------------------------------------------------------------------
 * Function 	: copyAllNodes
 * Inputs	: struct BranchPath	*newNode
 *		  struct BranchPath	*oldNode
 * Outputs	: struct BranchPath	*newNode
 *
 * Duplicate all the contents of a roadmap to a new memory region.
 * This is used for optimizeRoadmap.
 -------------------------------------------------------------------*/
struct BranchPath *copyAllNodes(struct BranchPath *newNode, struct BranchPath *oldNode) {
	do {
		newNode->moves = oldNode->moves;
		newNode->inventory = copyInventory(oldNode->inventory);
		newNode->description = oldNode->description;
		switch (newNode->description.action) {
			case (Begin) :
				newNode->description.data = NULL;
				break;
			case (Sort_Alpha_Asc) :
				newNode->description.data = NULL;
				break;
			case (Sort_Alpha_Des) :
				newNode->description.data = NULL;
				break;
			case (Sort_Type_Asc) :
				newNode->description.data = NULL;
				break;
			case (Sort_Type_Des) :
				newNode->description.data = NULL;
				break;
			case (Cook) :
				newNode->description.data = malloc(sizeof(struct Cook));
				copyCook((struct Cook *)newNode->description.data, (struct Cook *)oldNode->description.data);
				break;
			case (Ch5) :
				newNode->description.data = malloc(sizeof(struct CH5));
				struct CH5 *newData = (struct CH5 *)newNode->description.data;
				struct CH5 *oldData = (struct CH5 *)oldNode->description.data;
				newData->indexDriedBouquet = oldData->indexDriedBouquet;
				newData->indexCoconut =  oldData->indexCoconut;
				newData->ch5Sort = oldData->ch5Sort;
				newData->indexKeelMango = oldData->indexKeelMango;
				newData->indexCourageShell = oldData->indexCourageShell;
				newData->indexThunderRage = oldData->indexThunderRage;
				newData->lateSort = oldData->lateSort;
				break;
			default :
				break;
		}
		
		int *newOutputCreated = copyOutputsFulfilled(oldNode->outputCreated);
		newNode->outputCreated = newOutputCreated;
		newNode->numOutputsCreated = oldNode->numOutputsCreated;
		newNode->legalMoves = NULL;
		newNode->numLegalMoves = 0;
		if (newNode->numOutputsCreated < NUM_RECIPES) {
			newNode->next = malloc(sizeof(struct BranchPath));
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
 * Function 	: optimizeRoadmap
 * Inputs	: struct BranchPath		*root
 * Outputs	: struct OptimizeResult	result
 *
 * Given a complete roadmap, attempt to rearrange recipes such that they
 * are placed in more efficient locations in the roadmap. This is effective
 * in shaving off upwards of 100 frames off of a roadmap.
 * TODO: Compartmentalize more of this function
 -------------------------------------------------------------------*/
struct OptimizeResult optimizeRoadmap(struct BranchPath *root) {
	// First copy all nodes to new memory locations so we can begin rearranging nodes
	struct BranchPath *curNode = root;
	struct BranchPath *newNode = malloc(sizeof(struct BranchPath));
	struct BranchPath *newRoot = newNode;
	newNode->prev = NULL;
	newNode = copyAllNodes(newNode, curNode);
	
	// List of recipes that can be potentially rearranged into a better location within the roadmap
	enum Type_Sort *rearranged_recipes = NULL;
	int num_rearranged_recipes = 0;
	
	// Determine which steps can be rearranged
	// Evaluate the list in reverse order for easy list manipulation
	// Ignore the "Begin" node
	// Ignore the next node, as it must always have a 1-ingredient recipe
	// 	and we don't want to risk moving it, causing an invalid map
	
	// Ignore the last instance as the mistake can almost always be cooked last
	newNode = newNode->prev;
	
	while (newNode->moves > 1) {
		// Ignore sorts
		if (newNode->description.action != Cook) {
			newNode = newNode->prev;
			continue;
		}
		
		// Ignore recipes which do not toss the output
		struct Cook *cookData = (struct Cook *) newNode->description.data;
		if (cookData->handleOutput != Toss) {
			newNode = newNode->prev;
			continue;
		}
		
		// At this point, we have a recipe which tosses the output
		// This output can potentially be relocated to a quicker time
		enum Type_Sort tossed_item = cookData->output;
		rearranged_recipes = realloc(rearranged_recipes, sizeof(enum Type_Sort) * (num_rearranged_recipes + 1));
		rearranged_recipes[num_rearranged_recipes] = tossed_item;
		num_rearranged_recipes++;
		
		// First update subsequent nodes to remove this item from outputCreated
		struct BranchPath *node = newNode->next;
		while (node != NULL) {
			node->outputCreated[getIndexOfRecipe(tossed_item)] = 0;
			node->numOutputsCreated--;
			node = node->next;
		}
		
		// Now, get rid of this current node
		newNode->prev->next = newNode->next;
		newNode->next->prev = newNode->prev;
		node = newNode->prev;
		freeNode(newNode);
		newNode = node;
	}
	
	// Now that all rearranged items have been removed,
	// find the optimal place they can be inserted again, such that they don't affect the inventory
	for (int recipe_offset = 0; recipe_offset < num_rearranged_recipes; recipe_offset++) {
		// Establish a default bound for the optimal place for this item
		int record_frames = 9999;
		struct BranchPath *record_placement_node = NULL;
		struct Cook *record_description = NULL;
		
		// Evaluate all recipes and determine the optimal recipe and location
		int recipe_index = getIndexOfRecipe(rearranged_recipes[recipe_offset]);
		struct Recipe recipe = recipeList[recipe_index];
		for (int recipe_combo_index = 0; recipe_combo_index < recipe.countCombos; recipe_combo_index++) {
			struct ItemCombination combo = recipe.combos[recipe_combo_index];
			newNode = newRoot;
			
			// Look at every node
			while (newNode != NULL) {
				// Only want moments when there are no NULLs in the inventory
				if (countNullsInInventory(newNode->inventory, 0, 10) > 0) {
					newNode = newNode->next;
					continue;
				}
				
				// Only want recipes where all ingredients are in the last 10 slots of the evaluated inventory
				int indexItem1 = indexOfItemInInventory(newNode->inventory, combo.item1);
				int indexItem2 = indexOfItemInInventory(newNode->inventory, combo.item2);
				if (indexItem1 == -1 || (combo.numItems == 2 && indexItem2 == -1)) {
					newNode = newNode->next;
					continue;
				}
				
				if (indexItem1 < 10 || (combo.numItems == 2 && indexItem2 < 10)) {
					newNode = newNode->next;
					continue;
				}
				
				// This is a valid recipe and location to fulfill (and toss) the output
				// Calculate the frames needed to produce this step
				int temp_frames = TOSS_FRAMES;
				struct Cook *temp_description = malloc(sizeof(struct Cook));
				temp_description->output = recipe.output;
				temp_description->handleOutput = Toss;
				
				if (combo.numItems == 1) {
					// Only one ingredient to navigate to
					temp_frames += invFrames[20 - countNullsInInventory(newNode->inventory, 10, 20)][indexItem1];
					temp_description->numItems = 1;
					temp_description->item1 = combo.item1;
					temp_description->itemIndex1 = indexItem1;
					temp_description->item2 = -1;
					temp_description->itemIndex2 = -1;
				}
				else {
					// Two ingredients to navigate to, but order matters
					// Pick the larger-index number ingredient first, as it will reduce
					// the frames needed to reach the other ingredient
					temp_frames += CHOOSE_2ND_INGREDIENT_FRAMES;
					temp_description->numItems = 2;
					
					if (indexItem1 > indexItem2) {
						temp_frames += invFrames[20 - countNullsInInventory(newNode->inventory, 10, 20)][indexItem1];
						temp_frames += invFrames[19 - countNullsInInventory(newNode->inventory, 10, 20)][indexItem2];
						temp_description->item1 = combo.item1;
						temp_description->itemIndex1 = indexItem1;
						temp_description->item2 = combo.item2;
						temp_description->itemIndex2 = indexItem2;
					}
					else {
						temp_frames += invFrames[20 - countNullsInInventory(newNode->inventory, 10, 20)][indexItem1];
						temp_frames += invFrames[19 - countNullsInInventory(newNode->inventory, 10, 20)][indexItem2];
						temp_description->item1 = combo.item2;
						temp_description->itemIndex1 = indexItem2;
						temp_description->item2 = combo.item1;
						temp_description->itemIndex2 = indexItem1;
					}
				}
				
				// Compare the temp frames to the current record
				if (temp_frames < record_frames) {
					// Update the record information
					record_frames = temp_frames;
					record_placement_node = newNode;
					
					// If we are overwriting a previous record, free the previous description
					if (record_description != NULL) {
						free(record_description);
					}
					record_description = temp_description;
				}
				else {
					free(temp_description);
				}
				
				newNode = newNode->next;
			}
		}
		
		// All recipe combos and intervals have been evaluated
		// Insert the optimized output in the designated interval
		if (record_placement_node == NULL) {
			// This is an error
			recipeLog(7, "Calculator", "Roadmap", "Optimize", "OptimizeRoadmap couldn't find a valid placement...");
			exit(1);
		}
		
		newNode = newRoot;
		while (newNode != record_placement_node) {
			newNode = newNode->next;
		}
		
		struct BranchPath *insertNode = malloc(sizeof(struct BranchPath));
		insertNode->prev = newNode;
		newNode->next->prev = insertNode;
		insertNode->next = newNode->next;
		newNode->next = insertNode;
		insertNode->moves = insertNode->prev->moves + 1;
		insertNode->inventory = copyInventory(newNode->inventory);
		insertNode->description.action = Cook;
		insertNode->description.data = (void *) record_description;
		insertNode->description.framesTaken = record_frames;
		int *outputsCreated = copyOutputsFulfilled(newNode->outputCreated);
		outputsCreated[recipe_index] = 1;
		insertNode->outputCreated = outputsCreated;
		insertNode->numOutputsCreated = newNode->numOutputsCreated + 1;
		
		// Go through all subsequent nodes to specify this output has been created
		newNode = insertNode->next;
		while (newNode != NULL) {
			newNode->outputCreated[recipe_index] = 1;
			newNode->numOutputsCreated++;
			newNode = newNode->next;
		}
		insertNode->legalMoves = NULL;
		insertNode->numLegalMoves = 0;
	}
	
	// All items have been rearranged and placed into a new roadmap
	// Recalculate the total frame count
	newNode = newRoot;
	while (newNode->next != NULL) {
		newNode = newNode->next;
		newNode->description.totalFramesTaken = newNode->description.framesTaken + newNode->prev->description.totalFramesTaken;
	}
	
	struct OptimizeResult result;
	result.root = newRoot;
	result.last = newNode;
	return result;
}

/*-------------------------------------------------------------------
 * Function 	: popAllButFirstLegalMove
 * Inputs	: struct BranchPath	*node
 *
 * In the event we are within the last few nodes of the roadmap, get rid
 * of all but the fastest legal move.
 -------------------------------------------------------------------*/
void popAllButFirstLegalMove(struct BranchPath *node) {
	for (int i = 1; i < node->numLegalMoves; i++) {
		freeLegalMove(node, i);
		i--;
	}
	
	assert(node->numLegalMoves <= 1);
	
	return;
}

/*-------------------------------------------------------------------
 * Function 	: printCh5Data
 * Inputs	: struct BranchPath		*curNode
 *		  struct MoveDescription 	desc
 *		  FILE				*fp
 *
 * Print to a txt file the data which pertains to Chapter 5 evaluation
 * (where to place Dried Bouquet, Coconut, etc.)
 -------------------------------------------------------------------*/
void printCh5Data(struct BranchPath *curNode, struct MoveDescription desc, FILE *fp) {
	struct CH5 *ch5Data = desc.data;
	fprintf(fp, "Ch.5 Break: Replace #%d for DB, Replace #%d for CO, ", ch5Data->indexDriedBouquet, ch5Data->indexCoconut);
	if (ch5Data->lateSort == 0) {
		printCh5Sort(ch5Data, fp);
		fprintf(fp, "Replace #%d for KM, Replace #%d for CS, Use TR in #%d\t", ch5Data->indexKeelMango, ch5Data->indexCourageShell, ch5Data->indexThunderRage);
		return;
	}
	
	fprintf(fp, "Replace #%d for KM, ", ch5Data->indexKeelMango);
	printCh5Sort(ch5Data, fp);
	fprintf(fp, "Replace #%d for CS, Use TR in #%d\t", ch5Data->indexCourageShell, ch5Data->indexThunderRage);
}

/*-------------------------------------------------------------------
 * Function 	: printCh5Sort
 * Inputs	: struct CH5	*ch5Data
 *		  FILE		*fp
 *
 * Print to a txt file the data which pertains to Chapter 5 sorting
 -------------------------------------------------------------------*/
void printCh5Sort(struct CH5 *ch5Data, FILE *fp) {
	fprintf(fp, "Sort (");
	switch (ch5Data->ch5Sort) {
		case Sort_Alpha_Asc :
			fprintf(fp, "Alpha), ");
			break;
		case Sort_Alpha_Des :
			fprintf(fp, "Reverse-Alpha), ");
			break;
		case Sort_Type_Asc :
			fprintf(fp, "Type), ");
			break;
		case Sort_Type_Des :
			fprintf(fp, "Reverse-Type), ");
			break;
		default :
			fprintf(fp, "ERROR IN CH5SORT SWITCH CASE");
	};
}

/*-------------------------------------------------------------------
 * Function 	: printCookData
 * Inputs	: struct BranchPath 		*curNode
 *		  struct MoveDescription 	desc
 *		  FILE				*fp
 *
 * Print to a txt file the data which pertains to cooking a recipe,
 * which includes what items were used and what happens to the output.
 -------------------------------------------------------------------*/
void printCookData(struct BranchPath *curNode, struct MoveDescription desc, FILE *fp) {
	struct Cook *cookData = desc.data;
	fprintf(fp, "Use [%s] in slot %d ", getItemName(cookData->item1), cookData->itemIndex1 + 1);
	
	if (cookData->numItems == 2) {
		fprintf(fp, "and [%s] in slot %d ", getItemName(cookData->item2), cookData->itemIndex2 + 1);
	}
	fputs("to make ", fp);
	
	if (cookData->handleOutput == Toss) {
		fputs("(and toss) ", fp);
	}
	else if (cookData->handleOutput == Autoplace) {
		fputs("(and auto-place) ", fp);
	}
	fprintf(fp, "<%s>", getItemName(cookData->output));
	
	if (cookData->handleOutput == TossOther) {
		fprintf(fp, ", toss [%s] in slot %d", getItemName(cookData->toss), cookData->indexToss + 1);
	}
	
	if (curNode->numOutputsCreated == NUM_RECIPES && ((struct Cook *) curNode->description.data)->handleOutput == Autoplace) {
		fputs(" (No-Toss 5 Frame Penalty for Jump Storage)", fp);
	}
	else if (curNode->numOutputsCreated == NUM_RECIPES) {
		fputs(" (Jump Storage on Tossed Item)", fp);
	}
	fputs("\t", fp);
}

/*-------------------------------------------------------------------
 * Function 	: printFileHeader
 * Inputs	: FILE				*fp
 *
 * Print to a txt file the header information for the file.
 -------------------------------------------------------------------*/
void printFileHeader(FILE *fp) {
	fputs("Description\tFrames Taken\tTotal Frames", fp);
	for (int i = 0; i < 20; i++) {
		fprintf(fp, "\tSlot #%d", i+1);
	}
	for (int i = 0; i < NUM_RECIPES; i++) {
		fprintf(fp, "\t%s", getItemName(recipeList[i].output));
	}
	fprintf(fp, "\n");
	recipeLog(5, "Calculator", "File", "Write", "Header for new output written");
}

/*-------------------------------------------------------------------
 * Function 	: printInventoryData
 * Inputs	: struct BranchPath	*curNode
 *		  FILE			*fp
 *
 * Print to a txt file the header information for the file.
 -------------------------------------------------------------------*/
void printInventoryData(struct BranchPath *curNode, FILE *fp) {
	for (int i = 0; i < 20; i++) {
		if (curNode->inventory[i] == -1) {
			if (i<10) {
				fprintf(fp, "NULL\t");
			}
			else {
				fprintf(fp, "BLOCKED\t");
			}
			continue;
		}
			
		fprintf(fp, "%s\t", getItemName(curNode->inventory[i]));
	}
}

/*-------------------------------------------------------------------
 * Function 	: printOutputsCreated
 * Inputs	: struct BranchPath	*curNode
 *		  FILE			*fp
 *
 * Print to a txt file data pertaining to which recipes
 * have been cooked thus far.
 -------------------------------------------------------------------*/
void printOutputsCreated(struct BranchPath *curNode, FILE *fp) {
	for (int i = 0; i < NUM_RECIPES; i++) {
		if (curNode->outputCreated[i] == 1) {
			if (i == NUM_RECIPES - 1) {
				fprintf(fp, "True");
			}
			else {
				fprintf(fp, "True\t");
			}
		}
		else {
			if (i == NUM_RECIPES - 1) {
				fprintf(fp, "False");
			}
			else {
				fprintf(fp, "False\t");
			}
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: printResults
 * Inputs	: char			*filename
 *		  struct BranchPath	*path
 *
 * Parent function for children print functions. This parent function
 * is called when a roadmap has been found which beats the current
 * local record.
 -------------------------------------------------------------------*/
int printResults(char *filename, struct BranchPath *path) {
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		printf("Could not locate %s... This is a bug.\n", filename);
		printf("Press ENTER to exit.\n");
		getchar();
		exit(1);
	}
	// Write header information
	printFileHeader(fp);
	
	// Print data information
	struct BranchPath *curNode = path;
	do {
		struct MoveDescription desc = curNode->description;
		enum Action curNodeAction = desc.action;
		switch (curNodeAction) {
			case Cook :
				printCookData(curNode, desc, fp);
				break;
			case Ch5 :
				printCh5Data(curNode, desc, fp);
				break;
			case Begin :
				fputs("Begin\t", fp);
				break;
			default :
				// Some type of sorting
				printSortData(fp, curNodeAction);
		}
		
		// Print out frames taken
		fprintf(fp, "%d\t", curNode->description.framesTaken);
		// Print out total frames taken
		fprintf(fp, "%d\t", curNode->description.totalFramesTaken);

		// Print out inventory
		printInventoryData(curNode, fp);
		
		// Print out whether or not all 58 items were created
		printOutputsCreated(curNode, fp);
		
		// Add newline character to put next node on new line
		fprintf(fp, "\n");
		
		// Traverse to the next node in the roadmap
		curNode = curNode->next;
			
	} while (curNode != NULL);
	
	fclose(fp);
	
	recipeLog(5, "Calculator", "File", "Write", "Data for roadmap written.");
	
	return 0;
}

/*-------------------------------------------------------------------
 * Function 	: printSortData
 * Inputs	: FILE 	*fp
 *		  enum Action 	curNodeAction
 *
 * Print to a file data which pertains to sorting the inventory.
 -------------------------------------------------------------------*/
void printSortData(FILE *fp, enum Action curNodeAction) {
	fprintf(fp, "Sort - ");
	switch (curNodeAction) {
		case Sort_Alpha_Asc :
			fputs("Alphabetical\t", fp);
			break;
		case Sort_Alpha_Des :
			fputs("Reverse Alphabetical\t", fp);
			break;
		case Sort_Type_Asc :
			fputs("Type\t", fp);
			break;
		case Sort_Type_Des :
			fputs("Reverse Type\t", fp);
			break;
		default :
			fputs("ERROR IN HANDLING OF SORT", fp);
	};
}

/*-------------------------------------------------------------------
 * Function 	: selectSecondItemFirst
 * Inputs	: struct BranchPath 		*node
 *		  struct ItemCombination 	combo
 *		  int 				*ingredientLoc
 *		  int 				*ingredientOffset
 *		  int 				viableItems
 * Outputs	: int (0 or 1)
 *
 * Calculates a boolean expression which determines whether it is faster
 * to select the second item before the first item originally listed in
 * the recipe combo.
 -------------------------------------------------------------------*/
int selectSecondItemFirst(struct BranchPath *node, struct ItemCombination combo, int *ingredientLoc, int *ingredientOffset, int viableItems) {
	return (ingredientLoc[0] - ingredientOffset[0] >= 2 && ingredientLoc[0] > ingredientLoc[1] && ingredientLoc[0] - ingredientLoc[0] <= viableItems/2) || (ingredientLoc[0] < ingredientLoc[1] && ingredientLoc[0] - ingredientOffset[0] >= viableItems/2);

}

/*-------------------------------------------------------------------
 * Function 	: shiftDownLegalMoves
 * Inputs	: struct BranchPath	*node
 *		  int			lowerBound
 *		  int			upperBound	
 *
 * If this function is called, we want to make room in the legal moves
 * array to place a new legal move. Shift all legal moves starting at
 * lowerBound one index towards the end of the list, ending at upperBound
 -------------------------------------------------------------------*/
void shiftDownLegalMoves(struct BranchPath *node, int lowerBound, int uppderBound) {
	for (int i = uppderBound - 1; i >= lowerBound; i--) {
		node->legalMoves[i+1] = node->legalMoves[i];
	}
}

/*-------------------------------------------------------------------
 * Function 	: shiftUpLegalMoves
 * Inputs	: struct BranchPath	*node
 *		  int			index	
 *
 * There is a NULL in the array of legal moves. The first valid legal
 * move AFTER the null is index. Iterate starting at the index of the
 * NULL legal moves and shift all subsequent legal moves towards the
 * front of the array.
 -------------------------------------------------------------------*/
void shiftUpLegalMoves(struct BranchPath *node, int startIndex) {
	for (int i = startIndex - 1; i < node->numLegalMoves; i++) {
		node->legalMoves[i] = node->legalMoves[i+1];
	}
	// Null where the last entry was before shifting
	node->legalMoves[node->numLegalMoves] = NULL;
}

/*-------------------------------------------------------------------
 * Function 	: softMin
 * Inputs	: struct BranchPath	*node	
 *
 * This is an experimental function which may substitute the original
 * "select" methodology when determining what next node to explore.
 * This is a variation of the Softmax function. Essentially, the
 * original methodology uses an arbitrary distribution when determining
 * which legal move to take. softMin uses the framecount of each legal
 * move to construct a distribution for which a given node will have a
 * higher probability than subsequent nodes depending on how much faster
 * the given node is compared to the subsequent nodes.
 * For more information on Softmax: https://en.wikipedia.org/wiki/Softmax_function
 -------------------------------------------------------------------*/
void softMin(struct BranchPath *node) {
	// If numLegalMoves is 0 or 1, we will get an error when trying to do x % 0
	if (node->numLegalMoves < 2) {
		return;
	}
	
	// Calculate the sum of framecount for all legalMoves
	int frameCountSum = 0;
	for (int i = 0; i < node->numLegalMoves; i++) {
		frameCountSum += node->legalMoves[i]->description.framesTaken;
	}
	
	// Do some janky shit to recalculate the sum such that the node with the
	// lowest framecount has a higher "value"
	int weightSum = 0;
	for (int i = 0; i < node->numLegalMoves; i++) {
		weightSum += (frameCountSum - node->legalMoves[i]->description.framesTaken);
	}
	
	// Generate a random number between 0 and weightSum
	int modSum = rand() % weightSum;
	
	// Find the legal move that corresponds to the modSum
	int index;
	weightSum = 0;
	for (index = 0; index < node->numLegalMoves; index++) {
		weightSum += (frameCountSum - node->legalMoves[index]->description.framesTaken);
		if (modSum < weightSum) {
			// We have found the corresponding legal move
			break;
		}
	}
	
	// Move the indexth legal move to the front of the array
	// First store the indexth legal move in a separate pointer
	struct BranchPath *softMinNode = node->legalMoves[index];
	node->legalMoves[index] = NULL;
	
	// Make room at the beginning of the legal moves array for the softMinNode
	shiftDownLegalMoves(node, 0, index);
	
	// Set first index in array to the softMinNode
	node->legalMoves[0] = softMinNode;
}

/*-------------------------------------------------------------------
 * Function 	: shuffleLegalMoves
 * Inputs	: struct BranchPath	*node	
 *
 * Randomize the order of legal moves by switching two legal moves
 * numlegalMoves times.
 -------------------------------------------------------------------*/
void shuffleLegalMoves(struct BranchPath *node) {
	// Swap 2 legal moves a variable number of times
	for (int i = 0; i < node->numLegalMoves; i++) {
		int index1 = rand() % node->numLegalMoves;
		int index2 = rand() % node->numLegalMoves;
		struct BranchPath *temp = node->legalMoves[index1];
		node->legalMoves[index1] = node->legalMoves[index2];
		node->legalMoves[index2] = temp;
	}
	
	return;
}

/*-------------------------------------------------------------------
 * Function 	: swapItems
 * Inputs	: int	*ingredientLoc
 *		  int 	*ingredientOffset
 *
 * After determining that it is faster to pick the second item before
 * the first for a given recipe (based on the state of our inventory),
 * swap the item slot locations and offsets (for printing purposes).
 -------------------------------------------------------------------*/
void swapItems(int *ingredientLoc, int *ingredientOffset) {
	int locTemp;
	int offsetTemp;
	
	locTemp = ingredientLoc[0];
	ingredientLoc[0] = ingredientLoc[1];
	ingredientLoc[1] = locTemp;
	
	offsetTemp = ingredientOffset[0];
	ingredientOffset[0] = ingredientOffset[1];
	ingredientOffset[1] = offsetTemp;
	
	return;
}

/*-------------------------------------------------------------------
 * Function 	: tryTossInventoryItem
 * Inputs	: struct BranchPath 	  *curNode
 *		  enum Type_Sort	  *tempInventory
 *		  struct MoveDescription useDescription
 *		  int 			  *tempOutputsFulfilled
 *		  int 			  numOutputsFulfilled
 *		  int 			  tossedIndex
 *		  enum Type_Sort	  output
 *		  int 			  tempFrames
 *		  int 			  viableItems
 *
 * For the given recipe, try to toss items in the inventory in order
 * to make room for the recipe output.
 -------------------------------------------------------------------*/
void tryTossInventoryItem(struct BranchPath *curNode, enum Type_Sort *tempInventory, struct MoveDescription useDescription, int *tempOutputsFulfilled, int numOutputsFulfilled, enum Type_Sort output, int tempFrames, int viableItems) {
	for (int tossedIndex = 0; tossedIndex < 10; tossedIndex++) {
		// Make a copy of the tempInventory with the replaced item
		enum Type_Sort *replacedInventory = copyInventory(tempInventory);
		enum Type_Sort tossedItem = tempInventory[tossedIndex];
		
		// All items before the selected removal item get moved down 1 position
		replacedInventory[tossedIndex] = -1;
		shiftUpToFillNull(replacedInventory);
		
		// The vacancy at the start of the inventory is now occupied with the new item
		replacedInventory[0] = output;
		
		if (stateOK(replacedInventory, tempOutputsFulfilled, recipeList) == 0) {
			free(replacedInventory);
			replacedInventory = NULL;
			return;
		}
		
		// Calculate the additional tossed frames.
		int replacedFrames = tempFrames + invFrames[viableItems][tossedIndex+1];
		useDescription.framesTaken += invFrames[viableItems][tossedIndex+1];
		useDescription.totalFramesTaken += invFrames[viableItems][tossedIndex+1];
		
		finalizeLegalMove(curNode, replacedFrames, useDescription, replacedInventory, tempOutputsFulfilled, numOutputsFulfilled, TossOther, tossedItem, tossedIndex);
		// Inventory is copied within finalizeLegalMove, so free replacedInventory
		free(replacedInventory);
	}
	
	return;
}

/*-------------------------------------------------------------------
 * Function 	: alpha_sort
 * Inputs	: const void	*elem1
 *		  const void	*elem2
 * Outputs	: int		comparisonValue
 *
 * Evaluate an alphabetical ascending sort. This is a comparator function
 * called by stdlib's qsort.
 -------------------------------------------------------------------*/
int alpha_sort(const void *elem1, const void *elem2) {
	enum Type_Sort item1 = *((enum Type_Sort*)elem1);
	enum Type_Sort item2 = *((enum Type_Sort*)elem2);
	
	// Get corresponding alpha_key for the items
	enum Alpha_Sort item1_akey = getAlphaKey(item1);
	enum Alpha_Sort item2_akey = getAlphaKey(item2);
	
	// Handle case of null slots
	if (item1_akey == -1) {return -1;}
	if (item2_akey == -1) {return 1;}
	
	return item1_akey - item2_akey;
}

/*-------------------------------------------------------------------
 * Function 	: alpha_sort_reverse
 * Inputs	: const void	*elem1
 *		  const void	*elem2
 * Outputs	: int		comparisonValue
 *
 * Evaluate an alphabetical descending sort. This is a comparator function
 * called by stdlib's qsort.
 -------------------------------------------------------------------*/
int alpha_sort_reverse(const void *elem1, const void *elem2) {
	enum Type_Sort item1 = *((enum Type_Sort*)elem1);
	enum Type_Sort item2 = *((enum Type_Sort*)elem2);
	
	// Get corresponding alpha_key for the items
	enum Alpha_Sort item1_akey = getAlphaKey(item1);
	enum Alpha_Sort item2_akey = getAlphaKey(item2);
	
	// Handle case of null slots
	if (item1_akey == -1) {return -1;}
	if (item2_akey == -1) {return 1;}
	
	return item2_akey - item1_akey;
}

/*-------------------------------------------------------------------
 * Function 	: type_sort
 * Inputs	: const void	*elem1
 *		  const void	*elem2
 * Outputs	: int		comparisonValue
 *
 * Evaluate an type ascending sort. This is a comparator function
 * called by stdlib's qsort.
 -------------------------------------------------------------------*/
int type_sort(const void *elem1, const void *elem2) {
	enum Type_Sort item1 = *((enum Type_Sort*)elem1);
	enum Type_Sort item2 = *((enum Type_Sort*)elem2);
	
	// Handle case of null slots
	if (item1 == -1) {return -1;}
	if (item2 == -1) {return 1;}
	
	return item1 - item2;
}

/*-------------------------------------------------------------------
 * Function 	: type_sort_reverse
 * Inputs	: const void	*elem1
 *		  const void	*elem2
 * Outputs	: int		comparisonValue
 *
 * Evaluate an type descending sort. This is a comparator function
 * called by stdlib's qsort.
 -------------------------------------------------------------------*/
int type_sort_reverse(const void *elem1, const void *elem2) {
	enum Type_Sort item1 = *((enum Type_Sort*)elem1);
	enum Type_Sort item2 = *((enum Type_Sort*)elem2);
	
	// Handle case of null slots
	if (item1 == -1) {return -1;}
	if (item2 == -1) {return 1;}
	
	return item2 - item1;
}

/*-------------------------------------------------------------------
 * Function 	: getSortedInventory
 * Inputs	: enum Type_Sort *inventory
 *		  enum Action 	  sort
 * Outputs	: enum Type_Sort *sorted_inventory
 *
 * Given the type of sort, use qsort to create a new sorted inventory.
 -------------------------------------------------------------------*/
enum Type_Sort *getSortedInventory(enum Type_Sort *inventory, enum Action sort) {
	// We first need to copy the inventory to a new array
	enum Type_Sort *sorted_inventory = copyInventory(inventory);
	
	// Move nulls to the end of the inventory
	while (countNullsInInventory(sorted_inventory, 0, 10) > 0) {
		shiftDownToFillNull(sorted_inventory);
	}
	
	// Count BLOCKED indices at end of inventory
	int blocked = countNullsInInventory(sorted_inventory, 10, 20);
	
	// Only sort the portion of the inventory which contains valid items

	// Use qsort and execute sort function depending on sort type
	switch(sort) {
		case Sort_Alpha_Asc :
			qsort(sorted_inventory, 20 - blocked, sizeof(enum Type_Sort), alpha_sort);
			return sorted_inventory;
		case Sort_Alpha_Des :
			qsort(sorted_inventory, 20 - blocked, sizeof(enum Type_Sort), alpha_sort_reverse);
			return sorted_inventory;
		case Sort_Type_Asc :
			qsort(sorted_inventory, 20 - blocked, sizeof(enum Type_Sort), type_sort);
			return sorted_inventory;
		case Sort_Type_Des :
			qsort(sorted_inventory, 20 - blocked, sizeof(enum Type_Sort), type_sort_reverse);
			return sorted_inventory;
		default :
			return NULL;
	}
}

/*void userDebugSession(struct Job job) {
	struct BranchPath *root = initializeRoot(job);
	struct BranchPath *curNode = root;
	while (1) {
		int action = -1;
		printf("Choose an action:\n0 - Cook\n1 - Sort Alpha Asc\n2 - Sort Alpha Des\n3 - Sort Type Asc\n4 - Sort Type Des\n5 - Chapter 5\n");
		scanf("%d", &action);
		while (action < 0 || action > 5) {
			printf("Invalid action. Choose a number between 0-5: ");
			scanf("%d", &action);
		}
		
		int recipeIndex = -1;
		enum Type_Sort *newInventory;
		struct BranchPath *newNode;
		int *newOutputsFulfilled;
		int DB_place_index;
		int CO_place_index;
		int lateSort;
		int KM_place_index;
		int CS_place_index;
		int TR_use_index;
		int sort;
		int remainingCanBeFulfilled;
		enum Action sortType;
		switch (action) {
			case 0 :
				// Cook
				printf("Choose the recipe index you are cooking: ");
				scanf("%d", &recipeIndex);
				while (recipeIndex <  0 || recipeIndex > NUM_RECIPES) {
					printf("Invalid recipe index. Choose a number between 0 and 57: ");
					scanf("%d", &recipeIndex);
				}
				
				struct Recipe recipe = recipeList[recipeIndex];
				printf("Choose one of the following combos:\n");
				for (int i = 0; i < recipe.countCombos; i++) {
					if (recipe.combos[i].numItems == 1) {
						printf("%d -> Use %s to make %s\n", i, getItemName(recipe.combos[i].item1), getItemName(recipe.output));
					}
					else {
						printf("%d -> Use %s and %s to make %s\n", i, getItemName(recipe.combos[i].item1), getItemName(recipe.combos[i].item2), getItemName(recipe.output));
					}
				}
				
				// Have user choose one of the recipe combos
				int comboIndex = -1;
				scanf("%d", &comboIndex);
				while (comboIndex < 0 || comboIndex > recipe.countCombos) {
					printf("Invalid combo index. Choose a number between 0 and %d: ", recipe.countCombos - 1);
					scanf("%d", &comboIndex);
				}
				
				struct ItemCombination combo = recipe.combos[comboIndex];
				
				// Modify the inventory to reflect changes to the inventory
				newInventory = copyInventory(curNode->inventory);
				
				// If either item is in the first 10 slots, set it to null
				int indexItem1 = indexOfItemInInventory(newInventory, combo.item1);
				int indexItem2 = indexOfItemInInventory(newInventory, combo.item2);
				if (indexItem1 < 10) {
					newInventory[indexItem1] = -1;
				}
				if (combo.numItems == 2) {
					if (indexItem2 < 10) {
						newInventory[indexItem2] = -1;
					}
				}
				
				enum HandleOutput handleOutput;
				if (countNullsInInventory(newInventory, 0, 10) > 0) {
					printf("Output will be autoplaced.");
					handleOutput = Autoplace;
				}
				else {
					// Have the user specify what to do with the output
					printf("How should we handle the output?\n");
					printf("0 -> Toss the output\n");
					printf("1 -> Toss a different item\n");
					int handleOutputInt = -1;
					scanf("%d", &handleOutputInt);
					while (handleOutputInt < 0 || handleOutputInt > 1) {
						printf("Invalid handle value.");
						scanf("%d", &handleOutputInt);
					}
					if (handleOutputInt == 0) {
						handleOutput = Toss;
					}
					else {
						handleOutput = TossOther;
					}
				}
				
				enum Type_Sort tossedItem = enum Type_Sort -1;
				int tossIndex = -1;
				
				// If tossing output, no modifications to inventory are necessary
				if (handleOutput == Toss) {
					;
				}
				else if (handleOutput == Autoplace) {
					// Shift inventory to fill null
					shiftUpToFillNull(newInventory);
					// Occupancy in first index is where new output is placed
					newInventory[0] = recipe.output;
				}
				else {
					// Toss other
					// Specify items and their indices to be tossed
					printf("What index would you like to toss?\n");
					for (int i = 0; i < 10; i++) {
						printf("%d -> %s\n", i, getItemName(newInventory[i]));
					}
					scanf("%d", &tossIndex);
					while (tossIndex < 0 || tossIndex >= 10) {
						printf("Invalid toss index.");
						scanf("%d", &tossIndex);
					}
					
					tossedItem = newInventory[tossIndex];
					
					// Set the tossed item index to null
					newInventory[tossIndex] = -1;
					shiftUpToFillNull(newInventory);
					newInventory[0] = recipe.output;
				}
				
				// Set the outputFulfilled index of the current recipe to 1
				newOutputsFulfilled = copyOutputsFulfilled(curNode->outputCreated);
				newOutputsFulfilled[recipeIndex] = 1;
				
				// Now for the moment of truth...
				// Can all the other recipes be fulfilled?
				remainingCanBeFulfilled = stateOK(newInventory, newOutputsFulfilled, recipeList);
				if (remainingCanBeFulfilled == 0) {
					printf("stateOK determined that this move is not possible.\n");
					exit(1);
				}
				
				printf("This has been determined to be a legal move. Moving to the next node...\n");
				
				newNode = malloc(sizeof(struct BranchPath));
				newNode->prev = curNode;
				curNode->next = newNode;
				newNode->moves = curNode->moves + 1;
				newNode->inventory = newInventory;
				newNode->description.action = Cook;
				
				struct Cook *cook = malloc(sizeof(struct Cook));
				cook->numItems = combo.numItems;
				cook->item1 = combo.item1;
				cook->itemIndex1 = indexItem1;
				if (combo.numItems == 2) {
					cook->item2 = combo.item2;
					cook->itemIndex2 = indexItem2;
				}
				else {
					cook->item2 = -1;
					cook->itemIndex2 = -1;
				}
				cook->output = recipe.output;
				cook->handleOutput = handleOutput;
				if (cook->handleOutput == Toss || cook->handleOutput == Autoplace) {
					cook->toss = -1;
				}
				else {
					cook->toss = tossedItem;
					cook->indexToss = tossIndex;
				}
				newNode->description.data = (void *)cook;
				newNode->outputCreated = newOutputsFulfilled;
				newNode->numOutputsCreated = curNode->numOutputsCreated + 1;
				curNode = newNode;
				break;
			case 1 : // Sort Alpha Asc
				newInventory = getSortedInventory(curNode->inventory, Sort_Alpha_Asc);
				newNode = malloc(sizeof(struct BranchPath));
				curNode->next = newNode;
				newNode->prev = curNode;
				newNode->moves = curNode->moves + 1;
				newNode->inventory = newInventory;
				newNode->description.action = Sort_Alpha_Asc;
				newNode->description.data = NULL;
				
				int *newOutputsFulfilled = copyOutputsFulfilled(curNode->outputCreated);
				newNode->outputCreated = newOutputsFulfilled;
				newNode->numOutputsCreated = curNode->numOutputsCreated;
				curNode = newNode;
				break;
			case 2 : // Sort Alpha Des
				newInventory = getSortedInventory(curNode->inventory, Sort_Alpha_Des);
				newNode = malloc(sizeof(struct BranchPath));
				curNode->next = newNode;
				newNode->prev = curNode;
				newNode->moves = curNode->moves + 1;
				newNode->inventory = newInventory;
				newNode->description.action = Sort_Alpha_Des;
				newNode->description.data = NULL;
				
				newOutputsFulfilled = copyOutputsFulfilled(curNode->outputCreated);
				newNode->outputCreated = newOutputsFulfilled;
				newNode->numOutputsCreated = curNode->numOutputsCreated;
				curNode = newNode;
				break;
			case 3 : // Sort Type Asc
				newInventory = getSortedInventory(curNode->inventory, Sort_Type_Asc);
				newNode = malloc(sizeof(struct BranchPath));
				curNode->next = newNode;
				newNode->prev = curNode;
				newNode->moves = curNode->moves + 1;
				newNode->inventory = newInventory;
				newNode->description.action = Sort_Type_Asc;
				newNode->description.data = NULL;
				
				newOutputsFulfilled = copyOutputsFulfilled(curNode->outputCreated);
				newNode->outputCreated = newOutputsFulfilled;
				newNode->numOutputsCreated = curNode->numOutputsCreated;
				curNode = newNode;
				break;
			case 4: // Sort Type Des
				newInventory = getSortedInventory(curNode->inventory, Sort_Type_Des);
				newNode = malloc(sizeof(struct BranchPath));
				curNode->next = newNode;
				newNode->prev = curNode;
				newNode->moves = curNode->moves + 1;
				newNode->inventory = newInventory;
				newNode->description.action = Sort_Type_Des;
				newNode->description.data = NULL;
				
				newOutputsFulfilled = copyOutputsFulfilled(curNode->outputCreated);
				newNode->outputCreated = newOutputsFulfilled;
				newNode->numOutputsCreated = curNode->numOutputsCreated;
				curNode = newNode;
				break;
			case 5 : // CH5
				newOutputsFulfilled = copyOutputsFulfilled(curNode->outputCreated);
				newOutputsFulfilled[getIndexOfRecipe(Dried_Bouquet)] = 1;
			
				// If Hot Dog is in first half of inventory, inform user and quit
				if (indexOfItemInInventory(curNode->inventory, getItem(Hot_Dog)) < 10) {
					printf("We cannot complete Chapter 5 now, as Hot Dog is in the first half of the inventory!");
					exit(1);
				}
				
				// Dried Bouquet session
				newInventory = copyInventory(curNode->inventory);
				int mousse_Cake_Index = indexOfItemInInventory(newInventory, Mousse_Cake);
				if (mousse_Cake_Index <  10) {
					newInventory[mousse_Cake_Index] = -1;
				}
				
				// Handle allocation of Dried Bouquet
				
				// If there is a null in the inventory, place the Dried Bouquet automatically
				if (countNullsInInventory(newInventory, 0, 10) > 0) {
					shiftUpToFillNull(newInventory);
					newInventory[0] = Dried_Bouquet;
					DB_place_index = 0;
					printf("Dried Bouquet was auto-placed in slot 1\n");
				}
				else {
					// Toss an item
					printf("What index would you like to toss for the Dried Bouquet?\n");
					for (int i = 0; i < 10; i++) {
						printf("%d -> %s\n", i, getItemName(newInventory[i]));
					}
					scanf("%d", &DB_place_index);
					while (DB_place_index < 0 || DB_place_index >= 10) {
						printf("Invalid toss index.");
						scanf("%d", &DB_place_index);
					}
					
					newInventory[DB_place_index] = -1;
					shiftUpToFillNull(newInventory);
					newInventory[0] = Dried_Bouquet;
				}
				
				// Now grab the Coconut
				
				// If there is a null in the inventory, place the Coconut automatically
				if (countNullsInInventory(newInventory, 0, 10) > 0) {
					shiftUpToFillNull(newInventory);
					newInventory[0] = Coconut;
					CO_place_index = 0;
					printf("Coconut was auto-placed in slot 1\n");
				}
				else {
					// Toss an item
					printf("What index would you like to toss for the Coconut?\n");
					for (int i = 0; i < 10; i++) {
						printf("%d -> %s\n", i, getItemName(newInventory[i]));
					}
					scanf("%d", &CO_place_index);
					while (CO_place_index < 0 || CO_place_index >= 10) {
						printf("Invalid toss index.");
						scanf("%d", &CO_place_index);
					}
					
					newInventory[CO_place_index] = -1;
					shiftUpToFillNull(newInventory);
					newInventory[0] = Coconut;
				}
				
				// Ask the user to either do an early sort or late sort
				printf("Early sort or late sort?\n0 - Early Sort\n1 - Late Sort\n");
				scanf("%d", &lateSort);
				while (lateSort < 0 || lateSort > 1) {
					printf("Invalid option.");
					scanf("%d", &lateSort);
				}
				
				// If early sort, sort the inventory
				if (lateSort == 0) {
					printf("What type of sort?\n0 - Alpha Ascending\n1 - Alpha Descending\n2 - Type Ascending\n3 - Type Descending");
					scanf("%d", &sort);
					while (sort < 0 || sort > 3) {
						printf("Invalid option.");
						scanf("%d", &sort);
					}
					
					switch (sort) {
						case 0 :
							sortType = Sort_Alpha_Asc;
							break;
						case 1 :
							sortType = Sort_Alpha_Des;
							break;
						case 2 :
							sortType = Sort_Type_Asc;
							break;
						case 3 : sortType = Sort_Type_Des;
							break;
					}
					
					newInventory = getSortedInventory(newInventory, sortType);
					
					// Ask where to place the Keel Mango
					if (countNullsInInventory(newInventory, 0, 10) > 0) {
						shiftUpToFillNull(newInventory);
						newInventory[0] = Keel_Mango;
						KM_place_index = 0;
						printf("Keel Mango was auto-placed in slot 1\n");
					}
					else {
						// Ask what item to toss
						printf("What index would you like to toss for the Keel Mango?\n");
						for (int i = 0; i < 10; i++) {
							printf("%d -> %s\n", i, getItemName(newInventory[i]));
						}
						scanf("%d", &KM_place_index);
						while (KM_place_index < 0 || KM_place_index >= 10) {
							printf("Invalid toss index.");
							scanf("%d", &KM_place_index);
						}
						
						newInventory[KM_place_index] = -1;
						shiftUpToFillNull(newInventory);
						newInventory[0] = Keel_Mango;
					}
					
					// Ask where to place the Courage Shell
					if (countNullsInInventory(newInventory, 0, 10) > 0) {
						shiftUpToFillNull(newInventory);
						newInventory[0] = Courage_Shell;
						CS_place_index = 0;
						printf("Courage Shell was auto-placed in slot 1\n");
					}
					else {
						printf("What index would you like to toss for the Courage Shell?\n");
						for (int i = 0; i < 10; i++) {
							printf("%d -> %s\n", i, getItemName(newInventory[i]));
						}
						scanf("%d", &CS_place_index);
						while (CS_place_index < 0 || CS_place_index >= 10) {
							printf("Invalid toss index.");
							scanf("%d", &CS_place_index);
						}
						
						newInventory[CS_place_index] = -1;
						shiftUpToFillNull(newInventory);
						newInventory[0] = Courage_Shell;
					}
				}
				else {
					// Ask where to place the Keel Mango
					if (countNullsInInventory(newInventory, 0, 10) > 0) {
						shiftUpToFillNull(newInventory);
						newInventory[0] = Keel_Mango;
						KM_place_index = 0;
						printf("Keel Mango was auto-placed in slot 1\n");
					}
					else {
						// Ask what item to toss
						printf("What index would you like to toss for the Keel Mango?\n");
						for (int i = 0; i < 10; i++) {
							printf("%d -> %s\n", i, getItemName(newInventory[i]));
						}
						scanf("%d", &KM_place_index);
						while (KM_place_index < 0 || KM_place_index >= 10) {
							printf("Invalid toss index.");
							scanf("%d", &KM_place_index);
						}
						
						newInventory[KM_place_index] = -1;
						shiftUpToFillNull(newInventory);
						newInventory[0] = Keel_Mango;
					}
					
					printf("What type of sort?\n0 - Alpha Ascending\n1 - Alpha Descending\n2 - Type Ascending\n3 - Type Descending");
					scanf("%d", &sort);
					while (sort < 0 || sort > 3) {
						printf("Invalid option.");
						scanf("%d", &sort);
					}
					
					switch (sort) {
						case 0 :
							sortType = Sort_Alpha_Asc;
							break;
						case 1 :
							sortType = Sort_Alpha_Des;
							break;
						case 2 :
							sortType = Sort_Type_Asc;
							break;
						case 3 : sortType = Sort_Type_Des;
							break;
					}
					
					newInventory = getSortedInventory(newInventory, sortType);
					
					// Ask where to place the Courage Shell
					if (countNullsInInventory(newInventory, 0, 10) > 0) {
						shiftUpToFillNull(newInventory);
						newInventory[0] = Courage_Shell;
						CS_place_index = 0;
						printf("Courage Shell was auto-placed in slot 1\n");
					}
					else {
						printf("What index would you like to toss for the Courage Shell?\n");
						for (int i = 0; i < 10; i++) {
							printf("%d -> %s\n", i, getItemName(newInventory[i]));
						}
						scanf("%d", &CS_place_index);
						while (CS_place_index < 0 || CS_place_index >= 10) {
							printf("Invalid toss index.");
							scanf("%d", &CS_place_index);
						}
						
						newInventory[CS_place_index] = -1;
						shiftUpToFillNull(newInventory);
						newInventory[0] = Courage_Shell;
					}
				}
				
				// Lastly, determine if the Thunder Rage leaves the inventory
				TR_use_index = indexOfItemInInventory(newInventory, Thunder_Rage);
				if (TR_use_index < 10) {
					newInventory[TR_use_index] = -1;
				}
				
				// Now, determine if this state is okay
				remainingCanBeFulfilled = stateOK(newInventory, newOutputsFulfilled, recipeList);
				if (remainingCanBeFulfilled == 0) {
					printf("stateOK determined that this move is not possible.\n");
					exit(1);
				}
				
				printf("This has been determined to be a legal move. Moving to the next node...\n");
				
				newNode = malloc(sizeof(struct BranchPath));
				curNode->next = newNode;
				newNode->prev = curNode;
				newNode->moves = curNode->moves + 1;
				newNode->inventory = newInventory;
				newNode->description.action = Ch5;
				newNode->description.data = (void *) createChapter5Struct(DB_place_index, CO_place_index, KM_place_index, CS_place_index, TR_use_index, sortType, lateSort);
				newNode->outputCreated = newOutputsFulfilled;
				newNode->numOutputsCreated = curNode->numOutputsCreated + 1;
				curNode = newNode;
				break;
			default :
				break;
		}
		
		// Print inventory
		printf("We now have the following inventory:\n");
		for (int i = 0; i < 20; i++) {
			printf("%s\n", getItemName(curNode->inventory[i]));
		}
		
		if (curNode->numOutputsCreated == NUM_RECIPES) {
			printf("Finished roadmap!");
			printResults("userDebug.txt", root);
			exit(1);
		}
	}
}*/

/*-------------------------------------------------------------------
 * Function 	: calculateOrder
 * Inputs	: struct Job 		job
 * Outputs	: struct Result	result
 *
 * This is the main roadmap evaluation function. This calls various
 * child functions to generate a sequence of legal moves. It then
 * uses parameters to determine which legal move to traverse to. Once
 * a roadmap is found, the data is printed to a .txt file, and the result
 * is passed back to start.c to try submitting to the server.
 -------------------------------------------------------------------*/
struct Result calculateOrder(struct Job job) {
	config_t *config = getConfig();
	
	int randomise;
	config_lookup_int(config, "randomise", &randomise);
	
	int select;
	config_lookup_int(config, "select", &select);
	recipeList = getRecipeList();
	
	int debug;
	config_lookup_int(config, "Debug", &debug);
	
	/*if (debug) {
		userDebugSession(job);
	}*/
	
	/*
	#===============================================================================
	# GOAL
	#===============================================================================
	# The goal of this program is to find a roadmap of fulfilling all 57 Zess T.
	# recipes in as few frames as possible.
	#===============================================================================
	# RULES & ASSUMPTIONS
	#===============================================================================
	# A known glitch has been exploited that allows the ability to duplicate ingredient items
	# If you use an ingredient in slots 1-10, that slot becomes "NULL" and isn't duplicated
	# If you use an ingredient in slots 11-20, the item is duplicated
	# If there's a NULL slot, the recipe output will automatically fill the 1st NULL position
	# If there's no NULL slots, the player is given the option of tossing either the output item,
	#	or replacing any inventory in the first 10 slots item with the output item
	#	Attempting to replace an inventory item in slots 11-20 will leave the inventory unchanged
	#		(In this instance, it's always faster to just toss the output item outright)
	# If there's a sort, all NULL slots are wiped away and are no longer available, becoming "BLOCKED" at the end of the inventory
	# "BLOCKED" spaces are assumed to be permanently unavailable for the remainder of recipe fulfillment
	# When navigating the inventory, it is assumed all "NULL" and "BLOCKED" spaces are hidden from the player
	#	For example, if 2 spaces are NULL, the player will only see the other 18 items to navigate through in the inventory
	# The first recipe that is fulfilled can only use a single ingredient


	# At some point, the player needs to trade 2 Hot Dogs and a Mousse Cake for a Dried Bouquet (Which is only needed for the Space Food recipe)
	# At another point, the player will need to collect the Keel Mango, Coconut, and Courage Shell after Chapter 5
	# So there's 2 sessions of recipe-fulfillment that will be needed (Pre-Ch.5 and Post-Ch.5)
	#===============================================================================
	# Algorithm Description
	#===============================================================================
	# Start Loop
	# Iterate through all Items that haven't been made already
	# Iterate through all their recipes that can be made with the current materials
	# Iterate through all placement options of the output ingredient (Tossing, Autofill, etc)
	# Determine how many relevant frames are required to fulfill this recipe and add it to a list of "legal moves" for the current state
	# Pick the Legal Move that takes the least frames, update the inventory based on that move, and recurse the loop again
	# If no legal moves are available and the end-state has not been reached,
	#	then return back up a layer and pick the next-fastest legal move at that state to recurse down
	# Repeat recursion until search space has been exhausted
	#===============================================================================
	*/
	
	
	// TODO: Add functionality to keep evaluating near the current record
	// I reset iteration_count, but I need to not return until we reach the iteration limit
	// Cache the record after writing to a file and return the record frame_count
	// How many times a worker is evaluating a new random branch
	int total_dives = 0;
	
	// Deepest node at any particular point
	struct BranchPath *curNode = NULL;
	struct BranchPath *root;
	
	struct Result result_cache = (struct Result) {-1, -1};
	
	//Start main loop
	while (1) {
		int stepIndex = 0;
		int iterationCount = 0;

		// Create root of tree path
		curNode = initializeRoot(job);
		root = curNode; // Necessary when printing results starting from root
		
		total_dives++;
		char temp1[30];
		char temp2[30];
		sprintf(temp1, "Call %d", job.callNumber);
		sprintf(temp2, "Searching New Branch %d", total_dives);
		recipeLog(3, "Calculator", "Info", temp1, temp2);
		
		// Handle the case where the user may choose to disable both randomise and select,
		// in which case they would always iterate down the same path, even if we reset every n iterations
		// Set to 1,000,000 iterations before resetting at the root
		int configBool = (iterationCount < 100000 || (select == 0 && randomise == 0));
	
		// Start iteration loop
		while (configBool) {
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
					#pragma omp critical
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
							result_cache = (struct Result) {optimizeResult.last->description.totalFramesTaken, job.callNumber};
							
							// Reset the iteration count so we continue to explore near this record
							iterationCount = 0;
						}
					}
					
					freeAllNodes(optimizeResult.last);
				}
				
				// Regardless of record status, it's time to go back up and find new endstates
				curNode = curNode->prev;
				freeLegalMove(curNode, 0);
				curNode->next = NULL;
				stepIndex--;
				continue;
			}
			// End condition not met. Check if this current level has something in the event queue
			else if (curNode->legalMoves == NULL) {
				// This node has not yet been assigned an array of legal moves.
				
				// Generate the list of all possible decisions
				
				// Only evaluate the 57th recipe (Mistake) when it's the last recipe to fulfill
				// This is because it is relatively easy to craft this output with many of the previous outputs, and will take minimal frames
				int upperOutputLimit = (curNode->numOutputsCreated == NUM_RECIPES - 1) ? NUM_RECIPES : (NUM_RECIPES - 1);
				
				// Iterate through all recipe ingredient combos
				for (int recipeIndex = 0; recipeIndex < upperOutputLimit; recipeIndex++) {
					// Only want recipes that haven't been fulfilled
					if (curNode->outputCreated[recipeIndex] == 1) {
						continue;
					}
					
					// Dried Bouquet (Recipe index 56) represents the Chapter 5 intermission
					// Don't actually use the specified recipe, as it is handled later
					if (recipeIndex == getIndexOfRecipe(Dried_Bouquet)) {
						continue;
					}
					
					fulfillRecipes(curNode, recipeIndex);
				}
				
				// Special handling of the 56th recipe, which is representative of the Chapter 5 intermission
				
				// The first item is trading the Mousse Cake and 2 Hot Dogs for a Dried Bouquet
				// Inventory must contain both items, and Hot Dog must be in a slot such that it can be duplicated
				if (curNode->outputCreated[getIndexOfRecipe(Dried_Bouquet)] == 0 && indexOfItemInInventory(curNode->inventory, Mousse_Cake) > -1 &&
				    indexOfItemInInventory(curNode->inventory, Hot_Dog) >= 10) {
					fulfillChapter5(curNode);
				}
				
				// Special handling of inventory sorting
				// Avoid redundant searches
				if (curNode->description.action == Begin || curNode->description.action == Cook || curNode->description.action == Ch5) {
					handleSorts(curNode);
				}
				
				// All legal moves evaluated and listed!
				
				// Protect race condition of the local record
				#pragma omp critical
				{
					// Filter out all legal moves that would exceed the current frame limit
					filterLegalMovesExceedFrameLimit(curNode, getLocalRecord() + BUFFER_SEARCH_FRAMES);
				}
				
				if (curNode->moves == 0) {
					// Filter out all legal moves that use 2 ingredients in the very first legal move
					filterOut2Ingredients(curNode);
				}
				
				// Special filtering if we only had one recipe left to fulfill
				if (curNode->numOutputsCreated == NUM_RECIPES-1 && curNode->numLegalMoves > 0 && curNode->legalMoves[0]->description.action == Cook) {
					// If there are any legal moves that satisfy this final recipe,
					// strip out everything besides the fastest legal move
					// This saves on recursing down pointless states
					popAllButFirstLegalMove(curNode);
				}
				else {
					handleSelectAndRandom(curNode, select, randomise);
				}
				
				if (curNode->numLegalMoves == 0) {
					// There are no legal moves to iterate on
					// Go back up!
					
					// Handle the case where the root node runs out of legal moves
					if (curNode->prev == NULL) {
						assert(curNode->moves == 0);
						freeNode(curNode);
						return (struct Result) {-1, -1};
					}
					
					curNode = curNode->prev;
					freeLegalMove(curNode, 0);
					curNode->next = NULL;
					stepIndex--;
					continue;
				}
					
				// Once the list is generated choose the top-most path and iterate downward
				curNode->next = curNode->legalMoves[0];
				curNode = curNode->next;
				stepIndex++;
				
			}
			else {
				// Protect race condition of the local record
				#pragma omp critical
				{
					// Filter out all legal moves that would exceed the current frame limit
					filterLegalMovesExceedFrameLimit(curNode, getLocalRecord() + BUFFER_SEARCH_FRAMES);
				}
				
				if (curNode->numLegalMoves == 0) {
					// No legal moves are left to evaluate, go back up...
					// Wipe away the current node
					
					// Handle the case where the root node runs out of legal moves
					if (curNode->prev == NULL) {
						assert(curNode->moves == 0);
						freeNode(curNode);
						return (struct Result) {-1, -1};
					}
					
					curNode = curNode->prev;
					freeLegalMove(curNode, 0);
					curNode->next = NULL;
					stepIndex--;
					continue;
				}
				
				handleSelectAndRandom(curNode, select, randomise);
				
				// Once the list is generated, choose the top-most (quickest) path and iterate downward
				curNode->next = curNode->legalMoves[0];
				curNode = curNode->legalMoves[0];
				stepIndex++;
				
				// Logging for progress display
				iterationCount++;
				configBool = (iterationCount < 100000 || (select == 0 && randomise == 0));
				if (iterationCount % 10000 == 0) {
					char temp3[30];
					char temp4[100];
					sprintf(temp3, "Call %d", job.callNumber);
					sprintf(temp4, "%d steps currently taken, %d frames acculumated so far; %dk iterations", stepIndex, curNode->description.totalFramesTaken, iterationCount/1000);
					recipeLog(6, "Calculator", "Info", temp3, temp4);
					
					// Double check to see if the local record has been improved
					job.current_frame_record = getLocalRecord();
				}
			}
		}
		
		// We have passed the iteration maximum
		// Free everything before reinitializing
		freeAllNodes(curNode);
		
		// Check the cache to see if a result was generated
		if (result_cache.frames > -1) {
			// Return the cached result
			return result_cache;
		}
		
		
		// For profiling
		if (total_dives == 100) {
			exit(1);
		}
		
	}
}
