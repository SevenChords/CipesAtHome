#ifndef INVENTORY_H
#define INVENTORY_H
//#include "calculator.h"
enum Alpha_Sort {
	POW_Block,
	Icicle_Pop,
	Fright_Mask,
	Spicy_Soup,
	Ink_Pasta,
	Couples_Cake,
	Point_Swap,
	Space_Food,
	Ultra_Shroom,
	Golden_Leaf,
	Cake_Mix,
	Courage_Shell,
	Courage_Meal,
	Thunder_Bolt,
	Thunder_Rage,
	Koopa_Tea,
	Turtley_Leaf,
	Koopasta,
	Koopa_Bun,
	Spicy_Pasta,
	Omelette_Meal,
	Mushroom,
	Shroom_Fry,
	Shroom_Crepe,
	Shroom_Cake,
	Shroom_Steak,
	Shroom_Roast,
	Shooting_Star,
	Gold_Bar,
	Gold_Bar_x_3,
	Life_Shroom,
	Dizzy_Dial,
	Shroom_Broth,
	Ice_Storm,
	Coconut_Bomb,
	Coco_Candy,
	Spite_Pouch,
	Mistake,
	Dried_Shroom,
	Inn_Coupon,
	Choco_Cake,
	Trial_Stew,
	Slow_Shroom,
	Gradual_Syrup,
	Super_Shroom,
	HP_Drain,
	Tasty_Tonic,
	Stopwatch,
	Spaghetti,
	Inky_Sauce,
	Whacka_Bump,
	Horsetail,
	Repel_Cape,
	Boos_Sheet,
	Power_Punch,
	Keel_Mango,
	Poison_Shroom,
	Dried_Bouquet,
	Mystery,
	Zess_Cookie,
	Zess_Special,
	Zess_Dynamite,
	Zess_Tea,
	Zess_Dinner,
	Zess_Deluxe,
	Zess_Frappe,
	Sleepy_Sheep,
	Love_Pudding,
	Honey_Candy,
	Honey_Shroom,
	Honey_Super,
	Honey_Ultra,
	Honey_Syrup,
	Egg_Bomb,
	Volt_Shroom,
	Electro_Pop,
	Peach_Tart,
	Peachy_Peach,
	Fire_Pop,
	Fire_Flower,
	Mystic_Egg,
	Mr_Softener,
	Fruit_Parfait,
	Fresh_Juice,
	Healthy_Salad,
	Meteor_Meal,
	Hot_Dog,
	Ruin_Powder,
	Mango_Delight,
	Mini_Mr_Mini,
	Mousse_Cake,
	Maple_Shroom,
	Maple_Super,
	Maple_Ultra,
	Maple_Syrup,
	Fried_Egg,
	Heartful_Cake,
	Coconut,
	Snow_Bunny,
	Earth_Quake,
	Hot_Sauce,
	Jelly_Shroom,
	Jelly_Super,
	Jelly_Ultra,
	Jelly_Candy,
	Jammin_Jelly,
	Fresh_Pasta
};

enum Type_Sort {
	Mushroom_t,
	Super_Shroom_t,
	Ultra_Shroom_t,
	Life_Shroom_t,
	Slow_Shroom_t,
	Dried_Shroom_t,
	Honey_Syrup_t,
	Maple_Syrup_t,
	Jammin_Jelly_t,
	Gradual_Syrup_t,
	Tasty_Tonic_t,
	POW_Block_t,
	Fire_Flower_t,
	Ice_Storm_t,
	Earth_Quake_t,
	Thunder_Bolt_t,
	Thunder_Rage_t,
	Shooting_Star_t,
	Volt_Shroom_t,
	Repel_Cape_t,
	Boos_Sheet_t,
	Ruin_Powder_t,
	Sleepy_Sheep_t,
	Dizzy_Dial_t,
	Stopwatch_t,
	Power_Punch_t,
	Mini_Mr_Mini_t,
	Courage_Shell_t,
	Mr_Softener_t,
	HP_Drain_t,
	Point_Swap_t,
	Fright_Mask_t,
	Mystery_t,
	Inn_Coupon_t,
	Gold_Bar_t,
	Gold_Bar_x_3_t,
	Whacka_Bump_t,
	Hot_Dog_t,
	Coconut_t,
	Dried_Bouquet_t,
	Mystic_Egg_t,
	Golden_Leaf_t,
	Keel_Mango_t,
	Fresh_Pasta_t,
	Cake_Mix_t,
	Hot_Sauce_t,
	Turtley_Leaf_t,
	Horsetail_t,
	Peachy_Peach_t,
	Spite_Pouch_t,
	Shroom_Fry_t,
	Shroom_Roast_t,
	Shroom_Steak_t,
	Honey_Shroom_t,
	Maple_Shroom_t,
	Jelly_Shroom_t,
	Honey_Super_t,
	Maple_Super_t,
	Jelly_Super_t,
	Honey_Ultra_t,
	Maple_Ultra_t,
	Jelly_Ultra_t,
	Zess_Dinner_t,
	Zess_Special_t,
	Zess_Deluxe_t,
	Spaghetti_t,
	Koopasta_t,
	Spicy_Pasta_t,
	Ink_Pasta_t,
	Spicy_Soup_t,
	Fried_Egg_t,
	Omelette_Meal_t,
	Koopa_Bun_t,
	Healthy_Salad_t,
	Meteor_Meal_t,
	Couples_Cake_t,
	Mousse_Cake_t,
	Shroom_Cake_t,
	Choco_Cake_t,
	Heartful_Cake_t,
	Fruit_Parfait_t,
	Mango_Delight_t,
	Love_Pudding_t,
	Zess_Cookie_t,
	Shroom_Crepe_t,
	Peach_Tart_t,
	Koopa_Tea_t,
	Zess_Tea_t,
	Shroom_Broth_t,
	Fresh_Juice_t,
	Inky_Sauce_t,
	Icicle_Pop_t,
	Zess_Frappe_t,
	Snow_Bunny_t,
	Coco_Candy_t,
	Honey_Candy_t,
	Jelly_Candy_t,
	Electro_Pop_t,
	Fire_Pop_t,
	Space_Food_t,
	Poison_Shroom_t,
	Trial_Stew_t,
	Courage_Meal_t,
	Coconut_Bomb_t,
	Egg_Bomb_t,
	Zess_Dynamite_t,
	Mistake_t
};

struct Item {
	enum Alpha_Sort a_key;
	enum Type_Sort t_key;
};

struct ItemName {
	enum Alpha_Sort a_key;
	char *name;
};

// Returns 1 if the inventories are the same. Return 0 if they are different
int compareInventories(struct Item *inv1, struct Item *inv2);

int itemInDependentIndices(int index, int *dependentIndices, int numDependentIndices);

// Count the number of nulls in the inventory that occur before maxIndex
int countNullsInInventory(struct Item *inventory, int minIndex, int maxIndex);

// Returns the index of an item in the inventory. -1 if not found
int indexOfItemInInventory(struct Item *inventory, struct Item item);

// Count the number of non-NULL and non-BLOCKED items
int countItemsInInventory(struct Item *inventory);

// Copy inventory to a new pointer
void copyInventory(struct Item* newInventory, struct Item* oldInventory);

// Return the string name for a particular item
char *getItemName(enum Alpha_Sort a_key);

// Return a pointer to an array of length 21 with each index i containing a variable length j which tracks the frameloss to navigate to the jth index in an inventory of size i
int **getInventoryFrames();

// Determine if a particular item is in your inventory
int itemInInventory(enum Alpha_Sort a_key, struct Item *inventory);

// I don't believe we need this
/*struct Type_Sort getTypeKey (Alpha_Sort a_key);*/

// Return array of Item structs for the start of cooking recipes
struct Item *getStartingInventory();

// Get all item data
struct Item getItem(enum Alpha_Sort a_key);

#endif
