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
