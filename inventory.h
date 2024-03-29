#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdint.h>
#include "types.h"

int	itemComboInInventory(ItemCombination combo, Inventory inventory);
int	compareInventories(Inventory inv1, Inventory inv2); // Returns 1 if inventories are the same, otherwise 0
int	itemInDependentIndices(int index, int* dependentIndices, int numDependentIndices);
int indexOfItemInInventory(Inventory inventory, Type_Sort item); // Returns index of item in the inventory. -1 if not found
Alpha_Sort getAlphaKey(Type_Sort item);
char *getItemName(Type_Sort t_key); // Return the string name for a particular item
int **getInventoryFrames();

Inventory getStartingInventory(); // Return array of Item structs for the start of cooking recipes
Inventory replaceItem(Inventory inventory, int index, Type_Sort item);
Inventory addItem(Inventory inventory, Type_Sort item);
Inventory removeItem(Inventory inventory, int index);

#endif
