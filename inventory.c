#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inventory.h"

#define INVENTORY_MAX_SIZE 21
#define NUM_ITEMS 107

typedef enum Alpha_Sort Alpha_Sort;
typedef enum Type_Sort Type_Sort;
typedef struct Item Item;

/*====================NOTES====================
- Items are interpreted based on the Alpha_Sort enumerator value. A function can later be written to conver this to a string.
  - This negates the need to allocate memory to handle strings, which will reduce issues in the future.
  - Using the alpha key as the main identifier also negates the need to have a function to obtain the alpha key along with the type key.
- Each item in the inventory is represented with a struct which contains both its alphabetical and type sort keys, again using
  the alphabetical key as the main item identifier.
  
- TODO:
	- checkRecipe
	- remainingOutputsCanBeFulfilled
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

struct Item getItem(enum Alpha_Sort a_key) {
	return items[a_key];
}

// I don't think we need this function
/*Type_Sort getTypeKey (Alpha_Sort a_key) {
	for (int i = Mushroom_t; i <= Mistake_t; i++) {
		if (items[i].a_key == a_key) {
			return items[i].t_key;
		}
	}
	
	return -1;
}*/

int **getInventoryFrames() {
	static int *inv_frames[INVENTORY_MAX_SIZE];
	int frameList[11] = {0, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18};

	for (int i = 0; i <= INVENTORY_MAX_SIZE; i++) {
		int *frames = malloc(sizeof(int) * (i+1));
		for (int j = 0; j < i + 1; j++) {
			if (j < i+1-j)
				frames[j] = frameList[j];
			else
				frames[j] = frameList[i-j];
		}
		inv_frames[i] = frames;
	}
	return inv_frames;
}

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

/*int main()
{
	//FOR TESTING getInventoryFrames
	int **inv_frames;
	inv_frames = getInventoryFrames();
	for (int i = 0; i <= INVENTORY_MAX_SIZE; i++) {
		for (int j = 0; j < i; j++) {
			int *frames = inv_frames[i];
			printf("Inventory size %d:\tIndex %d:\t%d\n", i, j, frames[j]);
		}
	}
	
	
	//FOR TESTING ITEM SORT RETRIEVAL
	printf("Name\t\tAlpha\tType\n");
	for (int i = 0; i < NUM_ITEMS; i++) {
		char *name = items[i].name;
		enum Alpha_Sort a_key = items[i].a_key;
		enum Type_Sort t_key = items[i].t_key;
		printf("%s\t%d\t%d\n", name, a_key, t_key);
	}
	
	
	//FOR TESTING KEY RETRIEVAL
	Item *inventory = getStartingInventory();
	for (int i = 0; i < 20; i++) {
		printf("Slot   %d\tAlpha   %d\tType %d\t\n", i+1, inventory[i].a_key, inventory[i].t_key);
	}
	
	
	return 0;
}*/
