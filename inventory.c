#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inventory.h"
#include "logger.h"

#define INVENTORY_MAX_SIZE 21
#define NUM_ITEMS 107

typedef enum Alpha_Sort Alpha_Sort;
typedef enum Type_Sort Type_Sort;
typedef struct ItemName ItemName;

/*====================NOTES====================
- Items are interpreted based on the Alpha_Sort enumerator value. A function can later be written to conver this to a string.
  - This negates the need to allocate memory to handle strings, which will reduce issues in the future.
  - Using the alpha key as the main identifier also negates the need to have a function to obtain the alpha key along with the type key.
- Each item in the inventory is represented with a struct which contains both its alphabetical and type sort keys, again using
  the alphabetical key as the main item identifier.
====================     ====================*/

Alpha_Sort items[] = {
	Mushroom_a,
	Super_Shroom_a,
	Ultra_Shroom_a,
	Life_Shroom_a,
	Slow_Shroom_a,
	Dried_Shroom_a,
	Honey_Syrup_a,
	Maple_Syrup_a,
	Jammin_Jelly_a,
	Gradual_Syrup_a,
	Tasty_Tonic_a,
	POW_Block_a,
	Fire_Flower_a,
	Ice_Storm_a,
	Earth_Quake_a,
	Thunder_Bolt_a,
	Thunder_Rage_a,
	Shooting_Star_a,
	Volt_Shroom_a,
	Repel_Cape_a,
	Boos_Sheet_a,
	Ruin_Powder_a,
	Sleepy_Sheep_a,
	Dizzy_Dial_a,
	Stopwatch_a,
	Power_Punch_a,
	Mini_Mr_Mini_a,
	Courage_Shell_a,
	Mr_Softener_a,
	HP_Drain_a,
	Point_Swap_a,
	Fright_Mask_a,
	Mystery_a,
	Inn_Coupon_a,
	Gold_Bar_a,
	Gold_Bar_x_3_a,
	Whacka_Bump_a,
	Hot_Dog_a,
	Coconut_a,
	Dried_Bouquet_a,
	Mystic_Egg_a,
	Golden_Leaf_a,
	Keel_Mango_a,
	Fresh_Pasta_a,
	Cake_Mix_a,
	Hot_Sauce_a,
	Turtley_Leaf_a,
	Horsetail_a,
	Peachy_Peach_a,
	Spite_Pouch_a,
	Shroom_Fry_a,
	Shroom_Roast_a,
	Shroom_Steak_a,
	Honey_Shroom_a,
	Maple_Shroom_a,
	Jelly_Shroom_a,
	Honey_Super_a,
	Maple_Super_a,
	Jelly_Super_a,
	Honey_Ultra_a,
	Maple_Ultra_a,
	Jelly_Ultra_a,
	Zess_Dinner_a,
	Zess_Special_a,
	Zess_Deluxe_a,
	Spaghetti_a,
	Koopasta_a,
	Spicy_Pasta_a,
	Ink_Pasta_a,
	Spicy_Soup_a,
	Fried_Egg_a,
	Omelette_Meal_a,
	Koopa_Bun_a,
	Healthy_Salad_a,
	Meteor_Meal_a,
	Couples_Cake_a,
	Mousse_Cake_a,
	Shroom_Cake_a,
	Choco_Cake_a,
	Heartful_Cake_a,
	Fruit_Parfait_a,
	Mango_Delight_a,
	Love_Pudding_a,
	Zess_Cookie_a,
	Shroom_Crepe_a,
	Peach_Tart_a,
	Koopa_Tea_a,
	Zess_Tea_a,
	Shroom_Broth_a,
	Fresh_Juice_a,
	Inky_Sauce_a,
	Icicle_Pop_a,
	Zess_Frappe_a,
	Snow_Bunny_a,
	Coco_Candy_a,
	Honey_Candy_a,
	Jelly_Candy_a,
	Electro_Pop_a,
	Fire_Pop_a,
	Space_Food_a,
	Poison_Shroom_a,
	Trial_Stew_a,
	Courage_Meal_a,
	Coconut_Bomb_a,
	Egg_Bomb_a,
	Zess_Dynamite_a,
	Mistake_a
};

char *itemNames[NUM_ITEMS] = {
	"Mushroom",
	"Super_Shroom",
	"Ultra_Shroom",
	"Life_Shroom",
	"Slow_Shroom",
	"Dried_Shroom",
	"Honey_Syrup",
	"Maple_Syrup",
	"Jammin_Jelly",
	"Gradual_Syrup",
	"Tasty_Tonic",
	"POW_Block",
	"Fire_Flower",
	"Ice_Storm",
	"Earth_Quake",
	"Thunder_Bolt",
	"Thunder_Rage",
	"Shooting_Star",
	"Volt_Shroom",
	"Repel_Cape",
	"Boos_Sheet",
	"Ruin_Powder",
	"Sleepy_Sheep",
	"Dizzy_Dial",
	"Stopwatch",
	"Power_Punch",
	"Mini_Mr_Mini",
	"Courage_Shell",
	"Mr_Softener",
	"HP_Drain",
	"Point_Swap",
	"Fright_Mask",
	"Mystery",
	"Inn_Coupon",
	"Gold_Bar",
	"Gold_Bar_x_3",
	"Whacka_Bump",
	"Hot_Dog",
	"Coconut",
	"Dried_Bouquet",
	"Mystic_Egg",
	"Golden_Leaf",
	"Keel_Mango",
	"Fresh_Pasta",
	"Cake_Mix",
	"Hot_Sauce",
	"Turtley_Leaf",
	"Horsetail",
	"Peachy_Peach",
	"Spite_Pouch",
	"Shroom_Fry",
	"Shroom_Roast",
	"Shroom_Steak",
	"Honey_Shroom",
	"Maple_Shroom",
	"Jelly_Shroom",
	"Honey_Super",
	"Maple_Super",
	"Jelly_Super",
	"Honey_Ultra",
	"Maple_Ultra",
	"Jelly_Ultra",
	"Zess_Dinner",
	"Zess_Special",
	"Zess_Deluxe",
	"Spaghetti",
	"Koopasta",
	"Spicy_Pasta",
	"Ink_Pasta",
	"Spicy_Soup",
	"Fried_Egg",
	"Omelette_Meal",
	"Koopa_Bun",
	"Healthy_Salad",
	"Meteor_Meal",
	"Couples_Cake",
	"Mousse_Cake",
	"Shroom_Cake",
	"Choco_Cake",
	"Heartful_Cake",
	"Fruit_Parfait",
	"Mango_Delight",
	"Love_Pudding",
	"Zess_Cookie",
	"Shroom_Crepe",
	"Peach_Tart",
	"Koopa_Tea",
	"Zess_Tea",
	"Shroom_Broth",
	"Fresh_Juice",
	"Inky_Sauce",
	"Icicle_Pop",
	"Zess_Frappe",
	"Snow_Bunny",
	"Coco_Candy",
	"Honey_Candy",
	"Jelly_Candy",
	"Electro_Pop",
	"Fire_Pop",
	"Space_Food",
	"Poison_Shroom",
	"Trial_Stew",
	"Courage_Meal",
	"Coconut_Bomb",
	"Egg_Bomb",
	"Zess_Dynamite",
	"Mistake"
};

/*-------------------------------------------------------------------
 * Function 	: getAlphaKey
 * Inputs	: struct Type_Sort item
 * Outputs	: enum Alpha_Sort alpha_key
 *
 * Use items array to get alpha key for a given item when we need to sort.
 -------------------------------------------------------------------*/
enum Alpha_Sort getAlphaKey(enum Type_Sort item) {
	return items[item];
}

/*-------------------------------------------------------------------
 * Function 	: compareInventories
 * Inputs	: struct Type_Sort *inv1
 * 		  struct Type_Sort *inv2
 * Outputs	: 0 - inventories are different
 *		  1 - inventories are identical
 *
 * Compare two inventories for any differences. This is used to determine
 * if sorts changed the inventory at all.
 -------------------------------------------------------------------*/
int compareInventories(enum Type_Sort *inv1, enum Type_Sort *inv2) {
	return memcmp((void*)inv1, (void*)inv2, sizeof(enum Type_Sort) * 20) != 0;
}

/*-------------------------------------------------------------------
 * Function 	: itemComboInInventory
 * Inputs	: struct ItemCombination	combo
 *		  enum Type_Sort		*inventory
 * Outputs	: int (0 or 1)
 *
 * Determine whether the items in a recipe combination exist in the
 * inventory. In the case of a 1 item recipe, only check for the one item.
 -------------------------------------------------------------------*/
int itemComboInInventory(struct ItemCombination combo, enum Type_Sort *inventory) {
	if (combo.numItems == 1) {
		return indexOfItemInInventory(inventory, combo.item1) > -1;
	}
	
	return	indexOfItemInInventory(inventory, combo.item1) > -1 &&
		indexOfItemInInventory(inventory, combo.item2) > -1;
}

/*-------------------------------------------------------------------
 * Function 	: itemInDependentIndices
 * Inputs	: int index
 *		  int *dependentIndices
 *		  int numDependentIndices
 * Outputs	: 0 - item is not in dependentIndices
 *		  1 - item is is dependentIndices
 *
 * Compare two inventories for any differences. This is used to determine
 * if sorts changed the inventory at all.
 -------------------------------------------------------------------*/
int itemInDependentIndices(int index, int *dependentIndices, int numDependentIndices) {
	for (int i = 0; i < numDependentIndices; i++) {
		if (dependentIndices[i] == index)
			return 1;
	}
	return 0;
}

/*-------------------------------------------------------------------
 * Function 	: countNullsInInventory
 * Inputs	: struct Type_Sort *inventory
 *		  int		    minIndex
 *		  int		    maxIndex
 * Outputs	: The number of nulls in the inventory
 *
 * Traverse through the inventory and count the number of blank entries
 * in the inventory.
 -------------------------------------------------------------------*/
int countNullsInInventory(enum Type_Sort *inventory, int minIndex, int maxIndex) {
	int count = 0;
	for (int i = minIndex; i < maxIndex; i++) {
		if (inventory[i] == -1)
			count++;
	}
	return count;
}

/*-------------------------------------------------------------------
 * Function 	: indexOfItemInInventory
 * Inputs	: struct Type_Sort *inventory
 *		  struct Type_Sort item
 * Outputs	: index of item in inventory
 *
 * Traverse through the inventory and find the location of the provided
 * item. If not found, return -1.
 -------------------------------------------------------------------*/
int indexOfItemInInventory(enum Type_Sort *inventory, enum Type_Sort item) {
	for (int i = 0; i < 20; i++) {
		if (inventory[i] == item)
			return i;
	}
	return -1;
}

/*-------------------------------------------------------------------
 * Function 	: countItemsInInventory
 * Inputs	: enum Type_Sort 	*inventory
 * Outputs	: number of item in inventory
 *
 * Traverse through the inventory and count the number of valid items
 * in the inventory. This excludes blank (NULL) entries.
 -------------------------------------------------------------------*/
int countItemsInInventory(enum Type_Sort *inventory) {
	int count = 0;
	for (int i = 0; i < 20; i++) {
		if (inventory[i] != -1) {
			count++;
		}
	}
	
	return count;
}
			
/*-------------------------------------------------------------------
 * Function 	: copyInventory
 * Inputs	: enum Type_Sort *oldInventory
 * Outputs	: enum Type_Sort *newInventory
 *
 * Perform a simple memcpy to duplicate an old inventory to a newly
 * allocated inventory.
 -------------------------------------------------------------------*/
enum Type_Sort *copyInventory(enum Type_Sort* oldInventory) {
	enum Type_Sort *newInventory = malloc(sizeof(enum Type_Sort) * 20);
	memcpy((void *)newInventory, (void *)oldInventory, sizeof(enum Type_Sort) * 20);
	return newInventory;
}

/*-------------------------------------------------------------------
 * Function 	: getItemName
 * Inputs	: enum Alpha_Sort 	a_key
 * Outputs	: char			*itemName
 *
 * Access the itemNames array to associate an item's a_key with its
 * string counterpart. Also handles the case of a null item.
 -------------------------------------------------------------------*/
char *getItemName(enum Type_Sort t_key) {
	return t_key < 0 ? "NULL ITEM" : itemNames[t_key];
}

/*-------------------------------------------------------------------
 * Function 	: shiftDownToFillNull
 * Inputs	: enum Type_Sort	*inventory
 *
 * There is a null in the inventory. Shift items after the null towards
 * the beginning of the array to fill in the null. Then place the null
 * at the end of the array.
 -------------------------------------------------------------------*/
void shiftDownToFillNull(enum Type_Sort *inventory) {
	// First find the index of the first null
	int firstNull = -1;
	for (int i = 0; i < 20; i++) {
		if (inventory[i] == -1) {
			firstNull = i;
			break;
		}
	}

	// Now shift all items up in the inventory to place a null at the end of the inventory
	memmove(&inventory[firstNull], &inventory[firstNull + 1], (19 - firstNull) * sizeof(enum Type_Sort));

	// Set the last inventory slot to null
	inventory[19] = -1;
	
	return;
}

/*-------------------------------------------------------------------
 * Function 	: shiftUpToFillNull
 * Inputs	: enum Type_Sort *inventory
 *
 * There is a null in the inventory. Shift items before the null towards
 * the end of the array to fill in the null. A new item will be placed
 * at the start of the inventory after return.
 -------------------------------------------------------------------*/
void shiftUpToFillNull(enum Type_Sort *inventory) {
	// First find the index of the first null
	int firstNull = -1;
	for (int i = 0; i < 20; i++) {
		if (inventory[i] == -1) {
			firstNull = i;
			break;
		}
	}
	
	// Now shift all items further down in the inventory to make room for a new item
	memmove(&inventory[1], inventory, firstNull * sizeof(enum Type_Sort));
	
	return;
}

/*-------------------------------------------------------------------
 * Function 	: getInventoryFrames
 * Inputs	: 
 * Outputs	: int **inv_frames
 *
 * Returns a double pointer which can be used to reference how many
 * frames it takes to access a specific item
 * inv_frames[x][y] where:
 *	- x = number of valid items in inventory
 *	- y = index of item minus any nulls in the inventory prior to index of item
 -------------------------------------------------------------------*/
int **getInventoryFrames() {
	static int *inv_frames[INVENTORY_MAX_SIZE];
	int frameList[11] = {0, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18};

	for (int i = 0; i < INVENTORY_MAX_SIZE; i++) {
		int *frames = malloc(sizeof(int) * (i+1));
		for (int j = 0; j < i + 1; j++) {
			if (j < i+1-j)
				frames[j] = frameList[j];
			else
				frames[j] = frameList[i-j+1];
		}
		inv_frames[i] = frames;
	}
	recipeLog(2, "Inventory", "Frames", "Generate", "Inventory Frames Generated");
	return inv_frames;
}

/*-------------------------------------------------------------------
 * Function 	: getStartingInventory
 * Inputs	: 
 * Outputs	: enum Type_Sort *inventory
 *
 * Returns a pointer to an array which contains all items we start with
 -------------------------------------------------------------------*/
enum Type_Sort *getStartingInventory() {
	static enum Type_Sort inventory[] = {
		Golden_Leaf,
		Peachy_Peach,
		Shooting_Star,
		Ultra_Shroom,
		Cake_Mix,
		Thunder_Rage,
		Turtley_Leaf,
		Life_Shroom,
		Ice_Storm,
		Slow_Shroom,
		Mystery,
		Honey_Syrup,
		Volt_Shroom,
		Fire_Flower,
		Mystic_Egg,
		Hot_Dog,
		Ruin_Powder,
		Maple_Syrup,
		Hot_Sauce,
		Jammin_Jelly
	};
	
	return inventory;
}
