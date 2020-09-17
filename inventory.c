#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inventory.h"
#include "logger.h"

#define INVENTORY_MAX_SIZE 21
#define NUM_ITEMS 107

typedef enum Alpha_Sort Alpha_Sort;
typedef enum Type_Sort Type_Sort;
typedef struct Item Item;
typedef struct ItemName ItemName;

/*====================NOTES====================
- Items are interpreted based on the Alpha_Sort enumerator value. A function can later be written to conver this to a string.
  - This negates the need to allocate memory to handle strings, which will reduce issues in the future.
  - Using the alpha key as the main identifier also negates the need to have a function to obtain the alpha key along with the type key.
- Each item in the inventory is represented with a struct which contains both its alphabetical and type sort keys, again using
  the alphabetical key as the main item identifier.
====================     ====================*/

Item items[] = {
	{POW_Block, POW_Block_t},
	{Icicle_Pop, Icicle_Pop_t},
	{Fright_Mask, Fright_Mask_t},
	{Spicy_Soup, Spicy_Soup_t},
	{Ink_Pasta, Ink_Pasta_t},
	{Couples_Cake, Couples_Cake_t},
	{Point_Swap, Point_Swap_t},
	{Space_Food, Space_Food_t},
	{Ultra_Shroom, Ultra_Shroom_t},
	{Golden_Leaf, Golden_Leaf_t},
	{Cake_Mix, Cake_Mix_t},
	{Courage_Shell, Courage_Shell_t},
	{Courage_Meal, Courage_Meal_t},
	{Thunder_Bolt, Thunder_Bolt_t},
	{Thunder_Rage, Thunder_Rage_t},
	{Koopa_Tea, Koopa_Tea_t},
	{Turtley_Leaf, Turtley_Leaf_t},
	{Koopasta, Koopasta_t},
	{Koopa_Bun, Koopa_Bun_t},
	{Spicy_Pasta, Spicy_Pasta_t},
	{Omelette_Meal, Omelette_Meal_t},
	{Mushroom, Mushroom_t},
	{Shroom_Fry, Shroom_Fry_t},
	{Shroom_Crepe, Shroom_Crepe_t},
	{Shroom_Cake, Shroom_Cake_t},
	{Shroom_Steak, Shroom_Steak_t},
	{Shroom_Roast, Shroom_Roast_t},
	{Shooting_Star, Shooting_Star_t},
	{Gold_Bar, Gold_Bar_t},
	{Gold_Bar_x_3, Gold_Bar_x_3_t},
	{Life_Shroom, Life_Shroom_t},
	{Dizzy_Dial, Dizzy_Dial_t},
	{Shroom_Broth, Shroom_Broth_t},
	{Ice_Storm, Ice_Storm_t},
	{Coconut_Bomb, Coconut_Bomb_t},
	{Coco_Candy, Coco_Candy_t},
	{Spite_Pouch, Spite_Pouch_t},
	{Mistake, Mistake_t},
	{Dried_Shroom, Dried_Shroom_t},
	{Inn_Coupon, Inn_Coupon_t},
	{Choco_Cake, Choco_Cake_t},
	{Trial_Stew, Trial_Stew_t},
	{Slow_Shroom, Slow_Shroom_t},
	{Gradual_Syrup, Gradual_Syrup_t},
	{Super_Shroom, Super_Shroom_t},
	{HP_Drain, HP_Drain_t},
	{Tasty_Tonic, Tasty_Tonic_t},
	{Stopwatch, Stopwatch_t},
	{Spaghetti, Spaghetti_t},
	{Inky_Sauce, Inky_Sauce_t},
	{Whacka_Bump, Whacka_Bump_t},
	{Horsetail, Horsetail_t},
	{Repel_Cape, Repel_Cape_t},
	{Boos_Sheet, Boos_Sheet_t},
	{Power_Punch, Power_Punch_t},
	{Keel_Mango, Keel_Mango_t},
	{Poison_Shroom, Poison_Shroom_t},
	{Dried_Bouquet, Dried_Bouquet_t},
	{Mystery, Mystery_t},
	{Zess_Cookie, Zess_Cookie_t},
	{Zess_Special, Zess_Special_t},
	{Zess_Dynamite, Zess_Dynamite_t},
	{Zess_Tea, Zess_Tea_t},
	{Zess_Dinner, Zess_Dinner_t},
	{Zess_Deluxe, Zess_Deluxe_t},
	{Zess_Frappe, Zess_Frappe_t},
	{Sleepy_Sheep, Sleepy_Sheep_t},
	{Love_Pudding, Love_Pudding_t},
	{Honey_Candy, Honey_Candy_t},
	{Honey_Shroom, Honey_Shroom_t},
	{Honey_Super, Honey_Super_t},
	{Honey_Ultra, Honey_Ultra_t},
	{Honey_Syrup, Honey_Syrup_t},
	{Egg_Bomb, Egg_Bomb_t},
	{Volt_Shroom, Volt_Shroom_t},
	{Electro_Pop, Electro_Pop_t},
	{Peach_Tart, Peach_Tart_t},
	{Peachy_Peach, Peachy_Peach_t},
	{Fire_Pop, Fire_Pop_t},
	{Fire_Flower, Fire_Flower_t},
	{Mystic_Egg, Mystic_Egg_t},
	{Mr_Softener, Mr_Softener_t},
	{Fruit_Parfait, Fruit_Parfait_t},
	{Fresh_Juice, Fresh_Juice_t},
	{Healthy_Salad, Healthy_Salad_t},
	{Meteor_Meal, Meteor_Meal_t},
	{Hot_Dog, Hot_Dog_t},
	{Ruin_Powder, Ruin_Powder_t},
	{Mango_Delight, Mango_Delight_t},
	{Mini_Mr_Mini, Mini_Mr_Mini_t},
	{Mousse_Cake, Mousse_Cake_t},
	{Maple_Shroom, Maple_Shroom_t},
	{Maple_Super, Maple_Super_t},
	{Maple_Ultra, Maple_Ultra_t},
	{Maple_Syrup, Maple_Syrup_t},
	{Fried_Egg, Fried_Egg_t},
	{Heartful_Cake, Heartful_Cake_t},
	{Coconut, Coconut_t},
	{Snow_Bunny, Snow_Bunny_t},
	{Earth_Quake, Earth_Quake_t},
	{Hot_Sauce, Hot_Sauce_t},
	{Jelly_Shroom, Jelly_Shroom_t},
	{Jelly_Super, Jelly_Super_t},
	{Jelly_Ultra, Jelly_Ultra_t},
	{Jelly_Candy, Jelly_Candy_t},
	{Jammin_Jelly, Jammin_Jelly_t},
	{Fresh_Pasta, Fresh_Pasta_t}
};

/*-------------------------------------------------------------------
 * Function 	: getItem
 * Inputs	: enum Alpha_Sort a_key
 * Outputs	: struct Item 	  item
 *
 * Use an item's a_key to retrieve its Item struct from the above array
 -------------------------------------------------------------------*/
struct Item getItem(enum Alpha_Sort a_key) {
	return items[a_key];
}

ItemName itemNames[] = {
	{POW_Block, "POW Block"},
	{Icicle_Pop, "Icicle Pop"},
	{Fright_Mask, "Fright Mask"},
	{Spicy_Soup, "Spicy Soup"},
	{Ink_Pasta, "Ink Pasta"},
	{Couples_Cake, "Couples Cake"},
	{Point_Swap, "Point Swap"},
	{Space_Food, "Space Food"},
	{Ultra_Shroom, "Ultra Shroom"},
	{Golden_Leaf, "Golden Leaf"},
	{Cake_Mix, "Cake Mix"},
	{Courage_Shell, "Courage Shell"},
	{Courage_Meal, "Courage Meal"},
	{Thunder_Bolt, "Thunder Bolt"},
	{Thunder_Rage, "Thunder Rage"},
	{Koopa_Tea, "Koopa Tea"},
	{Turtley_Leaf, "Turtley Leaf"},
	{Koopasta, "Koopasta"},
	{Koopa_Bun, "Koopa Bun"},
	{Spicy_Pasta, "Spicy Pasta"},
	{Omelette_Meal, "Omelette Meal"},
	{Mushroom, "Mushroom"},
	{Shroom_Fry, "Shroom Fry"},
	{Shroom_Crepe, "Shroom Crepe"},
	{Shroom_Cake, "Shroom Cake"},
	{Shroom_Steak, "Shroom Steak"},
	{Shroom_Roast, "Shroom Roast"},
	{Shooting_Star, "Shooting Star"},
	{Gold_Bar, "Gold Bar"},
	{Gold_Bar_x_3, "Gold Bar x 3"},
	{Life_Shroom, "Life Shroom"},
	{Dizzy_Dial, "Dizzy Dial"},
	{Shroom_Broth, "Shroom Broth"},
	{Ice_Storm, "Ice Storm"},
	{Coconut_Bomb, "Coconut Bomb"},
	{Coco_Candy, "Coco Candy"},
	{Spite_Pouch, "Spite Pouch"},
	{Mistake, "Mistake"},
	{Dried_Shroom, "Dried Shroom"},
	{Inn_Coupon, "Inn Coupon"},
	{Choco_Cake, "Choco Cake"},
	{Trial_Stew, "Trial Stew"},
	{Slow_Shroom, "Slow Shroom"},
	{Gradual_Syrup, "Gradual Syrup"},
	{Super_Shroom, "Super Shroom"},
	{HP_Drain, "HP Drain"},
	{Tasty_Tonic, "Tasty Tonic"},
	{Stopwatch, "Stopwatch"},
	{Spaghetti, "Spaghetti"},
	{Inky_Sauce, "Inky Sauce"},
	{Whacka_Bump, "Whacka Bump"},
	{Horsetail, "Horsetail"},
	{Repel_Cape, "Repel Cape"},
	{Boos_Sheet, "Boos Sheet"},
	{Power_Punch, "Power Punch"},
	{Keel_Mango, "Keel Mango"},
	{Poison_Shroom, "Poison Shroom"},
	{Dried_Bouquet, "Dried Bouquet"},
	{Mystery, "Mystery"},
	{Zess_Cookie, "Zess Cookie"},
	{Zess_Special, "Zess Special"},
	{Zess_Dynamite, "Zess Dynamite"},
	{Zess_Tea, "Zess Tea"},
	{Zess_Dinner, "Zess Dinner"},
	{Zess_Deluxe, "Zess Deluxe"},
	{Zess_Frappe, "Zess Frappe"},
	{Sleepy_Sheep, "Sleepy Sheep"},
	{Love_Pudding, "Love Pudding"},
	{Honey_Candy, "Honey Candy"},
	{Honey_Shroom, "Honey Shroom"},
	{Honey_Super, "Honey Super"},
	{Honey_Ultra, "Honey Ultra"},
	{Honey_Syrup, "Honey Syrup"},
	{Egg_Bomb, "Egg Bomb"},
	{Volt_Shroom, "Volt Shroom"},
	{Electro_Pop, "Electro Pop"},
	{Peach_Tart, "Peach Tart"},
	{Peachy_Peach, "Peachy Peach"},
	{Fire_Pop, "Fire Pop"},
	{Fire_Flower, "Fire Flower"},
	{Mystic_Egg, "Mystic Egg"},
	{Mr_Softener, "Mr. Softener"},
	{Fruit_Parfait, "Fruit Parfait"},
	{Fresh_Juice, "Fresh Juice"},
	{Healthy_Salad, "Healthy Salad"},
	{Meteor_Meal, "Meteor Meal"},
	{Hot_Dog, "Hot Dog"},
	{Ruin_Powder, "Ruin Powder"},
	{Mango_Delight, "Mango Delight"},
	{Mini_Mr_Mini, "Mini Mr. Mini"},
	{Mousse_Cake, "Mousse Cake"},
	{Maple_Shroom, "Maple Shroom"},
	{Maple_Super, "Maple Super"},
	{Maple_Ultra, "Maple Ultra"},
	{Maple_Syrup, "Maple Syrup"},
	{Fried_Egg, "Fried Egg"},
	{Heartful_Cake, "Heartful Cake"},
	{Coconut, "Coconut"},
	{Snow_Bunny, "Snow Bunny"},
	{Earth_Quake, "Earth Quake"},
	{Hot_Sauce, "Hot Sauce"},
	{Jelly_Shroom, "Jelly Shroom"},
	{Jelly_Super, "Jelly Super"},
	{Jelly_Ultra, "Jelly Ultra"},
	{Jelly_Candy, "Jelly Candy"},
	{Jammin_Jelly, "Jammin' Jelly"},
	{Fresh_Pasta, "Fresh Pasta"}
};

/*-------------------------------------------------------------------
 * Function 	: compareInventories
 * Inputs	: struct Item 	*inv1
 * 		  struct Item	*inv2
 * Outputs	: 0 - inventories are different
 *		  1 - inventories are identical
 *
 * Compare two inventories for any differences. This is used to determine
 * if sorts changed the inventory at all.
 -------------------------------------------------------------------*/
int compareInventories(struct Item *inv1, struct Item *inv2) {
	for (int i = 0; i < 20; i++) {
		if (inv1[i].a_key != inv2[i].a_key)
			return 0;
	}
	return 1;
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
 * Inputs	: struct Item 	*inventory
 *		  int		minIndex
 *		  int		maxIndex
 * Outputs	: The number of nulls in the inventory
 *
 * Traverse through the inventory and count the number of blank entries
 * in the inventory.
 -------------------------------------------------------------------*/
int countNullsInInventory(struct Item *inventory, int minIndex, int maxIndex) {
	int count = 0;
	for (int i = minIndex; i < maxIndex; i++) {
		if ((int) inventory[i].a_key == -1)
			count++;
	}
	return count;
}

/*-------------------------------------------------------------------
 * Function 	: indexOfItemInInventory
 * Inputs	: struct Item 	*inventory
 *		  struct Item	item
 * Outputs	: index of item in inventory
 *
 * Traverse through the inventory and find the location of the provided
 * item. If not found, return -1.
 -------------------------------------------------------------------*/
int indexOfItemInInventory(struct Item *inventory, struct Item item) {
	for (int i = 0; i < 20; i++) {
		if (inventory[i].a_key == item.a_key)
			return i;
	}
	return -1;
}

/*-------------------------------------------------------------------
 * Function 	: countItemsInInventory
 * Inputs	: struct Item 	*inventory
 * Outputs	: number of item in inventory
 *
 * Traverse through the inventory and count the number of valid items
 * in the inventory. This excludes blank (NULL) entries.
 -------------------------------------------------------------------*/
int countItemsInInventory(struct Item *inventory) {
	int count = 0;
	for (int i = 0; i < 20; i++) {
		if (inventory[i].a_key != -1) {
			count++;
		}
	}
	
	return count;
}
			
/*-------------------------------------------------------------------
 * Function 	: copyInventory
 * Inputs	: struct Item 	*oldInventory
 * Outputs	: struct Item	*newInventory
 *
 * Perform a simple memcpy to duplicate an old inventory to a newly
 * allocated inventory.
 -------------------------------------------------------------------*/
struct Item *copyInventory(struct Item* oldInventory) {
	struct Item *newInventory = malloc(sizeof(struct Item) * 20);
	memcpy((void *)newInventory, (void *)oldInventory, sizeof(struct Item) * 20);
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
char *getItemName(Alpha_Sort a_key) {
	for (int i = 0; i < NUM_ITEMS; i++) {
		if (itemNames[i].a_key == a_key) {
			return itemNames[i].name;
		}
	}
	
	return "NULL ITEM";
}

/*-------------------------------------------------------------------
 * Function 	: shiftDownToFillNull
 * Inputs	: struct Item	*inventory
 *
 * There is a null in the inventory. Shift items after the null towards
 * the beginning of the array to fill in the null. Then place the null
 * at the end of the array.
 -------------------------------------------------------------------*/
void shiftDownToFillNull(struct  Item *inventory) {
	// First find the index of the first null
	int firstNull = -1;
	for (int i = 0; i < 20; i++) {
		if (inventory[i].a_key == -1) {
			firstNull = i;
			break;
		}
	}
	
	// Now shift all items up in the inventory to place a null at the end of the inventory
	for (int i = firstNull; i < 20 - 1; i++) {
		inventory[i] = inventory[i+1];
	}
	
	// Set the last inventory slot to null
	inventory[19] = (struct Item) {-1, -1};
	
	return;
}

/*-------------------------------------------------------------------
 * Function 	: shiftUpToFillNull
 * Inputs	: struct Item	*inventory
 *
 * There is a null in the inventory. Shift items before the null towards
 * the end of the array to fill in the null. A new item will be placed
 * at the start of the inventory after return.
 -------------------------------------------------------------------*/
void shiftUpToFillNull(struct Item *inventory) {
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
 * Outputs	: Item *inventory
 *
 * Returns a pointer to an array which contains all items we start with
 -------------------------------------------------------------------*/
Item *getStartingInventory() {
	static Item inventory[] = {
		{Golden_Leaf, Golden_Leaf_t},
		{Peachy_Peach, Peachy_Peach_t},
		{Shooting_Star, Shooting_Star_t},
		{Ultra_Shroom, Ultra_Shroom_t},
		{Cake_Mix, Cake_Mix_t},
		{Thunder_Rage, Thunder_Rage_t},
		{Turtley_Leaf, Turtley_Leaf_t},
		{Life_Shroom, Life_Shroom_t},
		{Ice_Storm, Ice_Storm_t},
		{Slow_Shroom, Slow_Shroom_t},
		{Mystery, Mystery_t},
		{Honey_Syrup, Honey_Syrup_t},
		{Volt_Shroom, Volt_Shroom_t},
		{Fire_Flower, Fire_Flower_t},
		{Mystic_Egg, Mystic_Egg_t},
		{Hot_Dog, Hot_Dog_t},
		{Ruin_Powder, Ruin_Powder_t},
		{Maple_Syrup, Maple_Syrup_t},
		{Hot_Sauce, Hot_Sauce_t},
		{Jammin_Jelly, Jammin_Jelly_t}
	};
	
	return inventory;
}
