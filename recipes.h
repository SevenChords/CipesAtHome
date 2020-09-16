#ifndef RECIPES_H
#define RECIPES_H

#include "inventory.h"

struct ItemCombination {
	int numItems; // If set to 1, ignore item2
	struct Item item1;
	struct Item item2;
};

struct Recipe {
	struct Item output;
	int countCombos;
	struct ItemCombination *combos; // Where there are countCombos different ways to cook output
};

struct ItemCombination parseCombo(int itemCount, struct Item item1, struct Item item2);
struct Recipe *getRecipeList();

void placeInventoryInMakeableItems(int *makeableItems, struct Item *inventory);

void copyDependentIndices(int *newDependentIndices, int *dependentIndices);

// Determine if the recipe items can still be fulfilled
int checkRecipe(struct ItemCombination combo, int *makeableItems, int *outputsCreated, int *dependentIndices, struct Recipe *recipeList);

// Determine if the remaining outputs can be fulfilled with the current given inventory
int stateOK(struct Item *inventory, int *outputsCreated, struct Recipe *recipeList);

// Returns the index in the recipeList for the given recipe output "item". -1 if not a recipe output
int getIndexOfRecipe(struct Item item);

#endif
