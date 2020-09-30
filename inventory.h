#ifndef INVENTORY_H
#define INVENTORY_H

#include "recipes.h"

enum Alpha_Sort {
	POW_Block_a,
	Icicle_Pop_a,
	Fright_Mask_a,
	Spicy_Soup_a,
	Ink_Pasta_a,
	Couples_Cake_a,
	Point_Swap_a,
	Space_Food_a,
	Ultra_Shroom_a,
	Golden_Leaf_a,
	Cake_Mix_a,
	Courage_Shell_a,
	Courage_Meal_a,
	Thunder_Bolt_a,
	Thunder_Rage_a,
	Koopa_Tea_a,
	Turtley_Leaf_a,
	Koopasta_a,
	Koopa_Bun_a,
	Spicy_Pasta_a,
	Omelette_Meal_a,
	Mushroom_a,
	Shroom_Fry_a,
	Shroom_Crepe_a,
	Shroom_Cake_a,
	Shroom_Steak_a,
	Shroom_Roast_a,
	Shooting_Star_a,
	Gold_Bar_a,
	Gold_Bar_x_3_a,
	Life_Shroom_a,
	Dizzy_Dial_a,
	Shroom_Broth_a,
	Ice_Storm_a,
	Coconut_Bomb_a,
	Coco_Candy_a,
	Spite_Pouch_a,
	Mistake_a,
	Dried_Shroom_a,
	Inn_Coupon_a,
	Choco_Cake_a,
	Trial_Stew_a,
	Slow_Shroom_a,
	Gradual_Syrup_a,
	Super_Shroom_a,
	HP_Drain_a,
	Tasty_Tonic_a,
	Stopwatch_a,
	Spaghetti_a,
	Inky_Sauce_a,
	Whacka_Bump_a,
	Horsetail_a,
	Repel_Cape_a,
	Boos_Sheet_a,
	Power_Punch_a,
	Keel_Mango_a,
	Poison_Shroom_a,
	Dried_Bouquet_a,
	Mystery_a,
	Zess_Cookie_a,
	Zess_Special_a,
	Zess_Dynamite_a,
	Zess_Tea_a,
	Zess_Dinner_a,
	Zess_Deluxe_a,
	Zess_Frappe_a,
	Sleepy_Sheep_a,
	Love_Pudding_a,
	Honey_Candy_a,
	Honey_Shroom_a,
	Honey_Super_a,
	Honey_Ultra_a,
	Honey_Syrup_a,
	Egg_Bomb_a,
	Volt_Shroom_a,
	Electro_Pop_a,
	Peach_Tart_a,
	Peachy_Peach_a,
	Fire_Pop_a,
	Fire_Flower_a,
	Mystic_Egg_a,
	Mr_Softener_a,
	Fruit_Parfait_a,
	Fresh_Juice_a,
	Healthy_Salad_a,
	Meteor_Meal_a,
	Hot_Dog_a,
	Ruin_Powder_a,
	Mango_Delight_a,
	Mini_Mr_Mini_a,
	Mousse_Cake_a,
	Maple_Shroom_a,
	Maple_Super_a,
	Maple_Ultra_a,
	Maple_Syrup_a,
	Fried_Egg_a,
	Heartful_Cake_a,
	Coconut_a,
	Snow_Bunny_a,
	Earth_Quake_a,
	Hot_Sauce_a,
	Jelly_Shroom_a,
	Jelly_Super_a,
	Jelly_Ultra_a,
	Jelly_Candy_a,
	Jammin_Jelly_a,
	Fresh_Pasta_a
};

enum Type_Sort {
	Mushroom,
	Super_Shroom,
	Ultra_Shroom,
	Life_Shroom,
	Slow_Shroom,
	Dried_Shroom,
	Honey_Syrup,
	Maple_Syrup,
	Jammin_Jelly,
	Gradual_Syrup,
	Tasty_Tonic,
	POW_Block,
	Fire_Flower,
	Ice_Storm,
	Earth_Quake,
	Thunder_Bolt,
	Thunder_Rage,
	Shooting_Star,
	Volt_Shroom,
	Repel_Cape,
	Boos_Sheet,
	Ruin_Powder,
	Sleepy_Sheep,
	Dizzy_Dial,
	Stopwatch,
	Power_Punch,
	Mini_Mr_Mini,
	Courage_Shell,
	Mr_Softener,
	HP_Drain,
	Point_Swap,
	Fright_Mask,
	Mystery,
	Inn_Coupon,
	Gold_Bar,
	Gold_Bar_x_3,
	Whacka_Bump,
	Hot_Dog,
	Coconut,
	Dried_Bouquet,
	Mystic_Egg,
	Golden_Leaf,
	Keel_Mango,
	Fresh_Pasta,
	Cake_Mix,
	Hot_Sauce,
	Turtley_Leaf,
	Horsetail,
	Peachy_Peach,
	Spite_Pouch,
	Shroom_Fry,
	Shroom_Roast,
	Shroom_Steak,
	Honey_Shroom,
	Maple_Shroom,
	Jelly_Shroom,
	Honey_Super,
	Maple_Super,
	Jelly_Super,
	Honey_Ultra,
	Maple_Ultra,
	Jelly_Ultra,
	Zess_Dinner,
	Zess_Special,
	Zess_Deluxe,
	Spaghetti,
	Koopasta,
	Spicy_Pasta,
	Ink_Pasta,
	Spicy_Soup,
	Fried_Egg,
	Omelette_Meal,
	Koopa_Bun,
	Healthy_Salad,
	Meteor_Meal,
	Couples_Cake,
	Mousse_Cake,
	Shroom_Cake,
	Choco_Cake,
	Heartful_Cake,
	Fruit_Parfait,
	Mango_Delight,
	Love_Pudding,
	Zess_Cookie,
	Shroom_Crepe,
	Peach_Tart,
	Koopa_Tea,
	Zess_Tea,
	Shroom_Broth,
	Fresh_Juice,
	Inky_Sauce,
	Icicle_Pop,
	Zess_Frappe,
	Snow_Bunny,
	Coco_Candy,
	Honey_Candy,
	Jelly_Candy,
	Electro_Pop,
	Fire_Pop,
	Space_Food,
	Poison_Shroom,
	Trial_Stew,
	Courage_Meal,
	Coconut_Bomb,
	Egg_Bomb,
	Zess_Dynamite,
	Mistake
};

struct Inventory {
	size_t nulls;
	size_t length;
	enum Type_Sort inventory[20];
};

struct ItemCombination {
	int numItems; // If set to 1, ignore item2
	enum Type_Sort item1;
	enum Type_Sort item2;
};

struct Recipe {
	enum Type_Sort output;
	int countCombos;
	struct ItemCombination *combos; // Where there are countCombos different ways to cook output
};

// Recipe functions
int getIndexOfRecipe(enum Type_Sort item);
struct Recipe* getRecipeList();
int stateOK(struct Inventory inventory, const int* const outputsCreated, struct Recipe* recipeList);

struct ItemCombination parseCombo(int itemCount, enum Type_Sort item1, enum Type_Sort item2);

void placeInventoryInMakeableItems(int *makeableItems, struct Inventory inventory);

int itemComboInInventory(struct ItemCombination combo, struct Inventory inventory);

// Returns 1 if the inventories are the same. Return 0 if they are different
int compareInventories(struct Inventory inv1, struct Inventory inv2);

int itemInDependentIndices(int index, int *dependentIndices, int numDependentIndices);

// Returns the index of an item in the inventory. -1 if not found
int indexOfItemInInventory(struct Inventory inventory, enum Type_Sort item);

enum Alpha_Sort getAlphaKey(enum Type_Sort item);

// Return the string name for a particular item
char *getItemName(enum Type_Sort t_key);

// Return a pointer to an array of length 21 with each index i containing a variable length j which tracks the frameloss to navigate to the jth index in an inventory of size i
int **getInventoryFrames();

// I don't believe we need this
/*struct Type_Sort getTypeKey (Alpha_Sort a_key);*/

// Return array of Item structs for the start of cooking recipes
struct Inventory getStartingInventory();

struct Inventory replaceItem(struct Inventory inventory, int index, enum Type_Sort item);

struct Inventory addItem(struct Inventory inventory, enum Type_Sort item);

struct Inventory removeItem(struct Inventory inventory, int index);

#endif
