#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define NUM_RECIPES 58      // Including Chapter 5 representation and Dried Bouquet trade
#define INVENTORY_MAX_SIZE 21

typedef struct Result Result;
struct Result {
	int frames;
	int callNumber;
};

typedef struct Serial Serial;
struct Serial {
	uint8_t length;
	void* data;
};

typedef bool outputCreatedArray_t[NUM_RECIPES];

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
typedef enum Alpha_Sort Alpha_Sort;

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
typedef enum Type_Sort Type_Sort;

typedef struct Inventory Inventory;
struct Inventory {
	// The maximum value these should have is 20.
	// These are used in format strings, so we want fixed width, not potentially variable that uint_fast8_t can be
	uint8_t nulls;
	uint8_t length;
	Type_Sort inventory[20];
};

// Represent the action at a particular node in the roadmap
typedef enum Action Action;
enum Action {
	EBegin = 0,
	ECook = 1,
	ESort_Alpha_Asc = 2,
	ESort_Alpha_Des = 3,
	ESort_Type_Asc = 4,
	ESort_Type_Des = 5,
	ECh5 = 6
};

// Overall data pertaining to what we did at a particular point in the roadmap
typedef struct MoveDescription MoveDescription;
struct MoveDescription {
	Action action;		// Cook, sort, handle CH5,...
	void* data;				// This data may be either of type Cook, CH5, or NULL if we are just sorting
	int framesTaken;		// How many frames were wasted to perform this move
	int totalFramesTaken;	// Cummulative frame loss
};

typedef struct BranchPath BranchPath;
struct BranchPath {
	int moves;							// Represents how many nodes we've traversed down a particular branch (0 for root, 57 for leaf node)
	Inventory inventory;
	MoveDescription description;
	BranchPath* prev;
	BranchPath* next;
	outputCreatedArray_t outputCreated;					// Array of 58 items, true if item was produced, false if not; indexed by recipe ordering
	int numOutputsCreated;				// Number of valid outputCreated entries to eliminate a lengthy for-loop
	BranchPath** legalMoves;		// Represents possible next paths to take
	int numLegalMoves;
	int totalSorts;
	Serial serial;
};

// What do we do with the produced item after crafting a recipe?
enum HandleOutput {
	Toss,		// Toss the recipe itself
	Autoplace,	// The recipe is placed in an empty slot
	TossOther	// Toss a different item to make room
};

// Information pertaining to cooking a recipe
typedef struct Cook Cook;
struct Cook {
	int numItems;
	enum Type_Sort item1;
	int itemIndex1;
	enum Type_Sort item2;
	int itemIndex2;
	enum Type_Sort output;
	enum HandleOutput handleOutput;
	enum Type_Sort toss;
	int indexToss;
};

typedef struct ItemCombination ItemCombination;
struct ItemCombination {
	int numItems; // If set to 1, ignore item2
	Type_Sort item1;
	Type_Sort item2;
};

typedef struct Recipe Recipe;
struct Recipe {
	Type_Sort output;
	int countCombos;
	ItemCombination* combos; // Where there are countCombos different ways to cook output
};

// Information pertaining to the Chapter 5 intermission
typedef struct CH5 CH5;
struct CH5 {
	int indexDriedBouquet;	// index of item we toss to make room for Dried Bouquet
	int indexCoconut;		// index of item we toss to make room for Coconut
	enum Action ch5Sort;	// The sort type we perform after Coconut
	int indexKeelMango;		// index of item we toss to make room for Keel Mango
	int indexCourageShell;	// index of item we toss to make room for Courage Shell
	int indexThunderRage;	// index of Thunder Rage when we use it during Smorg (if 0-9, TR is removed)
	int lateSort;			// 0 - sort after Coconut, 1 - sort after Keel Mango
};

#endif