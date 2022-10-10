#ifndef RECIPES_H
#define RECIPES_H

#include "types.h"

#include "absl/base/attributes.h"

ABSL_MUST_USE_RESULT  // Output is newly allocated and needs to be freed at some point
struct Recipe* getRecipeList();

int	getOutputIndex(Type_Sort item);
ItemCombination	parseCombo(int itemCount, Type_Sort item1, Type_Sort item2);
void placeInventoryInMakeableItems(int* makeableItems, Inventory inventory);
int	stateOK(Inventory inventory, const outputCreatedArray_t outputsCreated, Recipe* recipeList);

#endif
