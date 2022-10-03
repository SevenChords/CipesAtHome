#include <stdio.h>
#include <stdlib.h>
#include "calculator.h"
#include "base.h"
#include "FTPManagement.h"
#include "node_print.h"
#include "recipes.h"
#include "start.h"
#include "shutdown.h"
#include <libconfig.h>
#include "logger.h"
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <omp.h>
#include "absl/base/port.h"

#define NUM_RECIPES 58 			// Including Chapter 5 representation
#define CHOOSE_2ND_INGREDIENT_FRAMES 56 	// Penalty for choosing a 2nd item
#define TOSS_FRAMES 32				// Penalty for having to toss an item
#define ALPHA_SORT_FRAMES 38			// Penalty to perform alphabetical ascending sort
#define REVERSE_ALPHA_SORT_FRAMES 40		// Penalty to perform alphabetical descending sort
#define TYPE_SORT_FRAMES 39			// Penalty to perform type ascending sort
#define REVERSE_TYPE_SORT_FRAMES 41		// Penalty to perform type descending sort
#define JUMP_STORAGE_NO_TOSS_FRAMES 5		// Penalty for not tossing the last item (because we need to get Jump Storage)
#define BUFFER_SEARCH_FRAMES 150		// Threshold to try optimizing a roadmap to attempt to beat the current record
#define DEFAULT_ITERATION_LIMIT 100000 // Cutoff for iterations explored before resetting
#define ITERATION_LIMIT_INCREASE 100000000 // Amount to increase the iteration limit by when finding a new record
#define INVENTORY_SIZE 20

#define CHECK_SHUTDOWN_INTERVAL 30000
static const int UNSET_INDEX_SIGNED = -99999;

static const int INT_OUTPUT_ARRAY_SIZE_BYTES = sizeof(outputCreatedArray_t);
static const outputCreatedArray_t EMPTY_RECIPES = {0};
static const Cook EMPTY_COOK = {0};

int **invFrames;
Recipe *recipeList;

Serial *visitedBranches = NULL;
int numVisitedBranches = 0;

// Used to uniquely identify a particular item combination for serialization purposes
int recipeOffsetLookup[57] = {
	0, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15, 16, 17, 18, 23, 24, 25,
	26, 28, 31, 35, 37, 39, 40, 41, 44, 45, 46, 48, 50, 52,
	59, 60, 61, 62, 63, 64, 66, 68, 71, 77, 82, 83, 85, 86,
	87, 88, 89, 90, 92, 147, 148, 149, 152, 153, 154, 155};

// Uniquely identify a particular item combination
// Utilize the hard-coded offset lookup to reduce time complexity a bit
uint8_t getRecipeIndex(Cook *pCook)
{
	int i = getIndexOfRecipe(pCook->output);
	int offset = recipeOffsetLookup[i];

	for (int j = 0; j < recipeList[i].countCombos; j++)
	{
		ItemCombination listCombo = recipeList[i].combos[j];

		if (listCombo.numItems != pCook->numItems)
			continue;

		bool bFirstItemMatch = listCombo.item1 == pCook->item1 || (pCook->numItems == 2 && listCombo.item1 == pCook->item2);
		bool bSecondItemMatch = listCombo.item2 == pCook->item2 || (pCook->numItems == 2 && listCombo.item2 == pCook->item1);

		if (bFirstItemMatch && (pCook->numItems == 1 || bSecondItemMatch))
			return offset + j;
	}

	return UINT8_MAX;
}

// returns the number of bytes used to serialize the node
uint8_t serializeCookNode(BranchPath *node, void **data)
{
	uint8_t dataLen = 0;
	Cook *pCook = (Cook*)node->description.data;
	Serial parentSerial = node->prev->serial;

	// Don't really need to hash Mistake, and removing all recipe combos for mistakes saves space for our serialization
	if (pCook->output == Mistake)
	{
		*data = NULL;
		return 0;
	}
	else if (pCook->output == Dried_Bouquet)
	{
		// This shouldn't happen
		exit(1);
	}

	uint8_t recipeIdx = getRecipeIndex(pCook);
	assert(recipeIdx != UINT8_MAX);

	dataLen = 1;

	int8_t outputPlace = pCook->indexToss;
	bool bAutoplace = (outputPlace == -1);

	if (!bAutoplace)
		dataLen = 2;

	if (parentSerial.length == 0)
	{
		// We have nothing to copy. Just malloc 3 bytes
		*data = malloc(dataLen);
		checkMallocFailed(*data);
	}
	else
	{
		*data = malloc(parentSerial.length + dataLen);
		checkMallocFailed(*data);

		memcpy(*data, parentSerial.data, parentSerial.length);
	}

	memset(*data + parentSerial.length, recipeIdx, 1);

	if (!bAutoplace)
		memset(*data + parentSerial.length + 1, (uint8_t) outputPlace, 1);

	return dataLen;
}

uint8_t serializeSortNode(BranchPath *node, void **data)
{
	uint8_t dataLen = 1;
	Serial parentSerial = node->prev->serial;

	if (parentSerial.length == 0)
	{
		*data = malloc(dataLen);
		checkMallocFailed(*data);
	}
	else
	{
		*data = malloc(parentSerial.length + dataLen);
		checkMallocFailed(*data);
		memcpy(*data, parentSerial.data, parentSerial.length);
	}

	memset(*data + parentSerial.length, (node->description.action - 2) + recipeOffsetLookup[56], 1);

	return dataLen;
}

uint8_t serializeCH5Node(BranchPath *node, void **data)
{
	uint8_t actionValue = recipeOffsetLookup[56] + 4;
	Serial parentSerial = node->prev->serial;

	CH5 *pCH5 = (CH5*)node->description.data;
	uint8_t lateSort = (uint8_t) pCH5->lateSort;
	uint8_t sort = (uint8_t) pCH5->ch5Sort - 2;
	uint8_t uCS = (uint8_t) pCH5->indexCourageShell;
	int iDB = pCH5->indexDriedBouquet;
	int iCO = pCH5->indexCoconut;
	int iKM = pCH5->indexKeelMango;

	if (lateSort == 1)
		actionValue += 40;
	
	actionValue += (sort * 10);
	actionValue += uCS;

	uint8_t dataLen = 1;

	// Tack on additional bits for each item that is not autoplaced
	uint8_t optionalByte1 = 0;
	uint8_t optionalByte2 = 0;

	if (iDB > -1)
	{
		dataLen = 2;
		optionalByte1 = ((uint8_t)(iDB)) << 4;
	}
	if (iCO > -1)
	{
		dataLen = 2;
		if (iDB > -1)
			optionalByte1 |= (uint8_t)(iCO);
		else
			optionalByte1 = ((uint8_t)(iCO)) << 4;
	}
	if (iKM > -1)
	{
		dataLen = 2;
		if (iDB > -1 && iCO > -1)
		{
			dataLen = 3;
			optionalByte2 = ((uint8_t)(iKM)) << 4;
		}
		else if (iDB > -1 || iCO > -1)
			optionalByte1 |= (uint8_t)(iKM);
		else
			optionalByte1 = ((uint8_t)(iKM)) << 4;
	}
	
	*data = malloc(parentSerial.length + dataLen);
	checkMallocFailed(*data);
	if (parentSerial.length > 0)	
		memcpy(*data, parentSerial.data, parentSerial.length);

	memset(*data + parentSerial.length, actionValue, 1);

	if (dataLen >= 2)
		memset(*data + parentSerial.length+1, optionalByte1, 1);
	if (dataLen == 3)
		memset(*data + parentSerial.length+2, optionalByte2, 1);

	return dataLen;
}

// To serialize nodes, we need to represent the following actions as a number:
// 0-154: all possible recipe combinations
// 155-158: all four sorts
// 159-238: all possible CH5 actions
// Additionally, we need to tack on extra bits if a recipe output is not auto-placed
// and tack on extra bits if any CH5 items are not auto-placed
void serializeNode(BranchPath *node)
{
	if (node->moves == 0)
	{
		node->serial = (Serial) {0, NULL};
		return;
	}

	void *data = NULL;
	uint8_t dataLen = 0; // in bytes
	Serial parentSerial = node->prev->serial;
	Action nodeAction = node->description.action;

	switch(nodeAction)
	{
	case EBegin:
		
	case ECook:
		dataLen = serializeCookNode(node, &data);
		break;
	case ESort_Alpha_Asc:
	case ESort_Alpha_Des:
	case ESort_Type_Asc:
	case ESort_Type_Des:
		dataLen = serializeSortNode(node, &data);
		break;
	case ECh5:
		dataLen = serializeCH5Node(node, &data);
		break;
	default:
		exit(1);
	}

	node->serial = (Serial) {parentSerial.length + dataLen, data};
}

ABSL_ATTRIBUTE_ALWAYS_INLINE
inline int serialcmp(Serial s1, Serial s2)
{
	int diff = memcmp(s1.data, s2.data, min(s1.length, s2.length));

	// if the prefix is identical, then we are dealing with a parent-child
	// in this case, we want to keep moving left if s2 is larger
	if (diff == 0)
		return s1.length > s2.length ? 1 : -1;

	return diff;
}

// returns the index the serial was inserted at
uint32_t insertSorted(Serial serial)
{
	// First add space
	if (numVisitedBranches == 0)
		visitedBranches = malloc(++numVisitedBranches * sizeof(Serial)); // numVisitedBranches should be 0 here
	else
		visitedBranches = realloc(visitedBranches, ++numVisitedBranches * sizeof(Serial));

	// Last serial data is empty from realloc, so walk backwards from second-to-last
	int i = 0;
	for (i = numVisitedBranches-2; i >= 0 && serialcmp(visitedBranches[i], serial) > 0; i--)
		visitedBranches[i + 1] = visitedBranches[i];
	
	visitedBranches[i + 1] = serial;
	return i + 1;
}

uint32_t indexToInsert(Serial serial, int low, int high)
{
	if (high <= low)
		return (serialcmp(serial, visitedBranches[low]) > 0) ? low + 1 : low;
	
	int mid = (low + high) / 2;

	int cmpMid = serialcmp(serial, visitedBranches[mid]);
	if (cmpMid == 0)
		return mid+1;
	
	if (cmpMid > 0)
		return indexToInsert(serial, mid+1, high);
	return indexToInsert(serial, low, mid-1);
}

void insertIntoCache(Serial serial, int index, int deletedChildren)
{
	// Handle the case where our array is empty
	if (numVisitedBranches == 0)
	{
		visitedBranches = malloc((++numVisitedBranches) * sizeof(Serial)); // numVisitedBranches should be 0 here
		visitedBranches[index] = serial;
		return;
	}

	// Handle the case where we are deleting children
	if (deletedChildren > 0)
	{
		visitedBranches[index] = serial;

		// We're just replacing the child with the parent
		if (deletedChildren == 1)
			return;

		// We will realloc with a smaller space, but we need to perform the memmove first
		memmove(visitedBranches + index + 1, visitedBranches + index + deletedChildren, (numVisitedBranches - index - deletedChildren) * sizeof(Serial));
		numVisitedBranches = numVisitedBranches - deletedChildren + 1;
		visitedBranches = realloc(visitedBranches, numVisitedBranches * sizeof(Serial));
		return;
	}

	// We are increasing the arr size
	visitedBranches = realloc(visitedBranches, (++numVisitedBranches) * sizeof(Serial));
	
	if (index < numVisitedBranches - 1)
		memmove(&visitedBranches[index + 1], &visitedBranches[index], (numVisitedBranches - index - 1) * sizeof(Serial));

	visitedBranches[index] = serial;
}

uint32_t deleteAndFreeChildSerials(Serial serial, uint32_t index)
{
	uint32_t deletedChildren = 0;

	for (uint32_t i = index; i < numVisitedBranches; i++)
	{
		Serial arrSerial = visitedBranches[i];
		// If the next serial is shorter than the current serial, then we know that
		// the next serial is higher up in the tree. Thus, we can terminate the loop
		if (arrSerial.length < serial.length)
			break;
		
		int diff = memcmp(arrSerial.data, serial.data, serial.length);
		
		// Does this serial correspond with some other parent actions than the current serial?
		if (diff != 0)
			break;

		// if diff == 0, then the ith serial is a child of the current serial, so delete ith serial
		if (diff == 0)
		{
			++deletedChildren;

			// Free the child!
			free(arrSerial.data);
			arrSerial.length = 0;
			arrSerial.data = NULL;
		}
	}

	return deletedChildren;
}

void shiftSerialArrayToFillFreedChildren(uint32_t index, uint32_t deletedChildren)
{
	// We want to move bytes:
	// FROM 		index+deletedChildren
	// TO 			index
	// CONTAINING	numVisitedBranches - index - deletedChildren indices
	// i.e. if index = 5, deleted 2 children, and have 10 total serials (before realloc), then we want to move serials 7-9 to indices 5-7

	uint32_t fromIdx = index + deletedChildren;
	Serial *fromPtr = visitedBranches + fromIdx;
	Serial *toPtr 	= visitedBranches + index;
	uint32_t numIndicesToCopy = numVisitedBranches - index - deletedChildren;
	uint32_t numBytesToCopy = numIndicesToCopy * sizeof(Serial);

	if (fromIdx < numVisitedBranches)
		memmove(toPtr, fromPtr, numBytesToCopy);

	// Now that we've shifted the array around, realloc to the new numVisitedBranches
	numVisitedBranches -= deletedChildren;
	visitedBranches = realloc(visitedBranches, numVisitedBranches * sizeof(Serial));
}

void cacheSerial(BranchPath *node)
{
	// ignore root node and Mistake
	if (node->serial.length == 0 || node->serial.data == NULL)
		return;

	#pragma omp critical
	{
		if (node->serial.data == NULL)
			exit(2);

		void *cachedData = malloc(node->serial.length);
		memcpy(cachedData, node->serial.data, node->serial.length);

		Serial cachedSerial = (Serial) {node->serial.length, cachedData};

		uint32_t index = 0;

		if (numVisitedBranches > 0)
			index = indexToInsert(cachedSerial, 0, numVisitedBranches-1);

		// Free and note how many children we are freeing
		int childrenFreed = deleteAndFreeChildSerials(cachedSerial, index);
		insertIntoCache(cachedSerial, index, childrenFreed);
	}
}

ABSL_ATTRIBUTE_ALWAYS_INLINE static inline bool checkShutdownOnIndex(int i) {
	return i % CHECK_SHUTDOWN_INTERVAL == 0 && askedToShutdown();
}

ABSL_ATTRIBUTE_ALWAYS_INLINE static inline bool checkShutdownOnIndexLong(long i) {
	return i % CHECK_SHUTDOWN_INTERVAL == 0 && askedToShutdown();
}

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
 * Inputs	: BranchPath *node
 *
 * Looks at the node's Cook data. If the item is autoplaced, then add
 * a penalty for not tossing the item. Adjust framesTaken and
 * totalFramesTaken to reflect this change.
 -------------------------------------------------------------------*/
void applyJumpStorageFramePenalty(BranchPath *node) {
	if (((Cook *) node->description.data)->handleOutput == Autoplace) {
		node->description.framesTaken += JUMP_STORAGE_NO_TOSS_FRAMES;
		node->description.totalFramesTaken += JUMP_STORAGE_NO_TOSS_FRAMES;
	}
}

/*-------------------------------------------------------------------
 * Function 	: copyOutputsFulfilled
 * Inputs	: int *destOutputsFulfilled, int *srcOutputsFulfilled
 * Outputs	:
 * A simple memcpy to duplicate srcOutputsFulfilled into destOutputsFulfilled
 -------------------------------------------------------------------*/
ABSL_ATTRIBUTE_ALWAYS_INLINE static inline void copyOutputsFulfilled(outputCreatedArray_t destOutputsFulfilled, const outputCreatedArray_t srcOutputsFulfilled) {
	memcpy((void *)destOutputsFulfilled, (const void *)srcOutputsFulfilled, INT_OUTPUT_ARRAY_SIZE_BYTES);
}

/*-------------------------------------------------------------------
 * Function 	: createChapter5Struct
 * Inputs	: int				DB_place_index
 *		  int				CO_place_index
 *		  int				KM_place_index
 *		  int				CS_place_index
 *		  int				TR_use_index
 *		  int				lateSort
 * Outputs	: CH5			*ch5
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
 * Function 	: createCookDescription
 * Inputs	: BranchPath 	  *node
 *		  Recipe 	  recipe
 *		  ItemCombination combo
 *		  enum Type_Sort	  *tempInventory
 *		  int 			  *tempFrames
 *		  int 			  viableItems
 * Outputs	: MoveDescription useDescription
 *
 * Compartmentalization of generating a MoveDescription struct
 * based on various parameters dependent on what recip we're cooking
 -------------------------------------------------------------------*/
MoveDescription createCookDescription(const BranchPath *node, Recipe recipe, ItemCombination combo, Inventory *tempInventory, int *tempFrames, int viableItems) {
	MoveDescription useDescription;
	useDescription.action = ECook;

	int ingredientLoc[2];

	// Determine the locations of both ingredients
	ingredientLoc[0] = indexOfItemInInventory(*tempInventory, combo.item1);

	if (combo.numItems == 1) {
		createCookDescription1Item(node, recipe, combo, tempInventory, ingredientLoc, tempFrames, viableItems, &useDescription);
	}
	else {
		ingredientLoc[1] = indexOfItemInInventory(*tempInventory, combo.item2);
		createCookDescription2Items(node, recipe, combo, tempInventory, ingredientLoc, tempFrames, viableItems, &useDescription);
	}

	return useDescription;
}

/*-------------------------------------------------------------------
 * Function 	: createCookDescription1Item
 * Inputs	: BranchPath 		*node
 *		  Recipe 		recipe
 *		  ItemCombination 	combo
 *		  enum Type_Sort		*tempInventory
 *		  int 				*ingredientLoc
 *		  int 				*ingredientOffset
 *		  int 				*tempFrames
 *		  int 				viableItems
 		  MoveDescription	*useDescription
 *
 * Handles inventory management and frame calculation for recipes of
 * length 1. Generates Cook structure and points to this structure
 * in useDescription.
 -------------------------------------------------------------------*/
void createCookDescription1Item(const BranchPath *node, Recipe recipe, ItemCombination combo, Inventory *tempInventory, int *ingredientLoc, int *tempFrames, int viableItems, MoveDescription *useDescription) {
	// This is a potentially viable recipe with 1 ingredient
	// Determine how many frames will be needed to select that item
	*tempFrames = invFrames[viableItems - 1][ingredientLoc[0] - tempInventory->nulls];

	// Modify the inventory if the ingredient was in the first 10 slots
	if (ingredientLoc[0] < 10) {
		// Shift the inventory towards the front of the array to fill the null
		*tempInventory = removeItem(*tempInventory, ingredientLoc[0]);
	}

	generateCook(useDescription, combo, recipe, ingredientLoc, 0);
	generateFramesTaken(useDescription, node, *tempFrames);
}

/*-------------------------------------------------------------------
 * Function 	: createCookDescription2Items
 * Inputs	: BranchPath 		*node
 *		  Recipe 		recipe
 *		  ItemCombination 	combo
 *		  enum Type_Sort		*tempInventory
 *		  int 				*ingredientLoc
 *		  int 				*ingredientOffset
 *		  int 				*tempFrames
 *		  int 				viableItems
 		  MoveDescription	*useDescription
 *
 * Handles inventory management and frame calculation for recipes of
 * length 2. Swaps items if it's faster to choose the second item first.
 * Generates Cook structure and points to this structure in useDescription.
 -------------------------------------------------------------------*/
void createCookDescription2Items(const BranchPath *node, Recipe recipe, ItemCombination combo, Inventory *tempInventory, int *ingredientLoc, int *tempFrames, int viableItems, MoveDescription *useDescription) {
	// This is a potentially viable recipe with 2 ingredients
	//Baseline frames based on how many times we need to access the menu
	*tempFrames = CHOOSE_2ND_INGREDIENT_FRAMES;

	// Swap ingredient order if necessary. There are some configurations where
	// it is 2 frames faster to pick the ingredients in the reverse order or
	// only one order is possible.
	int swap = 0;
	if (selectSecondItemFirst(ingredientLoc, tempInventory->nulls, viableItems)) {
		swapItems(ingredientLoc);
		swap = 1;
	}

	int visibleLoc0 = ingredientLoc[0] - tempInventory->nulls;
	int visibleLoc1 = ingredientLoc[1] - tempInventory->nulls;

	// Add the frames to choose the first ingredient.
	*tempFrames += invFrames[viableItems - 1][visibleLoc0];

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
	*tempFrames += invFrames[viableItems - 1][visibleLoc1];

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
	generateCook(useDescription, combo, recipe, ingredientLoc, swap);
	generateFramesTaken(useDescription, node, *tempFrames);
}

/*-------------------------------------------------------------------
 * Function 	: createLegalMove
 * Inputs	: BranchPath		*node
 *		  enum Type_Sort		*inventory
 *		  MoveDescription	description
 *		  int				*outputsFulfilled
 *		  int				numOutputsFulfilled
 * Outputs	: BranchPath		*newLegalMove
 *
 * Given the input parameters, allocate and set attributes for a legalMove node
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
	
	serializeNode(newLegalMove);

	return newLegalMove;
}

/*-------------------------------------------------------------------
 * Function 	: filterOut2Ingredients
 * Inputs	: BranchPath		*node
 *
 * For the first node's legal moves, we cannot cook a recipe which
 * contains two items. Thus, we need to remove any legal moves
 * which require two ingredients
 -------------------------------------------------------------------*/
void filterOut2Ingredients(BranchPath *node) {
	for (int i = 0; i < node->numLegalMoves; i++) {
		if (node->legalMoves[i]->description.action == ECook) {
			Cook *cook = node->legalMoves[i]->description.data;
			if (cook->numItems == 2) {
				freeLegalMove(node, i);
				i--; // Update i so we don't skip over the newly moved legalMoves
			}
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: finalizeChapter5Eval
 * Inputs	: BranchPath		*node
 *		  enum Type_Sort		*inventory
 *		  enum Action			sort
 *		  CH5			*ch5Data
 *		  int 				temp_frame_sum
 *		  int				*outputsFulfilled
 *		  int				numOutputsFulfilled
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
 * Function 	: finalizeLegalMove
 * Inputs	: BranchPath		*node
 *		  int				tempFrames
 *		  MoveDescription	useDescription
 *		  enum Type_Sort		*tempInventory
 *		  int				*tempOutputsFulfilled
 *		  int				numOutputsFulfilled
 *		  enum HandleOutput		tossType
 *		  enum Type_Sort		toss
 *		  int				tossIndex
 *
 * Given input parameters, construct a new legal move to represent
 * a valid recipe move. Also checks to see if the legal move exceeds
 * the frame limit
 -------------------------------------------------------------------*/
void finalizeLegalMove(BranchPath *node, int tempFrames, MoveDescription useDescription, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, enum HandleOutput tossType, enum Type_Sort toss, int tossIndex) {
	// Determine if the legal move exceeds the frame limit. If so, return out
	if (useDescription.totalFramesTaken > getLocalRecord() + BUFFER_SEARCH_FRAMES) {
		return;
	}

	// Determine where to insert this legal move into the list of legal moves (sorted by frames taken)
	int insertIndex = getInsertionIndex(node, tempFrames);

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
 * Function 	: freeAllNodes
 * Inputs	: BranchPath	*node
 *
 * We've reached the iteration limit, so free all nodes in the roadmap
 * We additionally need to delete node from the previous node's list of
 * legalMoves to prevent a double-free
 -------------------------------------------------------------------*/
void freeAllNodes(BranchPath *node) {
	BranchPath *prevNode = NULL;

	do {
		prevNode = node->prev;

		// don't cache serials, as we haven't necessarily traversed all legal moves off of this node
		freeNode(node, false);

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
 * Function 	: freeLegalMoveOnly
 * Inputs	: BranchPath	*node
 *		  int			index
 *
 * Free the legal move at index in the node's array of legal moves,
 * but unlike freeLegalMove(), this does NOT shift the existing legal
 * moves to fill the gap. This is useful in cases where where the
 * caller can assure such consistency is not needed (For example,
 * freeing the last legal move or freeing all legal moves).
 -------------------------------------------------------------------*/
static void freeLegalMoveOnly(BranchPath *node, int index) {
	freeNode(node->legalMoves[index], true);
	node->legalMoves[index] = NULL;
	node->numLegalMoves--;
	node->next = NULL;
}

/*-------------------------------------------------------------------
 * Function 	: freeLegalMove
 * Inputs	: BranchPath	*node
 *		  int			index
 *
 * Free the legal move at index in the node's array of legal moves,
 * And shift the existing legal moves after the index to fill the gap.
 -------------------------------------------------------------------*/

void freeLegalMove(BranchPath *node, int index) {
	freeLegalMoveOnly(node, index);

	// Shift up the rest of the legal moves
	shiftUpLegalMoves(node, index + 1);
}

/*-------------------------------------------------------------------
 * Function 	: freeNode
 * Inputs	: BranchPath	*node
 *
 * Free the current node and all legal moves within the node
 -------------------------------------------------------------------*/
void freeNode(BranchPath *node, bool cache) {
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
			freeLegalMoveOnly(node, i++);
		}
		free(node->legalMoves);
	}

	if (cache)
		cacheSerial(node);

	if (node->serial.length > 0)
		free(node->serial.data);
	
	free(node);
}

/*-------------------------------------------------------------------
 * Function 	: fulfillChapter5
 * Inputs	: BranchPath	*curNode
 *
 * A preliminary step to determine Dried Bouquet and Coconut placement
 * before calling handleChapter5Eval
 -------------------------------------------------------------------*/
void fulfillChapter5(BranchPath *curNode) {
	// Create an outputs chart but with the Dried Bouquet collected
	// to ensure that the produced inventory can fulfill all remaining recipes
	outputCreatedArray_t tempOutputsFulfilled;
	copyOutputsFulfilled(tempOutputsFulfilled, curNode->outputCreated);
	tempOutputsFulfilled[getIndexOfRecipe(Dried_Bouquet)] = true;
	int numOutputsFulfilled = curNode->numOutputsCreated + 1;

	Inventory newInventory = curNode->inventory;

	int mousse_cake_index = indexOfItemInInventory(newInventory, Mousse_Cake);

	// Create the CH5 eval struct
	CH5_Eval eval;

	// Explicit int casts are to prevent intermediary underflows from the uint_8 math.
	int viableItems = (int)newInventory.length - newInventory.nulls - min((int)newInventory.length - 10, newInventory.nulls);

	// Calculate frames it takes the navigate to the Mousse Cake and the Hot Dog for the trade
	eval.frames_HD = 2 * invFrames[viableItems - 1][indexOfItemInInventory(newInventory, Hot_Dog) - newInventory.nulls];
	eval.frames_MC = invFrames[viableItems - 1][mousse_cake_index - newInventory.nulls];

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

	// We know tempOutputsFulfilled does not escape this scope, so safe to be unallocated on return.
}

/*-------------------------------------------------------------------
 * Function 	: fulfillRecipes
 * Inputs	: BranchPath	*curNode
 * 		  int			recipeIndex
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
		if (recipeIndex == getIndexOfRecipe(Dried_Bouquet)) {
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

			int tempFrames;

			MoveDescription useDescription = createCookDescription(curNode, recipe, combo, &newInventory, &tempFrames, viableItems);

			// Store the base useDescription's cook pointer to be freed later
			Cook *cookBase = (Cook *)useDescription.data;

			// Handle allocation of the output
			handleRecipeOutput(curNode, newInventory, tempFrames, useDescription, tempOutputsFulfilled, numOutputsFulfilled, recipe.output, viableItems);

			free(cookBase);
			// We know tempOutputsFulfilled does not escape this scope, so safe to be unallocated on return.
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: generateCook
 * Inputs	: MoveDescription	*description
 * 		  ItemCombination	combo
 *		  Recipe		recipe
 *		  int				*ingredientLoc
 *		  int				swap
 *
 * Given input parameters, generate Cook structure
 -------------------------------------------------------------------*/
void generateCook(MoveDescription *description, const ItemCombination combo, const Recipe recipe, const int *ingredientLoc, int swap) {
	Cook *cook = malloc(sizeof(Cook));

	checkMallocFailed(cook);

	description->action = ECook;
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
 * Inputs	: MoveDescription	*description
 * 		  BranchPath		*node
 *		  int				framesTaken
 *
 * Assign frame duration to description structure and reference the
 * previous node to find the total frame duration for the roadmap thus far
 -------------------------------------------------------------------*/
void generateFramesTaken(MoveDescription *description, const BranchPath *node, int framesTaken) {
	description->framesTaken = framesTaken;
	description->totalFramesTaken = node->description.totalFramesTaken + framesTaken;
}

/*-------------------------------------------------------------------
 * Function 	: getInsertionIndex
 * Inputs	: BranchPath	*curNode
 *		  int			frames
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
 * Inputs	: enum Action action
 * Outputs	: int frames
 *
 * Depending on the type of sort, return the corresponding frame cost.
 -------------------------------------------------------------------*/
int getSortFrames(enum Action action) {
	switch (action) {
		case ESort_Alpha_Asc:
			return ALPHA_SORT_FRAMES;
		case ESort_Alpha_Des:
			return REVERSE_ALPHA_SORT_FRAMES;
		case ESort_Type_Asc:
			return TYPE_SORT_FRAMES;
		case ESort_Type_Des:
			return REVERSE_TYPE_SORT_FRAMES;
		default:
			// Critical error if we reach this point...
			// action should be some type of sort
			exit(-2);
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleChapter5EarlySortEndItems
 * Inputs	: BranchPath	*node
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
void handleChapter5EarlySortEndItems(BranchPath *node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	for (eval.KM_place_index = 0; eval.KM_place_index < 10; eval.KM_place_index++) {
		// Don't allow current move to remove Thunder Rage or previously
		// obtained items
		if (inventory.inventory[eval.KM_place_index] == Thunder_Rage
			|| inventory.inventory[eval.KM_place_index] == Dried_Bouquet) {
			continue;
		}

		// Replace the chosen item with the Keel Mango
		Inventory km_temp_inventory = replaceItem(inventory, eval.KM_place_index, Keel_Mango);
		// Calculate the frames for this action
		eval.frames_KM = TOSS_FRAMES + invFrames[inventory.length][eval.KM_place_index + 1];

		for (eval.CS_place_index = 1; eval.CS_place_index < 10; eval.CS_place_index++) {
			// Don't allow current move to remove Thunder Rage or previously
			// obtained items
			if (eval.CS_place_index == eval.KM_place_index
				|| km_temp_inventory.inventory[eval.CS_place_index] == Thunder_Rage
				|| inventory.inventory[eval.KM_place_index] == Dried_Bouquet) {
				continue;
			}

			// Replace the chosen item with the Courage Shell
			Inventory kmcs_temp_inventory = replaceItem(km_temp_inventory, eval.CS_place_index, Courage_Shell);
			// Calculate the frames for this action
			eval.frames_CS = TOSS_FRAMES + invFrames[kmcs_temp_inventory.length][eval.CS_place_index + 1];

			// The next event is using the Thunder Rage item before resuming the 2nd session of recipe fulfillment
			eval.TR_use_index = indexOfItemInInventory(kmcs_temp_inventory, Thunder_Rage);
			if (eval.TR_use_index < 10) {
				kmcs_temp_inventory = removeItem(kmcs_temp_inventory, eval.TR_use_index);
			}
			// Calculate the frames for this action
			eval.frames_TR = invFrames[kmcs_temp_inventory.length - 1][eval.TR_use_index];

			// Calculate the frames of all actions done
			int temp_frame_sum = eval.frames_DB + eval.frames_CO + eval.frames_KM + eval.frames_CS + eval.frames_TR + eval.frames_HD + eval.frames_MC + eval.sort_frames;

			// Determine if the remaining inventory is sufficient to fulfill all remaining recipes
			if (stateOK(kmcs_temp_inventory, outputsFulfilled, recipeList)) {
				CH5 *ch5Data = createChapter5Struct(eval, 0);
				finalizeChapter5Eval(node, kmcs_temp_inventory, ch5Data, temp_frame_sum, outputsFulfilled, numOutputsFulfilled);
			}
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleChapter5Eval
 * Inputs	: BranchPath	*node
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
void handleChapter5Eval(BranchPath *node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// Evaluate sorting before the Keel Mango
	// Use -1 to identify that we are not collecting the Keel Mango until after the sort
	eval.frames_KM = -1;
	eval.KM_place_index = -1;
	handleChapter5Sorts(node, inventory, outputsFulfilled, numOutputsFulfilled, eval);

	// Place the Keel Mango in a null spot if one is available.
	if (inventory.nulls >= 1) {
		// Making a copy of the temp inventory for what it looks like after the allocation of the KM
		Inventory km_temp_inventory = addItem(inventory, Keel_Mango);
		eval.frames_KM = 0;
		eval.KM_place_index = 0;

		// Perform all sorts
		handleChapter5Sorts(node, km_temp_inventory, outputsFulfilled, numOutputsFulfilled, eval);

	}
	else {
		// Place the Keel Mango starting after the other placed items.
		for (eval.KM_place_index = 2; eval.KM_place_index < 10; eval.KM_place_index++) {
			// Don't allow current move to remove Thunder Rage
			if (inventory.inventory[eval.KM_place_index] == Thunder_Rage) {
				continue;
			}

			// Making a copy of the temp inventory for what it looks like after the allocation of the KM
			Inventory km_temp_inventory = replaceItem(inventory, eval.KM_place_index, Keel_Mango);
			// Calculate the frames for this action
			eval.frames_KM = TOSS_FRAMES + invFrames[inventory.length][eval.KM_place_index + 1];

			// Perform all sorts
			handleChapter5Sorts(node, km_temp_inventory, outputsFulfilled, numOutputsFulfilled, eval);
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleChapter5LateSortEndItems
 * Inputs	: BranchPath	*node
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
void handleChapter5LateSortEndItems(BranchPath *node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// Place the Courage Shell
	for (eval.CS_place_index = 0; eval.CS_place_index < 10; eval.CS_place_index++) {
		// Don't allow current move to remove Thunder Rage
		if (inventory.inventory[eval.CS_place_index] == Thunder_Rage) {
			continue;
		}

		// Replace the chosen item with the Courage Shell
		Inventory cs_temp_inventory = replaceItem(inventory, eval.CS_place_index, Courage_Shell);
		// Calculate the frames for this action
		eval.frames_CS = TOSS_FRAMES + invFrames[cs_temp_inventory.length][eval.CS_place_index + 1];

		// The next event is using the Thunder Rage
		eval.TR_use_index = indexOfItemInInventory(cs_temp_inventory, Thunder_Rage);
		// Using the Thunder Rage in slots 1-10 will cause a NULL to appear in that slot
		if (eval.TR_use_index < 10) {
			cs_temp_inventory = removeItem(cs_temp_inventory, eval.TR_use_index);
		}
		// Calculate the frames for this action
		eval.frames_TR = invFrames[cs_temp_inventory.length - 1][eval.TR_use_index];

		// Calculate the frames of all actions done
		int temp_frame_sum = eval.frames_DB + eval.frames_CO + eval.frames_KM + eval.frames_CS + eval.frames_TR + eval.frames_HD + eval.frames_MC + eval.sort_frames;

		if (stateOK(cs_temp_inventory, outputsFulfilled, recipeList)) {
			CH5 *ch5Data = createChapter5Struct(eval, 1);
			finalizeChapter5Eval(node, cs_temp_inventory, ch5Data, temp_frame_sum, outputsFulfilled, numOutputsFulfilled);
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleChapter5Sorts
 * Inputs	: BranchPath	*node
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
void handleChapter5Sorts(BranchPath *node, Inventory inventory, const outputCreatedArray_t outputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	for (eval.sort = ESort_Alpha_Asc; eval.sort <= ESort_Type_Des; eval.sort++) {
		Inventory sorted_inventory = getSortedInventory(inventory, eval.sort);

		// Only bother with further evaluation if the sort placed the Coconut in the latter half of the inventory
		// because the Coconut is needed for duplication
		if (indexOfItemInInventory(sorted_inventory, Coconut) < 10) {
			continue;
		}

		// Handle all placements of the Keel Mango, Courage Shell, and usage of the Thunder Rage
		eval.sort_frames = getSortFrames(eval.sort);

		if (eval.frames_KM == -1) {
			handleChapter5EarlySortEndItems(node, sorted_inventory, outputsFulfilled, numOutputsFulfilled, eval);
			continue;
		}

		handleChapter5LateSortEndItems(node, sorted_inventory, outputsFulfilled, numOutputsFulfilled, eval);
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleDBCOAllocation0Nulls
 * Inputs	: BranchPath	*curNode
 *		  enum Type_Sort	*tempInventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			viableItems
 *
 * Preliminary function to allocate Dried Bouquet and Coconut before
 * evaluating the rest of Chapter 5. There are no nulls in the inventory.
 -------------------------------------------------------------------*/
void handleDBCOAllocation0Nulls(BranchPath *curNode, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// No nulls to utilize for Chapter 5 intermission
	// Both the DB and CO can only replace items in the first 10 slots
	// The remaining items always slide down to fill the vacancy
	// The DB will eventually end up in slot #2 and
	// the CO will eventually end up in slot #1
	for (eval.DB_place_index = 0; eval.DB_place_index < 10; eval.DB_place_index++) {
		// Don't allow current move to remove Thunder Rage
		if (tempInventory.inventory[eval.DB_place_index] == Thunder_Rage) {
			continue;
		}

		// Replace the chosen item with the Dried Bouquet
		Inventory db_temp_inventory = replaceItem(tempInventory, eval.DB_place_index, Dried_Bouquet);
		// Calculate the frames for this action
		eval.frames_DB = TOSS_FRAMES + invFrames[tempInventory.length][eval.DB_place_index + 1];

		for (eval.CO_place_index = 1; eval.CO_place_index < 10; eval.CO_place_index++) {
			// Don't allow current move to remove needed items
			if (eval.CO_place_index == eval.DB_place_index
				|| db_temp_inventory.inventory[eval.CO_place_index] == Thunder_Rage) {
				continue;
			}

			// Replace the chosen item with the Coconut
			Inventory dbco_temp_inventory = replaceItem(db_temp_inventory, eval.CO_place_index, Coconut);

			// Calculate the frames of this action
			eval.frames_CO = TOSS_FRAMES + invFrames[tempInventory.length][eval.CO_place_index + 1];

			// Handle the allocation of the Coconut sort, Keel Mango, and Courage Shell
			handleChapter5Eval(curNode, dbco_temp_inventory, tempOutputsFulfilled, numOutputsFulfilled, eval);
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleDBCOAllocation1Null
 * Inputs	: BranchPath	*curNode
 *		  enum Type_Sort		*tempInventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			viableItems
 *
 * Preliminary function to allocate Dried Bouquet and Coconut before
 * evaluating the rest of Chapter 5. There is 1 null in the inventory.
 -------------------------------------------------------------------*/
void handleDBCOAllocation1Null(BranchPath *curNode, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// The Dried Bouquet gets auto-placed in the 1st slot,
	// and everything else gets shifted down one to fill the first NULL
	tempInventory = addItem(tempInventory, Dried_Bouquet);
	eval.DB_place_index = 0;
	eval.frames_DB = 0;

	// Dried Bouquet will always be in the first slot
	for (eval.CO_place_index = 1; eval.CO_place_index < 10; eval.CO_place_index++) {
		// Don't waste time replacing the Thunder Rage with the Coconut
		if (tempInventory.inventory[eval.CO_place_index] == Thunder_Rage) {
			continue;
		}

		// Replace the item with the Coconut
		Inventory co_temp_inventory = replaceItem(tempInventory, eval.CO_place_index, Coconut);
		// Calculate the number of frames needed to pick this slot for replacement
		eval.frames_CO = TOSS_FRAMES + invFrames[tempInventory.length][eval.CO_place_index + 1];

		// Handle the allocation of the Coconut sort, Keel Mango, and Courage Shell
		handleChapter5Eval(curNode, co_temp_inventory, tempOutputsFulfilled, numOutputsFulfilled, eval);
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleDBCOAllocation2Nulls
 * Inputs	: BranchPath	*curNode
 *		  enum Type_Sort	*tempInventory
 *		  int			*outputsFulfilled
 *		  int			numOutputsFulfilled
 *		  int			viableItems
 *
 * Preliminary function to allocate Dried Bouquet and Coconut before
 * evaluating the rest of Chapter 5. There are >=2 nulls in the inventory.
 -------------------------------------------------------------------*/
void handleDBCOAllocation2Nulls(BranchPath *curNode, Inventory tempInventory, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, CH5_Eval eval) {
	// The Dried Bouquet gets auto-placed due to having nulls
	tempInventory = addItem(tempInventory, Dried_Bouquet);
	eval.DB_place_index = 0;
	eval.frames_DB = 0;

	// The Coconut gets auto-placed due to having nulls
	tempInventory = addItem(tempInventory, Coconut);
	eval.CO_place_index = 0;
	eval.frames_CO = 0;

	// Handle the allocation of the Coconut, Sort, Keel Mango, and Courage Shell
	handleChapter5Eval(curNode, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, eval);
}

/*-------------------------------------------------------------------
 * Function 	: handleRecipeOutput
 * Inputs	: BranchPath		*curNode
 *		  enum Type_Sort		*tempInventory
 *		  int				tempFrames
 *		  MoveDescription	useDescription
 *		  int				*tempOutputsFulfilled
 *		  int				numOutputsFulfilled
 *		  enum Type_Sortf		output
 *		  int				viableItems
 *
 * After detecting that a recipe can be satisfied, see how we can handle
 * the output (either tossing the output, auto-placing it if there is a
 * null slot, or tossing a different item in the inventory)
 -------------------------------------------------------------------*/
void handleRecipeOutput(BranchPath *curNode, Inventory tempInventory, int tempFrames, MoveDescription useDescription, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, enum Type_Sort output, int viableItems) {
	// Options vary by whether there are NULLs within the inventory
	if (tempInventory.nulls >= 1) {
		tempInventory = addItem(tempInventory, ((Cook*)useDescription.data)->output);

		// Check to see if this state is viable
		if(stateOK(tempInventory, tempOutputsFulfilled, recipeList)) {
			finalizeLegalMove(curNode, tempFrames, useDescription, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, Autoplace, -1, -1);
		}
	}
	else {
		// There are no NULLs in the inventory. Something must be tossed
		// Total number of frames increased by forcing to toss something
		tempFrames += TOSS_FRAMES;
		useDescription.framesTaken += TOSS_FRAMES;
		useDescription.totalFramesTaken += TOSS_FRAMES;

		// Evaluate the viability of tossing all current inventory items
		// Assumed that it is impossible to toss and replace any items in the last 10 positions
		tryTossInventoryItem(curNode, tempInventory, useDescription, tempOutputsFulfilled, numOutputsFulfilled, output, tempFrames, viableItems);

		// Evaluate viability of tossing the output item itself
		if (stateOK(tempInventory, tempOutputsFulfilled, recipeList)) {
			finalizeLegalMove(curNode, tempFrames, useDescription, tempInventory, tempOutputsFulfilled, numOutputsFulfilled, Toss, output, -1);
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleSelectAndRandom
 * Inputs	: BranchPath	*curNode
 *		  int 			select
 *		  int 			randomise
 *
 * Based on configuration parameters select and randomise within config.txt,
 * manage the array of legal moves based on the designated behavior of the parameters.
 -------------------------------------------------------------------*/
void handleSelectAndRandom(BranchPath *curNode, int select, int randomise) {
	/*if (select && curNode->moves < 55 && curNode->numLegalMoves > 0) {
		softMin(curNode);
	}*/

	// Old method of handling select
	// Somewhat random process of picking the quicker moves to recurse down
	// Arbitrarily skip over the fastest legal move with a given probability
	if (select && curNode->moves < 55 && curNode->numLegalMoves > 0) {
		int nextMoveIndex = 0;
		while (nextMoveIndex < curNode->numLegalMoves - 1 && rand() % 100 < 50) {
			if (checkShutdownOnIndex(nextMoveIndex)) {
				break;
			}
			nextMoveIndex++;
		}

		if (askedToShutdown()) {
			return;
		}

		// Take the legal move at nextMoveIndex and move it to the front of the array
		BranchPath *nextMove = curNode->legalMoves[nextMoveIndex];
		curNode->legalMoves[nextMoveIndex] = NULL;
		shiftDownLegalMoves(curNode, 0, nextMoveIndex);
		curNode->legalMoves[0] = nextMove;
	}

	// When not doing the select methodology, and opting for randomize
	// just shuffle the entire list of legal moves and pick the new first item
	else if (randomise) {
		if (askedToShutdown()) {
			return;
		}
		shuffleLegalMoves(curNode);
	}
}

/*-------------------------------------------------------------------
 * Function 	: handleSorts
 * Inputs	: BranchPath	*curNode
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
 * Function 	: initializeRoot
 * Inputs	: struct Job		job
 * Outputs	: BranchPath	*root
 *
 * Generate the root of the tree graph
 -------------------------------------------------------------------*/
ABSL_MUST_USE_RESULT BranchPath *initializeRoot() {
	BranchPath *root = malloc(sizeof(BranchPath));

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
	serializeNode(root);
	return root;
}

/*-------------------------------------------------------------------
 * Function 	: insertIntoLegalMoves
 * Inputs	: int			insertIndex
 *		  BranchPath	*newLegalMove
 *		  BranchPath	*curNode
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
 * Function 	: copyAllNodes
 * Inputs	: BranchPath	*newNode
 *		  BranchPath	*oldNode
 * Outputs	: BranchPath	*newNode
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
		serializeNode(newNode);
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
 * Function 	: optimizeRoadmap
 * Inputs	: BranchPath		*root
 * Outputs	: struct OptimizeResult	result
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
 * Function : periodicGithubCheck
 * Inputs	:
 *
 * Check for the most recent Github repository release version. If there
 * is a newer version, alert the user.
 -------------------------------------------------------------------*/
void periodicGithubCheck() {
	// Double check the latest release on Github
	int update = checkForUpdates(getLocalVersion());
	if (update == -1) {
		printf("Could not check version on Github. Please check your internet connection.\n");
		printf("Otherwise, completed roadmaps may be inaccurate!\n");
	}
	else if (update == 1) {
		printf("Please visit https://github.com/SevenChords/CipesAtHome/releases to download the newest version of this program!\n");
		printf("Press ENTER to exit the program.\n");
		awaitKeyFromUser();
		exit(1);
	}
}

/*-------------------------------------------------------------------
 * Function 	: popAllButFirstLegalMove
 * Inputs	: BranchPath	*node
 *
 * In the event we are within the last few nodes of the roadmap, get rid
 * of all but the fastest legal move.
 -------------------------------------------------------------------*/
void popAllButFirstLegalMove(struct BranchPath *node) {
	// First case, we may need to set the final slot to NULL (the one _after_ the last element)
	// Which is handled by the full freeLegalMoves (in the shiftUpLegalMoves inner call).
	int i = node->numLegalMoves - 1;
	freeLegalMove(node, i--);
	for (; i > 0 ; --i) {
		// Now we don't need to check final slot.
		freeLegalMoveOnly(node, i);
	}
}

/*-------------------------------------------------------------------
 * Function : reallocateRecipes
 * Inputs	: BranchPath	*newRoot
 *			  enum Type_Sort	*rearranged_recipes
 *			  int				num_rearranged_recipes
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
		int recipe_index = getIndexOfRecipe(rearranged_recipes[recipe_offset]);
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
				int indexItem2 = UNSET_INDEX_SIGNED;
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
						printf("Fatal error! indexItem2 was not set in a branch where it should have.\n");
						printf("Press enter to quit.");
						awaitKeyFromUser();
						exit(1);
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
		insertNode->serial = (Serial) {0, NULL};

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
 * Inputs	: BranchPath	*node
 *			  enum Type_Sort	*rearranged_recipes
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
			newNode->outputCreated[getIndexOfRecipe(tossed_item)] = 0;
			newNode->numOutputsCreated--;
			newNode = newNode->next;
		}

		// Now, get rid of this current node
		checkMallocFailed(node->next);
		node->prev->next = node->next;
		node->next->prev = node->prev;
		newNode = node->prev;
		freeNode(node, false);
		node = newNode;
	}

	return num_rearranged_recipes;
}

/*-------------------------------------------------------------------
 * Function 	: selectSecondItemFirst
 * Inputs	: BranchPath 		*node
 *		  ItemCombination 	combo
 *		  int 				*ingredientLoc
 *		  int 				nulls
 *		  int 				viableItems
 * Outputs	: int (0 or 1)
 *
 * Determines whether it is either required or faster to select the
 * second item before the first item originally listed in the recipe
 * combo.
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
 * Function 	: shiftDownLegalMoves
 * Inputs	: BranchPath	*node
 *		  int			lowerBound
 *		  int			upperBound
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
 * Function 	: shiftUpLegalMoves
 * Inputs	: BranchPath	*node
 *		  int			index
 *
 * There is a NULL in the array of legal moves. The first valid legal
 * move AFTER the null is index. Iterate starting at the index of the
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
 * Function 	: softMin
 * Inputs	: BranchPath	*node
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
void softMin(BranchPath *node) {
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
	BranchPath *softMinNode = node->legalMoves[index];
	node->legalMoves[index] = NULL;

	// Make room at the beginning of the legal moves array for the softMinNode
	shiftDownLegalMoves(node, 0, index);

	// Set first index in array to the softMinNode
	node->legalMoves[0] = softMinNode;
}

/*-------------------------------------------------------------------
 * Function 	: shuffleLegalMoves
 * Inputs	: BranchPath	*node
 *
 * Randomize the order of legal moves by switching two legal moves
 * numlegalMoves times.
 -------------------------------------------------------------------*/
void shuffleLegalMoves(BranchPath *node) {
	// Swap 2 legal moves a variable number of times
	for (int i = 0; i < node->numLegalMoves; i++) {
		if (checkShutdownOnIndex(i)) {
			break;
		}
		int index1 = rand() % node->numLegalMoves;
		int index2 = rand() % node->numLegalMoves;
		BranchPath *temp = node->legalMoves[index1];
		node->legalMoves[index1] = node->legalMoves[index2];
		node->legalMoves[index2] = temp;
	}
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
void swapItems(int *ingredientLoc) {
	int locTemp;

	locTemp = ingredientLoc[0];
	ingredientLoc[0] = ingredientLoc[1];
	ingredientLoc[1] = locTemp;
}

/*-------------------------------------------------------------------
 * Function 	: tryTossInventoryItem
 * Inputs	: BranchPath 	  *curNode
 *		  enum Type_Sort	  *tempInventory
 *		  MoveDescription useDescription
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
void tryTossInventoryItem(BranchPath *curNode, Inventory tempInventory, MoveDescription useDescription, const outputCreatedArray_t tempOutputsFulfilled, int numOutputsFulfilled, enum Type_Sort output, int tempFrames, int viableItems) {
	for (int tossedIndex = 0; tossedIndex < 10; tossedIndex++) {
		enum Type_Sort tossedItem = tempInventory.inventory[tossedIndex];

		// Make a copy of the tempInventory with the replaced item
		Inventory replacedInventory = replaceItem(tempInventory, tossedIndex, output);

		if (!stateOK(replacedInventory, tempOutputsFulfilled, recipeList)) {
			continue;
		}

		// Calculate the additional tossed frames.
		int tossFrames = invFrames[viableItems][tossedIndex + 1];
		int replacedFrames = tempFrames + tossFrames;

		useDescription.framesTaken += tossFrames;
		useDescription.totalFramesTaken += tossFrames;

		finalizeLegalMove(curNode, replacedFrames, useDescription, replacedInventory, tempOutputsFulfilled, numOutputsFulfilled, TossOther, tossedItem, tossedIndex);
	}
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

	return getAlphaKey(item1) - getAlphaKey(item2);
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

	return getAlphaKey(item2) - getAlphaKey(item1);
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
Inventory getSortedInventory(Inventory inventory, enum Action sort) {
	// Set up the inventory for sorting
	memmove(inventory.inventory, inventory.inventory + inventory.nulls,
		(inventory.length - inventory.nulls) * sizeof(enum Type_Sort));
	inventory.length -= inventory.nulls;
	inventory.nulls = 0;

	// Use qsort and execute sort function depending on sort type
	switch(sort) {
		case ESort_Alpha_Asc:
			qsort((void*)inventory.inventory, inventory.length, sizeof(enum Type_Sort), alpha_sort);
			return inventory;
		case ESort_Alpha_Des:
			qsort((void*)inventory.inventory, inventory.length, sizeof(enum Type_Sort), alpha_sort_reverse);
			return inventory;
		case ESort_Type_Asc:
			qsort((void*)inventory.inventory, inventory.length, sizeof(enum Type_Sort), type_sort);
			return inventory;
		case ESort_Type_Des:
			qsort((void*)inventory.inventory, inventory.length, sizeof(enum Type_Sort), type_sort_reverse);
			return inventory;
		default:
			printf("Error in sorting inventory.\n");
			exit(2);
	}
}

void logIterations(int ID, int stepIndex, const BranchPath * curNode, int iterationCount, int level)
{
	char callString[30];
	char iterationString[100];
	sprintf(callString, "Call %d", ID);
	sprintf(iterationString, "%d steps currently taken, %d frames accumulated so far; %dk iterations",
		stepIndex, curNode->description.totalFramesTaken, iterationCount / 1000);
	recipeLog(level, "Calculator", "Info", callString, iterationString);
}

/*-------------------------------------------------------------------
 * Function 	: calculateOrder
 * Inputs	: int ID
 * Outputs	: Result	result
 *
 * This is the main roadmap evaluation function. This calls various
 * child functions to generate a sequence of legal moves. It then
 * uses parameters to determine which legal move to traverse to. Once
 * a roadmap is found, the data is printed to a .txt file, and the result
 * is passed back to start.c to try submitting to the server.
 -------------------------------------------------------------------*/
Result calculateOrder(const int ID) {
	int randomise = getConfigInt("randomise");
	int select = getConfigInt("select");
	int debug = getConfigInt("debug");
	// The user may disable all randomization but not be debugging.
	int freeRunning = !debug && !randomise && !select;
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

		total_dives++;

		if (total_dives % branchInterval == 0) {
			char temp1[30];
			char temp2[40];
			sprintf(temp1, "Call %d", ID);
			sprintf(temp2, "Searching New Branch %d", total_dives);
			recipeLog(3, "Calculator", "Info", temp1, temp2);
		}

		// If the user is not exploring only one branch, reset when it is time
		// Start iteration loop
		while (iterationCount < iterationLimit || freeRunning) {
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
							if (debug) {
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
				freeLegalMove(curNode, 0);
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
				if (!curNode->outputCreated[getIndexOfRecipe(Dried_Bouquet)]
					&& indexOfItemInInventory(curNode->inventory, Mousse_Cake) != -1
					&& indexOfItemInInventory(curNode->inventory, Hot_Dog) >= 10) {
					fulfillChapter5(curNode);
				}

				// Special handling of inventory sorting
				// Avoid redundant searches
				if (curNode->description.action == EBegin || curNode->description.action == ECook || curNode->description.action == ECh5) {
					handleSorts(curNode);
				}

				// All legal moves evaluated and listed!

				if (curNode->moves == 0) {
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
				// Apply randomization when not debugging or when done
				// choosing moves
				else if (!debug || freeRunning) {
					handleSelectAndRandom(curNode, select, randomise);
				}

				if (curNode->numLegalMoves == 0) {
					// There are no legal moves to iterate on
					// Go back up!

					// Handle the case where the root node runs out of legal moves
					if (curNode->prev == NULL) {
						freeNode(curNode, false);
						return (Result) {-1, -1};
					}

					curNode = curNode->prev;
					freeLegalMove(curNode, 0);
					curNode->next = NULL;
					stepIndex--;
					continue;
				}

				// Allow the user to choose their path when in debugging mode
				else if (debug && !freeRunning) {
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
						handleSelectAndRandom(curNode, select, randomise);
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
						freeNode(curNode, false);
						return (Result) {-1, -1};
					}

					curNode = curNode->prev;
					freeLegalMove(curNode, 0);
					curNode->next = NULL;
					stepIndex--;
					continue;
				}

				// Moves would already be shuffled with randomise, but select
				// would always choose the first one unless we select here
				handleSelectAndRandom(curNode, select, 0);

				// Once the list is generated, choose the top-most (quickest) path and iterate downward
				curNode->next = curNode->legalMoves[0];
				curNode = curNode->legalMoves[0];
				stepIndex++;

				// Logging for progress display
				iterationCount++;
				if (iterationCount % (branchInterval * DEFAULT_ITERATION_LIMIT) == 0
					&& (freeRunning || iterationLimit != DEFAULT_ITERATION_LIMIT)) {
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
		{
			writePersonalBest(&result_cache);

			// Return the cached result
			return result_cache;
		}

		// Period check for Github update (only perform on thread 0)
		if (total_dives % 10000 == 0 && omp_get_thread_num() == 0) {
			periodicGithubCheck();
		}

		// For profiling
		/*if (total_dives == 100) {
			exit(1);
		}*/

	}

	// Unexpected break out of loop. Return the nothing results.
	return (Result) { -1, -1 };
}

void writePersonalBest(Result *result)
{
	// Enter critical section to prevent corrupted file
	#pragma omp critical(pb)
	{
		// Prevent slower threads from overwriting a faster record in PB.txt
		// by first checking the current record
		FILE* fp = NULL;
		if ((fp = fopen("results/PB.txt", "r+")) == NULL) {
			// The file has not been created
			fp = fopen("results/PB.txt", "w");
		}
		else {
			// Verify that this new roadmap is faster than PB
			int pb_record = 9999;
			int num_assigned = fscanf(fp, "%d", &pb_record);
			fclose(fp);
			fp = NULL;
			if (ABSL_PREDICT_FALSE(num_assigned < 1)) {
				recipeLog(1, "Calculator", "Roadmap", "Error", "Unable to read current PB, overwriting.");
			}
			else if (ABSL_PREDICT_FALSE(pb_record < 0 || pb_record >= 9999)) {
				recipeLog(1, "Calculator", "Roadmap", "Error", "Current PB file is corrupt, overwriting with new PB.");
			}
			if (result->frames > pb_record) {
				// This is a slower thread and a faster record was already found
				*result = (Result) { -1, -1 };
			}
			else {
				if (ABSL_PREDICT_TRUE(result->frames > -1)) {
					fp = fopen("results/PB.txt", "w");
				}
			}
		}

		// Modify PB.txt
		if (fp != NULL) {
			if (ABSL_PREDICT_FALSE(result->frames < 0)) {
				// Somehow got invalid number of frames.
				// Fetch the current known max from the global.
				int localRecord = getLocalRecord();
				if (ABSL_PREDICT_FALSE(localRecord < 0)) {
					recipeLog(1, "Calculator", "Roadmap", "Error", "Current cached local record is corrupt (less then 0 frames). Not writing invalid PB file but your PB may be lost.");
				} else {
					fprintf(fp, "%d", localRecord);
				}
			} else {
				recipeLog(3, "Calculator", "Roadmap", "PB", "Wrote to PB.txt");
				fprintf(fp, "%d", result->frames);
			}
			fclose(fp);
		}
	}
}