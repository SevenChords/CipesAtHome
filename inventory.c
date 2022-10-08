#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inventory.h"

#include "types.h"

#include "base.h"
#include "logger.h"


#define VOLATILE_INVENTORY_SIZE 10
#define INVENTORY_SIZE 20
#define INVENTORY_MAX_SIZE 21
#define NUM_ITEMS 107

typedef enum Alpha_Sort Alpha_Sort;
typedef enum Type_Sort Type_Sort;
typedef struct ItemName ItemName;

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
 * Outputs	: Alpha_Sort alpha_key
 *
 * Use items array to get alpha key for a given item when we need to sort.
 -------------------------------------------------------------------*/
Alpha_Sort getAlphaKey(enum Type_Sort item) {
	return items[item];
}

/*-------------------------------------------------------------------
 * Function 	: compareInventories
 * Inputs	: struct Inventory *inv1
 * 		  struct Inventory *inv2
 * Outputs	: 0 - inventories are different
 *		  1 - inventories are identical
 *
 * Compare two inventories for any differences. This is used to determine
 * if sorts changed the inventory at all.
 -------------------------------------------------------------------*/
int compareInventories(struct Inventory inv1, struct Inventory inv2) {
	return inv1.nulls == inv2.nulls && inv1.length == inv2.length
		&& memcmp((void*)(inv1.inventory + inv1.nulls),
				  (void*)(inv2.inventory + inv2.nulls),
				  (inv1.length - inv1.nulls) * sizeof(enum Type_Sort)) == 0;
}

/*-------------------------------------------------------------------
 * Function 	: itemComboInInventory
 * Inputs	: struct ItemCombination	combo
 *		  enum Type_Sort		*inventory
 * Outputs	: int (0 or 1)
 *
 * Determine whether the items in a recipe combination exist in the
 * inventory. In the case of a 1 item recipe, only check for the one item.
 * Correctly handles the case where items at the end of our inventory
 * may not be viewable depending on how many NULLs there are in inventory.
 -------------------------------------------------------------------*/
int itemComboInInventory(struct ItemCombination combo, struct Inventory inventory) {
	int indexItem1 = indexOfItemInInventory(inventory, combo.item1);
	if (indexItem1 == -1) {
		return 0;
	}
	else if (combo.numItems == 1) {
		return 1;
	}

	int indexItem2 = indexOfItemInInventory(inventory, combo.item2);
	return indexItem2 != -1;
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
 * Function 	: indexOfItemInInventory
 * Inputs	: struct Inventory *inventory
 *		  struct Type_Sort item
 * Outputs	: index of item in inventory
 *
 * Traverse through the inventory and find the location of the provided
 * item. If not found, return -1.
 -------------------------------------------------------------------*/
int indexOfItemInInventory(struct Inventory inventory, enum Type_Sort item) {
	int i;
	for (i = inventory.nulls; i < VOLATILE_INVENTORY_SIZE; ++i) {
		if (inventory.inventory[i] == item)
			return i;
	}
	int visibleLength = inventory.length - inventory.nulls;
	for (; i < visibleLength; ++i) {
		if (inventory.inventory[i] == item)
			return i;
	}
	return -1;
}

/*-------------------------------------------------------------------
 * Function 	: getItemName
 * Inputs	: enum Type_Sort t_key
 * Outputs	: char			*itemName
 *
 * Access the itemNames array to associate an item's a_key with its
 * string counterpart. Also handles the case of a null item.
 -------------------------------------------------------------------*/
char *getItemName(Type_Sort t_key) {
	return itemNames[t_key];
}

/*-------------------------------------------------------------------
 * Function 	: getInventoryFrames
 * Inputs	:
 * Outputs	: int **inv_frames
 *
 * Returns a double pointer which can be used to reference how many
 * frames it takes to access a specific item
 * inv_frames[x][y] where:
 *	- x = number of valid items in inventory - 1
 *	- y = index of item minus any nulls in the inventory prior to index of item
 -------------------------------------------------------------------*/
int **getInventoryFrames() {
	static int *inv_frames[INVENTORY_MAX_SIZE];
	int frameList[11] = {0, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18};

	for (int i = 0; i < INVENTORY_MAX_SIZE; i++) {
		int *frames = malloc(sizeof(int) * (i+1));

		checkMallocFailed(frames);

		for (int j = 0; j < i + 1; j++) {
			if (j < i+1-j)
				frames[j] = frameList[j];
			else
				frames[j] = frameList[i-j+1];
		}
		inv_frames[i] = frames;
	}
	recipeLog(5, "Inventory", "Frames", "Generate", "Inventory Frames Generated");
	return inv_frames;
}

/*-------------------------------------------------------------------
 * Function 	: getStartingInventory
 * Inputs	:
 * Outputs	: enum Type_Sort *inventory
 *
 * Returns a pointer to an array which contains all items we start with
 -------------------------------------------------------------------*/
struct Inventory getStartingInventory() {
	static struct Inventory inventory;
	inventory.nulls = 0;
	inventory.length = 20;
	inventory.inventory[0] = Golden_Leaf;
	inventory.inventory[1] = Peachy_Peach;
	inventory.inventory[2] = Shooting_Star;
	inventory.inventory[3] = Ultra_Shroom;
	inventory.inventory[4] = Cake_Mix;
	inventory.inventory[5] = Thunder_Rage;
	inventory.inventory[6] = Turtley_Leaf;
	inventory.inventory[7] = Life_Shroom;
	inventory.inventory[8] = Ice_Storm;
	inventory.inventory[9] = Slow_Shroom;
	inventory.inventory[10] = Mystery;
	inventory.inventory[11] = Honey_Syrup;
	inventory.inventory[12] = Volt_Shroom;
	inventory.inventory[13] = Fire_Flower;
	inventory.inventory[14] = Mystic_Egg;
	inventory.inventory[15] = Hot_Dog;
	inventory.inventory[16] = Ruin_Powder;
	inventory.inventory[17] = Maple_Syrup;
	inventory.inventory[18] = Hot_Sauce;
	inventory.inventory[19] = Jammin_Jelly;

	return inventory;
}

struct Inventory replaceItem(struct Inventory inventory, int index, Type_Sort item) {
	memmove(inventory.inventory + 1, inventory.inventory, index * sizeof(Type_Sort));
	inventory.inventory[inventory.nulls] = item;

	return inventory;
}

struct Inventory addItem(struct Inventory inventory, Type_Sort item) {
	inventory.inventory[--inventory.nulls] = item;

	return inventory;
}

struct Inventory removeItem(struct Inventory inventory, int index) {
	memmove(inventory.inventory + 1, inventory.inventory, index * sizeof(Type_Sort));
	++inventory.nulls;

	return inventory;
}
