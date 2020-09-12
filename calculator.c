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

// TODO: Eliminate struct Item, solely use t_key. If sort required, use function to look up a_key
// 	 	- Use alpha_sort_positions[item/t_key] to quickly index for the a_key

#define NUM_RECIPES 58 // Including Dried Bouquet trade

// Total frames to choose an additional ingredient (as opposed to just a single ingredient)
// This does not include the additional frames needed to navigate to the items that you want to use
#define CHOOSE_2ND_INGREDIENT_FRAMES 56

// Total frames to toss any item (as opposed to the item being automatically placed in the inventory when a NULL space exists
// This does not include the additional frames needed to navigate to the item that you want to toss
#define TOSS_FRAMES 32

// Frames needed to sort the inventory by each method
#define ALPHA_SORT_FRAMES 38
#define REVERSE_ALPHA_SORT_FRAMES 40
#define TYPE_SORT_FRAMES 39
#define REVERSE_TYPE_SORT_FRAMES 41

// If the player does not toss the final output item, 5 extra frames are needed to obtain Jump Storage
#define JUMP_STORAGE_NO_TOSS_FRAMES 5

// Finished roadmaps can potentially have some legal moves rearranged to faster points in time
// Give the search space some buffer frames so that if a roadmap is discovered that is "close" to the frame record,
// perform optimal shuffling of the moves to find the best possible rearranged roadmap and evaluate for new records
#define BUFFER_SEARCH_FRAMES 150

int **invFrames;
struct Recipe  *recipeList;

void applyJumpStorageFramePenalty(struct BranchPath *node) {
	if (((struct Cook *) node->description.data)->handleOutput == Autoplace) {
		node->description.framesTaken += JUMP_STORAGE_NO_TOSS_FRAMES;
		node->description.totalFramesTaken += JUMP_STORAGE_NO_TOSS_FRAMES;
	}
	
	return;
}

void copyCook(struct Cook *cookNew, struct Cook *cookOld) {
	cookNew->numItems = cookOld->numItems;
	cookNew->item1 = cookOld->item1;
	cookNew->itemIndex1 = cookOld->itemIndex1;
	cookNew->item2 = cookOld->item2;
	cookNew->itemIndex2 = cookOld->itemIndex2;
	cookNew->output = cookOld->output;
	cookNew->handleOutput = cookOld->handleOutput;
	cookNew->toss = cookOld->toss;
	cookNew->indexToss = cookOld->indexToss;
	return;
}

void copyOutputsFulfilled(int *newOutputsFulfilled, int *oldOutputsFulfilled) {
	for (int i = 0; i < NUM_RECIPES; i++) {
		newOutputsFulfilled[i] = oldOutputsFulfilled[i];
	}
	return;
}

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

struct BranchPath *createLegalMove(struct BranchPath *node, struct Item *inventory, struct MoveDescription description, int *outputsFulfilled) {
	struct BranchPath *newLegalMove = malloc(sizeof(struct BranchPath));
	newLegalMove->moves = node->moves + 1;
	newLegalMove->inventory = inventory;
	newLegalMove->description = description;
	newLegalMove->prev = node;
	newLegalMove->next = NULL;
	newLegalMove->outputCreated = outputsFulfilled;
	newLegalMove->numOutputsCreated = node->numOutputsCreated + 1;
	newLegalMove->legalMoves = NULL;
	newLegalMove->numLegalMoves = 0;
	return newLegalMove;
}

void filterLegalMovesExceedFrameLimit(struct BranchPath *node, int frames) {
	int numLegalMoves = node->numLegalMoves;
	
	for (int i = 0; i < numLegalMoves; i++) {
		if (node->legalMoves[i]->description.totalFramesTaken > frames) {
			freeLegalMove(node, i);
			// Update i so we don't skip over the newly moved legalMoves
			i--;
			numLegalMoves--;
		}
	}
}

void filterOut2Ingredients(struct BranchPath *node) {
	int numLegalMoves = node->numLegalMoves;
	for (int i = 0; i < numLegalMoves; i++) {
		if (node->legalMoves[i]->description.action == Cook) {
			struct Cook *cook = node->legalMoves[i]->description.data;
			if (cook->numItems == 2) {
				freeLegalMove(node, i);
				// Update i so we don't skip over the newly moved legalMoves
				i--;
				numLegalMoves = node->numLegalMoves;
			}
		}
	}
}

void finalizeChapter5Eval(struct BranchPath *node, struct Item *inventory, enum Action sort, int DB_place_index, int CO_place_index, int KM_place_index, int CS_place_index, int TR_use_index, int temp_frame_sum, int *outputsFulfilled) {
	// Get the index of where to insert this legal move to
	int insertIndex = getInsertionIndex(node, temp_frame_sum);
	
	// Copy the inventory
	struct Item *legalInventory = malloc(sizeof(struct Item) * 20);
	copyInventory(legalInventory, inventory);
	
	// Describe how the break should play out
	struct CH5 *ch5 = malloc(sizeof(struct CH5));
	ch5->indexDriedBouquet = DB_place_index;
	ch5->indexCoconut = CO_place_index;
	ch5->ch5Sort = sort;
	ch5->indexKeelMango = KM_place_index;
	ch5->indexCourageShell = CS_place_index;
	ch5->indexThunderRage = TR_use_index;
	struct MoveDescription description;
	description.action = Ch5;
	description.data = ch5;
	description.framesTaken = temp_frame_sum;
	description.totalFramesTaken = node->description.totalFramesTaken + temp_frame_sum;
	description.data = ch5;
	int *copyOfOutputsFulfilled = malloc(sizeof(int) * 58);
	copyOutputsFulfilled(copyOfOutputsFulfilled, outputsFulfilled);
	struct BranchPath *legalMove = createLegalMove(node, legalInventory, description, copyOfOutputsFulfilled);
	
	// Apend the legal move
	insertIntoLegalMoves(insertIndex, legalMove, node);
	assert(node->legalMoves[node->numLegalMoves-1] != NULL);
}

void finalizeLegalMove(struct BranchPath *node, int tempFrames, struct MoveDescription useDescription, struct Item *tempInventory, struct Cook *cookBase, int *tempOutputsFulfilled, enum HandleOutput tossType, struct Item toss, int tossIndex) {
	// This is a viable state that doesn't increase frames at all (output was auto-placed)
	// Determine where to insert this legal move into the list of legal moves (sorted by frames taken)
	int insertIndex = getInsertionIndex(node, tempFrames);
	
	struct Item *legalInventory = malloc(sizeof(struct Item) * 20);
	copyInventory(legalInventory, tempInventory);
	struct Cook *cookNew = malloc(sizeof(struct Cook));
	copyCook(cookNew, cookBase);
	cookNew->handleOutput = tossType;
	cookNew->toss = toss;
	cookNew->indexToss = tossIndex;
	useDescription.data = cookNew;
	int *copyOfOutputsFulfilled = malloc(sizeof(int) * 58);
	copyOutputsFulfilled(copyOfOutputsFulfilled, tempOutputsFulfilled);
	
	// Create the legalMove node
	struct BranchPath *newLegalMove = createLegalMove(node, legalInventory, useDescription, copyOfOutputsFulfilled);
	
	// Insert this new move into the current node's legalMove array
	insertIntoLegalMoves(insertIndex, newLegalMove, node);
}

void freeAllNodes(struct BranchPath *node) {
	struct BranchPath *nextNode = NULL;
	
	/*
	// We're at the deepest node. Print some debugging information
	printf("Terminating all nodes. The following recipes were not satisfied at the deepest node after %d steps:\n", node->moves);

	for (int i = 0; i < NUM_RECIPES; i++) {
		if (node->outputCreated[i] == 0) {
			printf("%s\n", getItemName(recipeList[i].output.a_key));
		}
	}
	*/
	while(1) {
		nextNode = node->prev;
		freeNode(node);
		// Delete node in nextNode's list of legal moves to prevent a double free
		if (nextNode != NULL) {
			nextNode->legalMoves[0] = NULL;
			nextNode->numLegalMoves--;
			shiftDownLegalMoves(nextNode, 1);
			node = nextNode;
		}
		else {
			break;
		}
	}
}

void freeInvFrames(int **invFrames) {
	for (int i = 0; i < 21; i++) {
		free(invFrames[i]);
	}
}

void freeLegalMove(struct BranchPath *node, int index) {
	freeNode(node->legalMoves[index]);
	node->legalMoves[index] = NULL;
	node->numLegalMoves--;
	node->next = NULL;
	
	// Shift down the rest of the legal moves
	shiftDownLegalMoves(node, index + 1);
}

int freeNode(struct BranchPath *node) {
	node->prev = NULL;
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
	
	return 0;
}

void fulfillChapter5(struct BranchPath *curNode, struct Recipe *recipeList) {
	// Create an outputs chart but with the Dried Bouquet collected
	// to ensure that the produced inventory can fulfill all remaining recipes
	int *tempOutputsFulfilled = malloc(sizeof(int) * 58);
	copyOutputsFulfilled(tempOutputsFulfilled, curNode->outputCreated);
	tempOutputsFulfilled[getIndexOfRecipe(getItem(Dried_Bouquet), recipeList)] = 1;
	
	// Create a temp inventory
	struct Item *tempInventory = malloc(sizeof(struct Item) * 20);
	copyInventory(tempInventory, curNode->inventory);
	
	// Determine how many spaces are available in the inventory for frame calculation purposes
	int viableItems = countItemsInInventory(tempInventory);
	
	// If the Mousse Cake is in the first 10 slots, change it to NULL
	int mousse_cake_index = indexOfItemInInventory(tempInventory, getItem(Mousse_Cake));
	if (mousse_cake_index < 10) {
		tempInventory[mousse_cake_index] = (struct Item) {-1, -1};
		viableItems--;
	}
	
	// Handle allocation of the first 2 CH5 items (Dried Bouquet and Coconut)
	if (countNullsInInventory(tempInventory, 0, 10) >= 2) {
		// The Dried Bouquet gets auto-placed in the 1st slot,
		// and everything else gets shifted down one slot to fill the first NULL
		shiftDownToFillNull(tempInventory);
		
		// The vacancy at the start of the inventory is now occupied with the new item
		tempInventory[0] = getItem(Dried_Bouquet);
		/*int tempSlotDB = 1;*/
		viableItems++;
		
		// The Coconut gets auto-placed in the 1st slot
		// and everything else gets shifted down one slot to fill the first NULL
		shiftDownToFillNull(tempInventory);
		
		// The vacancy at the start of the inventory is now occupied with the new item
		tempInventory[0] = getItem(Coconut);
		/*int tempSlotCO = 1;*/
		viableItems++;
		
		// Auto-placing takes 0 frames
		int tempFramesDB = 0;
		int tempFramesCO = 0;
		
		// Handle the allocation of the Coconut, Sort, Keel Mango, and Courage Shell
		handleChapter5Eval(curNode, tempInventory, recipeList, tempOutputsFulfilled, tempFramesDB, tempFramesCO, 0, 0);
	}
	else if (countNullsInInventory(tempInventory, 0, 10) == 1) {
		// The Dried Bouquet gets auto-placed in the 1st slot,
		// and everything else gets shifted down one to fill the first NULL
		shiftDownToFillNull(tempInventory);
		
		// The vacancy at the start of the inventory is now occupied with the new item
		tempInventory[0] = getItem(Dried_Bouquet);
		/*int tempSlotDB = 0;*/
		viableItems++;
		
		// Auto-placing takes zero frames
		int temp_frames_DB = 0;
		
		// The Coconut can only be placed in the first 10 slots
		// Dried Bouquet will always be in the first slot
		for (int temp_index_CO = 1; temp_index_CO < 10; temp_index_CO++) {
			// Don't waste time replacing the Dried Bouquet or Thunder Rage with the Coconut
			if (tempInventory[temp_index_CO].a_key == getItem(Thunder_Rage).a_key) {
				continue;
			}
			// Replace the item with the Coconut
			// All items above the replaced item float down one slot
			// and the Coconut is always placed in slot 1
			struct Item *co_temp_inventory = malloc(sizeof(struct Item) * 20);
			copyInventory(co_temp_inventory, tempInventory);
			co_temp_inventory[temp_index_CO] = (struct Item) {-1, -1};
			shiftDownToFillNull(co_temp_inventory);
			co_temp_inventory[0] = getItem(Coconut);
			
			// Calculate the number of frames needed to pick this slot for replacement
			int temp_frames_CO = TOSS_FRAMES + invFrames[viableItems-1][temp_index_CO];
			
			// Handle the allocation of the Coconut sort, Keel Mango, and Courage Shell
			handleChapter5Eval(curNode, co_temp_inventory, recipeList, tempOutputsFulfilled, temp_frames_DB, temp_frames_CO, 0, temp_index_CO);
			
			free(co_temp_inventory);
		}
	}
	else {
		// No nulls to utilize for Chapter 5 intermission
		// Both the DB and CO can only replace items in the first 10 slots
		// The remaining items always slide down to fill the vacancy
		// The DB will eventually end up in slot #2 and
		// the CO will eventually end up in slot #1
		for (int temp_index_DB = 0; temp_index_DB < 10; temp_index_DB++) {
			for (int temp_index_CO = temp_index_DB + 1; temp_index_CO < 10; temp_index_CO++) {
				// Replace the 1st chosen item with the Dried Bouquet
				struct Item *dbco_temp_inventory = malloc(sizeof(struct Item) * 20);
				copyInventory(dbco_temp_inventory, tempInventory);
				dbco_temp_inventory[temp_index_DB] = (struct Item) {-1, -1};
				shiftDownToFillNull(dbco_temp_inventory);
				dbco_temp_inventory[0] = getItem(Dried_Bouquet);
				
				// Replace the 2nd chosen item with the Coconut
				dbco_temp_inventory[temp_index_CO] = (struct Item) {-1, -1};
				shiftDownToFillNull(dbco_temp_inventory);
				dbco_temp_inventory[0] = getItem(Coconut);
				
				// Calculate the frames of these actions
				int temp_frames_DB = TOSS_FRAMES + invFrames[viableItems-1][temp_index_DB];
				int temp_frames_CO = TOSS_FRAMES + invFrames[viableItems-1][temp_index_CO];
				
				// Only evaulate the remainder of the CH5 intermission if the Thunder Rage is still present in the inventory
				if (indexOfItemInInventory(dbco_temp_inventory, getItem(Thunder_Rage)) > -1) {
					// Handle the allocation of the Coconut sort, Keel Mango, and Courage Shell
					handleChapter5Eval(curNode, dbco_temp_inventory, recipeList, tempOutputsFulfilled, temp_frames_DB, temp_frames_CO, temp_index_DB, temp_index_CO);
				}
				
				free(dbco_temp_inventory);
			}
		}
	}
	
	free(tempInventory);
	free(tempOutputsFulfilled);
	tempInventory = NULL;
	tempOutputsFulfilled = NULL;
}

void fulfillRecipes(struct BranchPath *curNode, struct Recipe *recipeList, int recipeIndex) {
	// Only want ingredient combos that can be fulfilled right now!
	struct Recipe recipe = recipeList[recipeIndex];
	struct ItemCombination *combo = recipe.combos;
	for (int comboIndex = 0; comboIndex < recipe.countCombos; comboIndex++) {
		int itemComboInInventoryBoolean = (combo[comboIndex].numItems == 1 &&
							itemInInventory(combo[comboIndex].item1.a_key, curNode->inventory)
						   ) ||
						   (combo[comboIndex].numItems == 2 &&
						   	itemInInventory(combo[comboIndex].item1.a_key, curNode->inventory) && 
						   	itemInInventory(combo[comboIndex].item2.a_key, curNode->inventory)
						   );
		if (!itemComboInInventoryBoolean) {
			continue;
		}
			
		// This is a recipe that can be fulfilled right now!
		
		// Copy the inventory
		struct Item *tempInventory = malloc(sizeof(struct Item) * 20);
		copyInventory(tempInventory, curNode->inventory);
		
		// Mark that this output has been fulfilled for viability determination
		int *tempOutputsFulfilled = malloc(sizeof(int) * NUM_RECIPES);
		copyOutputsFulfilled(tempOutputsFulfilled, curNode->outputCreated);
		tempOutputsFulfilled[recipeIndex] = 1;
		
		// Determine how many viable items are in the list (No NULLS or BLOCKED)
		int viableItems = countItemsInInventory(tempInventory);
		
		int ingredientLoc[2]= {0, 0};
		int ingredientOffset[2] = {0, 0};
		int tempFrames;
		struct MoveDescription useDescription;
		
		struct Cook *cookBase;
		
		if (combo[comboIndex].numItems == 1) {
			// This is a potentially viable recipe with 1 ingredient
			// Determine the location of the ingredient
			ingredientLoc[0] = indexOfItemInInventory(tempInventory, combo[comboIndex].item1);
			
			// Determine the offset by NULLs before the desired item, as NULLs do not appear during the inventory navigation
			ingredientOffset[0] = countNullsInInventory(tempInventory, 0, ingredientLoc[0]);
			
			// Modify the inventory if the ingredient was in the first 10 slots
			if (ingredientLoc[0] < 10) {
				tempInventory[ingredientLoc[0]].a_key = -1;
				tempInventory[ingredientLoc[0]].t_key = -1;
			}
			
			// Determine how many frames will be needed to select that item
			tempFrames = invFrames[viableItems][ingredientLoc[0]-ingredientOffset[0]];
				
			
			// TODO: Compartmentalize this in a separate fn
			// Describe what items were used
			
			generateCook(&useDescription, combo[comboIndex], recipe, ingredientLoc);
			cookBase = (struct Cook *) useDescription.data;
			generateFramesTaken(&useDescription, curNode, tempFrames);
		}
		else {
			// This is a potentially viable recipe with 2 ingredients
			//Baseline frames based on how many times we need to access the menu
			tempFrames = CHOOSE_2ND_INGREDIENT_FRAMES;
			
			// Determine the locations of both ingredients
			ingredientLoc[0] = indexOfItemInInventory(tempInventory, combo[comboIndex].item1);
			ingredientLoc[1] = indexOfItemInInventory(tempInventory, combo[comboIndex].item2);
			
			// Determine the offset by NULLs before the desired items, as NULLs do not appear during the inventory navigation
			ingredientOffset[0] = countNullsInInventory(tempInventory, 0, ingredientLoc[0]);
			ingredientOffset[1] = countNullsInInventory(tempInventory, 0, ingredientLoc[1]);
			
			// Determine which order of ingredients to take
			// The first picked item always vanishes from the list of ingredients when picking the 2nd ingredient
			// There are some configurations where it is 2 frames faster to pick the ingredients in the reverse order
			if (selectSecondItemFirst(curNode, combo[comboIndex], ingredientLoc, ingredientOffset, viableItems))
				// It's faster to select the 2nd item, so make it the priority and switch the order
				swapItems(ingredientLoc, ingredientOffset);
			
			// Calculate the number of frames needed to grab the first item
			tempFrames += invFrames[viableItems][ingredientLoc[0]-ingredientOffset[0]];
			
			// Set each inventory index to null if the item was in the first 10 slots
			if (ingredientLoc[0] < 10) {
				tempInventory[ingredientLoc[0]] = (struct Item) {-1, -1};
			}
			if (ingredientLoc[1] < 10) {
				tempInventory[ingredientLoc[1]] = (struct Item) {-1, -1};
			}
			
			// Determine the frames needed for the 2nd ingredient
			// First ingredient is always removed from the menu, so there is always 1 less viable item
			if (ingredientLoc[1] > ingredientLoc[0]) {
				// In this case, the 2nd ingredient has "moved up" one slot since the 1st ingredient vanishes
				tempFrames += invFrames[viableItems-1][ingredientLoc[1]-ingredientOffset[1]-1];
			}
			else {
				// In this case, the 2nd ingredient was found earlier on than the 1st ingredient, so no change to index
				tempFrames += invFrames[viableItems-1][ingredientLoc[1]-ingredientOffset[1]];
			}
			
			// Describe what items were used
			generateCook(&useDescription, combo[comboIndex], recipe, ingredientLoc);
			cookBase = (struct Cook *) useDescription.data;
			generateFramesTaken(&useDescription, curNode, tempFrames);
		}
		
		// Handle allocation of the output variable
		// Options vary by whether there are NULLs within the inventory
		if (countNullsInInventory(tempInventory, 0, 10) >= 1) {
			// If there are NULLs in the inventory, all items before the 1st NULL get shifted down 1 position
			shiftDownToFillNull(tempInventory);
		
			// The vacancy at the start of the inventory is now occupied with the new item
			tempInventory[0] = ((struct Cook *) useDescription.data)->output;
		
			// Check to see if this state is viable
			if(remainingOutputsCanBeFulfilled(tempInventory, tempOutputsFulfilled, recipeList) == 1) {
				finalizeLegalMove(curNode, tempFrames, useDescription, tempInventory, cookBase, tempOutputsFulfilled, Autoplace, (struct Item) {-1, -1}, -1);
			}
		}
		else {
			// There are no NULLs in the inventory. Something must be tossed
			// Total number of frames increased by forcing to toss something
			tempFrames += TOSS_FRAMES;
			useDescription.framesTaken += TOSS_FRAMES;
			useDescription.totalFramesTaken += TOSS_FRAMES;
			
			// Evaluate viability of tossing the output item itself
			if (remainingOutputsCanBeFulfilled(tempInventory, tempOutputsFulfilled, recipeList) == 1) {
				finalizeLegalMove(curNode, tempFrames, useDescription, tempInventory, cookBase, tempOutputsFulfilled, Toss, recipe.output, -1);
			}
			
			// Evaluate the viability of tossing all current inventory items
			// Assumed that it is impossible to toss and replace any items in the last 10 positions
			for (int tossedIndex = 0; tossedIndex < 10; tossedIndex++) {
				tryTossInventoryItem(curNode, tempInventory, useDescription, cookBase, tempOutputsFulfilled, tossedIndex, recipe.output, tempFrames, viableItems);
			}
		}
		
		free(cookBase);
		free(tempInventory);
		free(tempOutputsFulfilled);
		tempInventory = NULL;
		tempOutputsFulfilled = NULL;
	}
}

void generateCook(struct MoveDescription *description, struct ItemCombination combo, struct Recipe recipe, int *ingredientLoc) {
	struct Cook *cook = malloc(sizeof(struct Cook));
	description->action = Cook;
	cook->numItems = combo.numItems;
	cook->item1 = combo.item1;
	cook->itemIndex1 = ingredientLoc[0];
	cook->item2 = combo.item2;
	cook->itemIndex2 = ingredientLoc[1];
	cook->output = recipe.output;
	description->data = cook;
}

void generateFramesTaken(struct MoveDescription *description, struct BranchPath *node, int framesTaken) {
	description->framesTaken = framesTaken;
	description->totalFramesTaken = node->description.totalFramesTaken + framesTaken;
}

int getInsertionIndex(struct BranchPath *curNode, int frames) {
	if (curNode->legalMoves == NULL) {
		return 0;
	}
	int tempIndex = 0;
	while (tempIndex < curNode->numLegalMoves && curNode->legalMoves[tempIndex]->description.framesTaken < frames) {
		tempIndex++;
	}
	
	return tempIndex;
}

void handleChapter5EarlySortEndItems(struct BranchPath *node, struct Item *inventory, struct Recipe *recipeList, int *outputsFulfilled, int sort_frames, enum Action sort, int frames_DB, int frames_CO, int DB_place_index, int CO_place_index) {
	// Place the Keel Mango and Courage Shell
	for (int KM_place_index = 0; KM_place_index < 10; KM_place_index++) {
		for (int CS_place_index = KM_place_index + 1; CS_place_index < 10; CS_place_index++) {
			// Replace the 1st chosen item with the Keel Mango
			struct Item *kmcs_temp_inventory = malloc(sizeof(struct Item) * 20);
			copyInventory(kmcs_temp_inventory, inventory);
			kmcs_temp_inventory[KM_place_index] = (struct Item) {-1, -1};
			shiftDownToFillNull(kmcs_temp_inventory);
			kmcs_temp_inventory[0] = getItem(Keel_Mango);
			
			// Replace the 2nd chosen item with the Courage Shell
			kmcs_temp_inventory[CS_place_index] = (struct Item) {-1,  -1};
			shiftDownToFillNull(kmcs_temp_inventory);
			kmcs_temp_inventory[0] = getItem(Courage_Shell);
			
			// Ensure the Thunder Rage is still in the inventory
			int TR_use_index = indexOfItemInInventory(kmcs_temp_inventory, getItem(Thunder_Rage));
			if (TR_use_index == -1) {
				// Thunder Rage is no longer in the inventory.
				free(kmcs_temp_inventory);
				continue;
			}
			
			// The Thunder Rage is still in the inventory.
			// The next event is using the Thunder Rage item before resuming the 2nd session of recipe fulfillment
			if (TR_use_index < 10) {
				// Using the Thunder Rage will cause a NULL to appear in that slot
				kmcs_temp_inventory[TR_use_index] = (struct Item) {-1, -1};
			}
			
			// Calculate the frames of these actions
			int temp_frames_KM = TOSS_FRAMES + invFrames[20-countNullsInInventory(kmcs_temp_inventory, 10, 20) -1][KM_place_index];
			int temp_frames_CS = TOSS_FRAMES + invFrames[20-countNullsInInventory(kmcs_temp_inventory, 10, 20) -1][CS_place_index];
			int temp_frames_TR = 		    invFrames[20-countNullsInInventory(kmcs_temp_inventory, 10, 20) -1][TR_use_index];
			int temp_frame_sum = frames_DB + frames_CO + temp_frames_KM + temp_frames_CS + temp_frames_TR + sort_frames;
			
			// Determine if the remaining inventory is sufficient to fulfill all remaining recipes
			if (remainingOutputsCanBeFulfilled(kmcs_temp_inventory, outputsFulfilled, recipeList)) {
				finalizeChapter5Eval(node, kmcs_temp_inventory, sort, DB_place_index, CO_place_index, KM_place_index, CS_place_index, TR_use_index, temp_frame_sum, outputsFulfilled);
			}
			
			free(kmcs_temp_inventory);
		}
	}
}

void handleChapter5Eval(struct BranchPath *node, struct Item *inventory, struct Recipe *recipeList, int *outputsFulfilled, int frames_DB, int frames_CO, int DB_place_index, int CO_place_index) {
	// Evaluate sorting before the Keel Mango
	handleChapter5Sorts(node, inventory, recipeList, outputsFulfilled, frames_DB, frames_CO, -1, DB_place_index, CO_place_index, -1);
	
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
		struct Item *km_temp_inventory = malloc(sizeof(struct Item) * 20);
		copyInventory(km_temp_inventory, inventory);
		int temp_frames_KM;
		
		// Calculate the needed frames
		if (KM_upper_bound == 10) {
			temp_frames_KM = TOSS_FRAMES + invFrames[20-countNullsInInventory(inventory, 10, 20) -1][KM_place_index];
			km_temp_inventory[KM_place_index] = (struct Item) {-1, -1};
		}
		else {
			temp_frames_KM = 0;
		}
		
		// Update the inventory such that all items above the null are moved down one place
		shiftDownToFillNull(km_temp_inventory);

		// The vacancy at the start of the inventory is now occupied with the new item
		km_temp_inventory[0] = getItem(Keel_Mango);
		
		// Ensure the Coconut is in the remaining inventory
		if (indexOfItemInInventory(km_temp_inventory, getItem(Coconut)) == -1) {
			free(km_temp_inventory);
			continue;
		}
		
		// Perform all sorts
		handleChapter5Sorts(node, km_temp_inventory, recipeList, outputsFulfilled, frames_DB, frames_CO, temp_frames_KM, DB_place_index, CO_place_index, KM_place_index);
		free(km_temp_inventory);
	}		
}
	
void handleChapter5LateSortEndItems(struct BranchPath *node, struct  Item *inventory, struct Recipe *recipeList, int *outputsFulfilled, int sort_frames, enum Action sort, int frames_DB, int frames_CO, int frames_KM, int DB_place_index, int CO_place_index, int KM_place_index) {
	// Place the Courage Shell
	for (int CS_place_index = 0; CS_place_index < 10; CS_place_index++) {
		// Replace the chosen item with the Courage Shell
		struct Item *cs_temp_inventory = malloc(sizeof(struct Item) * 20);
		copyInventory(cs_temp_inventory, inventory);
		
		cs_temp_inventory[CS_place_index] = (struct Item) {-1, -1};
		shiftDownToFillNull(cs_temp_inventory);
		cs_temp_inventory[0] = getItem(Courage_Shell);
		
		// Ensure that the Thunder Rage is still in the inventory
		int TR_use_index = indexOfItemInInventory(cs_temp_inventory, getItem(Thunder_Rage));
		if (TR_use_index == -1) {
			free(cs_temp_inventory);
			continue;
		}
		
		// The next event is using the Thunder Rage item before resuming the 2nd session of recipe fulfillment
		if (TR_use_index < 10) {
			// Using the  Thunder Rage will cause a NULL to appear in that slot
			cs_temp_inventory[TR_use_index] = (struct Item) {-1, -1};
		}
		// Calculate the frames of these actions
		int temp_frames_CS = TOSS_FRAMES + invFrames[20-countNullsInInventory(cs_temp_inventory, 10, 20) -1][CS_place_index];
		int temp_frames_TR = 		    invFrames[20-countNullsInInventory(cs_temp_inventory, 10, 20) -1][TR_use_index];
		int temp_frame_sum = frames_DB + frames_CO + frames_KM + temp_frames_CS + temp_frames_TR + sort_frames;
		
		if (remainingOutputsCanBeFulfilled(cs_temp_inventory, outputsFulfilled, recipeList)) {
			finalizeChapter5Eval(node, cs_temp_inventory, sort, DB_place_index, CO_place_index, KM_place_index, CS_place_index, TR_use_index, temp_frame_sum, outputsFulfilled);
		}
			
		free(cs_temp_inventory);
	}
}

void handleChapter5Sorts(struct BranchPath *node, struct Item *inventory, struct Recipe *recipeList, int *outputsFulfilled, int frames_DB, int frames_CO, int frames_KM, int DB_place_index, int CO_place_index, int KM_place_index) {
	for (enum Action action = Sort_Alpha_Asc; action <= Sort_Type_Des; action++) {
		struct Item *sorted_inventory = getSortedInventory(inventory, action);
		
		// Only bother with further evaluation if the sort placed the Coconut in the latter half of the inventory
		// because the Coconut is needed for duplication
		if (indexOfItemInInventory(sorted_inventory, getItem(Coconut)) >= 10) {
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
					// Something went wrong if we reach this point...
					// action should be some type of sort
					exit(-2);
			}
			
			if (frames_KM == -1) {
				handleChapter5EarlySortEndItems(node, sorted_inventory, recipeList, outputsFulfilled, sortFrames, action, frames_DB, frames_CO, DB_place_index, CO_place_index);
			}
			else {
				handleChapter5LateSortEndItems(node, sorted_inventory, recipeList, outputsFulfilled, sortFrames, action, frames_DB, frames_CO, frames_KM, DB_place_index, CO_place_index, KM_place_index);
			}
		}
		free(sorted_inventory);
	}
}

void handleSelectAndRandom(struct BranchPath *curNode, int select, int randomise) {
	// Somewhat random process of picking the quicker moves to recurse down
	// Arbitrarily remove the first listed move with a given probability
	// TODO: Modify select to do the following
		// Take the frame count of the fastest node
		// Divide by the sum of all frame counts
		// Generates a weight between 0 and 1
		// Determine what interval it falls in and its corresponding node
		// Softmax function
	if (select && curNode->moves < 55) {
		while (curNode->numLegalMoves > 1 && rand() % 100 < 50) {
			freeLegalMove(curNode, 0);
		}
	}
	// When not doing the select methodology, and opting for randomize
	// just shuffle the entire list of legal moves and pick the new first item
	else if (randomise) {
		shuffleLegalMoves(curNode);
	}
}

void handleSorts(struct BranchPath *curNode) {
	// Count the number of sorts for capping purposes
	// Limit the number of sorts allowed in a roadmap
	// NOTE: Reduced from 10 to 6 based off of the current set of fastest roadmaps
	if (countTotalSorts(curNode) < 6) {
		// Perform the 4 different sorts
		for (enum Action sort = Sort_Alpha_Asc; sort <= Sort_Type_Des; sort++) {
			struct Item *sorted_inventory = getSortedInventory(curNode->inventory,sort);
		
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
				int *copyOfOutputsFulfilled = malloc(sizeof(int) * 58);
				copyOutputsFulfilled(copyOfOutputsFulfilled, curNode->outputCreated);
				
				// Create the legalMove node
				struct BranchPath *newLegalMove = createLegalMove(curNode, sorted_inventory, description, copyOfOutputsFulfilled);
				
				// Insert this new move into the current node's legalMove array
				insertIntoLegalMoves(curNode->numLegalMoves, newLegalMove, curNode);
				assert(curNode->legalMoves[curNode->numLegalMoves-1] != NULL);
			}
			else {
				free(sorted_inventory);
				sorted_inventory = NULL;
			}
		}
	}
}

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

void insertIntoLegalMoves(int insertIndex, struct BranchPath *newLegalMove, struct BranchPath *curNode) {
	// Reallocate the legalMove array to make room for a new legal move
	curNode->legalMoves = realloc(curNode->legalMoves, sizeof(struct BranchPath *) * (curNode->numLegalMoves + 1));
	
	// Shift all legal moves further down the array to make room for a new legalMove
	for (int i = curNode->numLegalMoves - 1; i >= insertIndex; i--) {
		curNode->legalMoves[i+1] = curNode->legalMoves[i];
	}
	
	// Place newLegalMove in index insertIndex
	curNode->legalMoves[insertIndex] = newLegalMove;
	
	// Increase numLegalMoves
	curNode->numLegalMoves++;
	
	return;
}

void popAllButFirstLegalMove(struct BranchPath *node) {
	for (int i = 1; i < node->numLegalMoves; i++) {
		freeLegalMove(node, i);
		i--;
	}
	
	assert(node->numLegalMoves <= 1);
	
	return;
}

void printCh5Data(struct BranchPath *curNode, struct MoveDescription desc, FILE *fp) {
	struct CH5 *ch5Data = desc.data;
	fprintf(fp, "Ch.5 Break: Replace #%d for DB, Replace #%d for CO, Sort (", ch5Data->indexDriedBouquet, ch5Data->indexCoconut);
	
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
	
	fprintf(fp, "Replace #%d for KM, Replace #%d for CS, Use TR in #%d\t", ch5Data->indexKeelMango, ch5Data->indexCourageShell, ch5Data->indexThunderRage);
}

void printCookData(struct BranchPath *curNode, struct MoveDescription desc, FILE *fp) {
	struct Cook *cookData = desc.data;
	fprintf(fp, "Use [%s] in slot %d ", getItemName(cookData->item1.a_key), cookData->itemIndex1 + 1);
	
	if (cookData->numItems == 2) {
		fprintf(fp, "and [%s] in slot %d ", getItemName(cookData->item2.a_key), cookData->itemIndex2 + 1);
	}
	fputs("to make ", fp);
	
	if (cookData->handleOutput == Toss) {
		fputs("(and toss) ", fp);
	}
	else if (cookData->handleOutput == Autoplace) {
		fputs("(and auto-place) ", fp);
	}
	fprintf(fp, "<%s>", getItemName(cookData->output.a_key));
	
	if (cookData->handleOutput == TossOther) {
		fprintf(fp, ", toss [%s] in slot %d", getItemName(cookData->toss.a_key), cookData->indexToss + 1);
	}
	if (curNode->numOutputsCreated == NUM_RECIPES && ((struct Cook *) curNode->description.data)->handleOutput == Autoplace) {
		fputs(" (No-Toss 5 Frame Penalty for Jump Storage)", fp);
	}
	else if (curNode->numOutputsCreated == NUM_RECIPES) {
		fputs(" (Jump Storage on Tossed Item)", fp);
	}
	fputs("\t", fp);
}

void printFileHeader(FILE *fp) {
	fputs("Description\tFrames Taken\tTotal Frames", fp);
	struct Recipe *recipes = getRecipeList();
	for (int i = 0; i < 20; i++) {
		fprintf(fp, "\tSlot #%d", i+1);
	}
	for (int i = 0; i < NUM_RECIPES; i++) {
		fprintf(fp, "\t%s", getItemName(recipes[i].output.a_key));
	}
	fprintf(fp, "\n");
	recipeLog(5, "Calculator", "File", "Write", "Header for new output written");
}

void printInventoryData(struct BranchPath *curNode, FILE *fp) {
	for (int i = 0; i < 20; i++) {
		if (curNode->inventory[i].a_key == -1) {
			if (i<9) {
				fprintf(fp, "NULL\t");
			}
			else if (i==0) { 
				fprintf(fp, "NULL\t");
			}
			else {
				fprintf(fp, "BLOCKED\t");
			}
			continue;
		}
			
		fprintf(fp, "%s\t", getItemName(curNode->inventory[i].a_key));
	}
}

void printOutputsCreated(struct BranchPath *curNode, FILE *fp) {
	for (int i = 0; i < NUM_RECIPES; i++) {
		if (curNode->outputCreated[i] == 1) {
			fprintf(fp, "True\t");
		}
		else {
			fprintf(fp, "False\t");
		}
	}
}

int printResults(char *filename, struct BranchPath *path) {
	FILE *fp = fopen(filename, "w");
	
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

// TODO: This just doesn't look right...
int selectSecondItemFirst(struct BranchPath *node, struct ItemCombination combo, int *ingredientLoc, int *ingredientOffset, int viableItems) {
	return ((combo.numItems == 1 && itemInInventory(combo.item1.a_key, node->inventory)) ||
		(combo.numItems == 2 && itemInInventory(combo.item1.a_key, node->inventory) && itemInInventory(combo.item2.a_key, node->inventory)));
}

// startIndex is the first non-null index
void shiftDownLegalMoves(struct BranchPath *node, int startIndex) {
	for (int i = startIndex - 1; i < node->numLegalMoves; i++) {
		node->legalMoves[i] = node->legalMoves[i+1];
	}
	// Null where the last entry was before shifting
	node->legalMoves[node->numLegalMoves] = NULL;
}

void shiftDownToFillNull(struct Item *inventory) {
	// First find the index of the first null
	int firstNull = -1;
	for (int i = 0; i < 20; i++) {
		if (inventory[i].a_key == -1) {
			firstNull = i;
			break;
		}
	}
	
	// Now shift all items further down in the inventory to make room for a new item
	for (int i = firstNull; i > 0; i--) {
		inventory[i] = inventory[i - 1];
	}
	
	return;
}

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

void tryTossInventoryItem(struct BranchPath *curNode, struct Item *tempInventory, struct MoveDescription useDescription, struct Cook *cookBase, int *tempOutputsFulfilled, int tossedIndex, struct Item output, int tempFrames, int viableItems) {
	// Make a copy of the tempInventory with the replaced item
	struct Item *replacedInventory = malloc(sizeof(struct Item) * 20);
	copyInventory(replacedInventory, tempInventory);
	struct Item tossedItem = tempInventory[tossedIndex];
	
	// All items before the selected removal item get moved down 1 position
	replacedInventory[tossedIndex] = (struct Item) {-1, -1};
	shiftDownToFillNull(replacedInventory);
	
	// The vacancy at the start of the inventory is now occupied with the new item
	replacedInventory[0] = output;
	
	if (remainingOutputsCanBeFulfilled(replacedInventory, tempOutputsFulfilled, recipeList) == 0) {
		free(replacedInventory);
		replacedInventory = NULL;
		return;
	}
	
	// Calculate the additional tossed frames.
	int replacedFrames = tempFrames + invFrames[viableItems][tossedIndex+1];
	useDescription.framesTaken += invFrames[viableItems][tossedIndex+1];
	useDescription.totalFramesTaken += invFrames[viableItems][tossedIndex+1];
	
	finalizeLegalMove(curNode, replacedFrames, useDescription, replacedInventory, cookBase, tempOutputsFulfilled, TossOther, tossedItem, tossedIndex);
	// Inventory is copied within finalizeLegalMove, so free replacedInventory
	free(replacedInventory);
	
	return;
}

int alpha_sort(const void *elem1, const void *elem2) {
	struct Item item1 = *((struct Item*)elem1);
	struct Item item2 = *((struct Item*)elem2);
	// Handle case of null slots
	if (item1.a_key == -1) {return -1;}
	if (item2.a_key == -1) {return 1;}
	if (item1.a_key < item2.a_key) {return -1;}
	if (item1.a_key > item2.a_key) {return 1;}
	return 0;
}

int alpha_sort_reverse(const void *elem1, const void *elem2) {
	struct Item item1 = *((struct Item*)elem1);
	struct Item item2 = *((struct Item*)elem2);
	// Handle case of null slots
	if (item1.a_key == -1) {return -1;}
	if (item2.a_key == -1) {return 1;}
	if (item1.a_key < item2.a_key) {return 1;}
	if (item1.a_key > item2.a_key) {return -1;}
	return 0;
}

int type_sort(const void *elem1, const void *elem2) {
	struct Item item1 = *((struct Item*)elem1);
	struct Item item2 = *((struct Item*)elem2);
	// Handle case of null slots
	if (item1.t_key == -1) {return -1;}
	if (item2.t_key == -1) {return 1;}
	if (item1.t_key < item2.t_key) {return -1;}
	if (item1.t_key > item2.t_key) {return 1;}
	return 0;
}

int type_sort_reverse(const void *elem1, const void *elem2) {
	struct Item item1 = *((struct Item*)elem1);
	struct Item item2 = *((struct Item*)elem2);
	// Handle case of null slots
	if (item1.t_key == -1) {return -1;}
	if (item2.t_key == -1) {return 1;}
	if (item1.t_key < item2.t_key) {return 1;}
	if (item1.t_key > item2.t_key) {return -1;}
	return 0;
}

struct Item *getSortedInventory(struct Item *inventory, enum Action sort) {
	// We first need to copy the inventory to a new array
	struct Item *sorted_inventory = malloc(sizeof(struct Item) * 20);
	for (int i = 0; i < 20; i++) {
		sorted_inventory[i] = inventory[i];
	}

	// Use qsort and execute sort function depending on sort type
	switch(sort) {
		case Sort_Alpha_Asc :
			qsort(sorted_inventory, 20, sizeof(struct Item), alpha_sort);
			return sorted_inventory;
		case Sort_Alpha_Des :
			qsort(sorted_inventory, 20, sizeof(struct Item), alpha_sort_reverse);
			return sorted_inventory;
		case Sort_Type_Asc :
			qsort(sorted_inventory, 20, sizeof(struct Item), type_sort);
			return sorted_inventory;
		case Sort_Type_Des :
			qsort(sorted_inventory, 20, sizeof(struct Item), type_sort_reverse);
			return sorted_inventory;
		default :
			return NULL;
	}
}

struct Result calculateOrder(struct Job job) {
	config_t *config = getConfig();
	
	int randomise;
	config_lookup_int(config, "randomise", &randomise);
	
	int select;
	config_lookup_int(config, "select", &select);
	invFrames = getInventoryFrames();
	recipeList = getRecipeList();
	
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
	
	// How many times a worker is evaluating a new random branch
	int total_dives = 0;
	
	// Deepest node at any particular point
	struct BranchPath *curNode = NULL;
	struct BranchPath *root;
	
	//Start main loop
	while (1) {
		int stepIndex = 0;
		int iterationCount = 0;
		int currentFrameRecord = getFastestRecordOnBlob();

		// Create root of tree path
		curNode = initializeRoot(job);
		root = curNode; // Necessary when printing results starting from root
		
		total_dives++;
		char temp1[30];
		char temp2[30];
		sprintf(temp1, "Call %d", job.callNumber);
		sprintf(temp2, "Searching New Branch %d", total_dives);
		recipeLog(3, "Calculator", "Info", temp1, temp2);
		
		clock_t start, end;
		double cpu_time_used;
		
		start = clock();
		
		// Handle the case where the user may choose to disable both randomise and select,
		// in which case they would always iterate down the same path, even if we reset every n iterations
		// Set to 1,000,000 iterations before resetting at the root
		int configBool = (iterationCount < 100000 || (select == 0 && randomise == 0));
		
		// Periodic check for current version
		if (total_dives % 10 == 0) {
			int update = checkForUpdates(job.local_ver);
			if (update == -1) {
				printf("Please check your internet connection in order to continue.\n");
				printf("Otherwise, we can't submit compelted roadmaps to the server!\n");
				exit(-1);
			}
			else if (update == 1) {
				printf("Please visit https://github.com/SevenChords/CipesAtHome/releases to download the newest version of this program!");
				exit(-1);
			}
		}
		// Start iteration loop
		while (configBool) {
			// In the rare occassion that the root node runs out of legal moves due to "select",
			// exit out of the while loop to restart
			if (curNode == NULL) {
				break;
			}
			
			// Check for bad states to immediately retreat from
			// The Thunder Rage must remain in the inventory until the Chapter 5 intermission
			if (!(curNode->outputCreated[57]) && !(itemInInventory(Thunder_Rage, curNode->inventory))) {
				// Regardless of record status, it's time to go back up and find new endstates
				// Wipe away the current state
				curNode = curNode->prev;
				curNode->next = NULL;
				freeLegalMove(curNode, 0);
				stepIndex--;
				continue;
			}
			
			// Check for end condition (57 recipes + the Chapter 5 intermission)
			if(curNode->numOutputsCreated == NUM_RECIPES) {
				// All recipes have been fulfilled!
				// Check that the total time taken is strictly less than the current observed record.
				// Apply a frame penalty if the final move did not toss an item.
				applyJumpStorageFramePenalty(curNode);
				
				currentFrameRecord = getFastestRecordOnBlob();
				if (curNode->description.totalFramesTaken < currentFrameRecord) {
					// A finished roadmap has been generated
					// TODO: Implement optimizeRoadmap

					// Add log
				
					// TODO: Replace left side of boolean expression with rearranged_frame_record after implementing optimizeRoadmap
					if (/*curNode->description.totalFramesTaken < *(job.current_frame_record)*/ 1) {
						*(job.current_frame_record) = curNode->description.totalFramesTaken;
						char *filename = malloc(sizeof(char) * 17);
						sprintf(filename, "results/%d.txt", *(job.current_frame_record));
						printResults(filename, root);
						char tmp[50];
						sprintf(tmp, "Congrats! New fastest roadmap found! %d frames", curNode->description.totalFramesTaken);
						recipeLog(6, "Calculator", "Info", "Roadmap", tmp);
						free(filename);
						freeInvFrames(invFrames);
						invFrames = NULL;
						struct Result result = (struct Result) {*(job.current_frame_record), job.callNumber};
						return result;
					}
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
				
				struct Recipe *recipeList = job.recipeList;
				// Iterate through all recipe ingredient combos
					
				for (int recipeIndex = 0; recipeIndex < upperOutputLimit; recipeIndex++) {
					// Only want recipes that haven't been fulfilled
					if (curNode->outputCreated[recipeIndex] == 1) {
						continue;
					}
					
					// Dried Bouquet (Recipe index 56) represents the Chapter 5 intermission
					// Don't actually use the specified recipe, as it is handled later
					if (recipeIndex == getIndexOfRecipe(getItem(Dried_Bouquet), recipeList)) {
						continue;
					}
					
					fulfillRecipes(curNode, recipeList, recipeIndex);
				}
				
				// Special handling of the 58th item, which is representative of the Chapter 5 intermission
				
				// The first item is trading the Mousse Cake and 2 Hot Dogs for a Dried Bouquet
				// Inventory must contain both items, and Hot Dog must be in a slot such that it can be duplicated
				if (!curNode->outputCreated[getIndexOfRecipe(getItem(Dried_Bouquet), recipeList)] && itemInInventory(Mousse_Cake, curNode->inventory) &&
				    indexOfItemInInventory(curNode->inventory, getItem(Hot_Dog)) >= 10) {
					fulfillChapter5(curNode, recipeList);
				}
				
				// Special handling of inventory sorting
				// Avoid redundant searches
				if (curNode->description.action == Begin || curNode->description.action == Cook || curNode->description.action == Ch5) {
					handleSorts(curNode);
				}
				
				// All legal moves evaluated and listed!
				
				// Filter out all legal moves that would exceed the current frame limit
				filterLegalMovesExceedFrameLimit(curNode, currentFrameRecord + BUFFER_SEARCH_FRAMES);
				
				if (curNode->moves == 0) {
					// Filter out all legal moves that use 2 ingredients in the very first legal move
					filterOut2Ingredients(curNode);
				}
				
				// Special filtering if we only had one recipe left to fulfill
				if (curNode->numOutputsCreated == 57 && curNode->numLegalMoves > 0 && curNode->legalMoves[0]->description.action == Cook) {
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
				// Filter out all legal moves that would exceed the current frame limit
				filterLegalMovesExceedFrameLimit(curNode, currentFrameRecord);
				
				if (curNode->numLegalMoves == 0) {
					// No legal moves are left to evaluate, go back up...
					// Wipe away the current node
					
					curNode = curNode->prev;
					freeLegalMove(curNode, 0);
					curNode->next = NULL;
					stepIndex--;
					continue;
				}
				
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
				}
			}
		}
		
		// We have passed the iteration maximum
		// Print the incomplete roadmap
		char filename[30];
		sprintf(filename, "Invalid roadmap %d", total_dives);
		// Free everything before reinitializing
		freeAllNodes(curNode);
		
		end = clock();
		cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
		
		printf("100K iterations in %fs\n", cpu_time_used);
		
		/*
		// For profiling
		if (total_dives == 10) {
			exit(1);
		}
		*/
	}
}
