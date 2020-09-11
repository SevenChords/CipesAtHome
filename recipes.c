#include <stdlib.h>
#include <stdio.h>
#include "recipes.h"

#define NUM_RECIPES 58 // Including Dried Bouquet trade

struct ItemCombination parseCombo(int itemCount, struct Item item1, struct Item item2) {
	struct ItemCombination combo;
	combo = (struct ItemCombination) {itemCount, item1, item2};
	return combo;
}

struct Recipe *getRecipeList() {
	struct Recipe *recipes = malloc(sizeof(struct Recipe) * NUM_RECIPES);
	
	// Begin hard-coding recipes
	
	////////// Shroom Fry //////////
	recipes[0].output = getItem(Shroom_Fry);
	recipes[0].countCombos = 5;
	recipes[0].combos = malloc(sizeof(struct ItemCombination) * recipes[0].countCombos);
	recipes[0].combos[0] = 	parseCombo(1, 	getItem(Dried_Shroom), (struct Item) {-1,-1});
	recipes[0].combos[1] = 	parseCombo(1, 	getItem(Mushroom), 	(struct Item) {-1,-1});
	recipes[0].combos[2] =		parseCombo(1, 	getItem(Poison_Shroom),(struct Item) {-1,-1});
	recipes[0].combos[3] = 	parseCombo(1, 	getItem(Super_Shroom), (struct Item) {-1,-1});
	recipes[0].combos[4] = 	parseCombo(1, 	getItem(Volt_Shroom),	(struct Item) {-1,-1});
	
	////////// Shroom Roast //////////
	recipes[1].output = getItem(Shroom_Roast);
	recipes[1].countCombos = 2;
	recipes[1].combos = malloc(sizeof(struct ItemCombination) * recipes[1].countCombos);
	recipes[1].combos[0] = 	parseCombo(1, 	getItem(Slow_Shroom),	(struct Item) {-1,-1});
	recipes[1].combos[1] = 	parseCombo(1, 	getItem(Life_Shroom),	(struct Item) {-1,-1});
	
	////////// Shroom Steak //////////
	recipes[2].output = getItem(Shroom_Steak);
	recipes[2].countCombos = 1;
	recipes[2].combos = malloc(sizeof(struct ItemCombination) * recipes[2].countCombos);
	recipes[2].combos[0] = 	parseCombo(1, 	getItem(Ultra_Shroom),	(struct Item) {-1,-1});
	
	////////// Honey Shroom //////////
	recipes[3].output = getItem(Honey_Shroom);
	recipes[3].countCombos = 1;
	recipes[3].combos = malloc(sizeof(struct ItemCombination) * recipes[3].countCombos);
	recipes[3].combos[0] = 	parseCombo(1, 	getItem(Mystery),	(struct Item) {-1,-1});
	
	////////// Maple Shroom //////////
	recipes[4].output = getItem(Maple_Shroom);
	recipes[4].countCombos = 3;
	recipes[4].combos = malloc(sizeof(struct ItemCombination) * recipes[4].countCombos);
	recipes[4].combos[0] = 	parseCombo(2, 	getItem(Mushroom),	getItem(Maple_Syrup));
	recipes[4].combos[1] = 	parseCombo(2, 	getItem(Volt_Shroom), 	getItem(Maple_Syrup));
	recipes[4].combos[2] = 	parseCombo(2, 	getItem(Slow_Shroom), 	getItem(Maple_Syrup));
	
	////////// Jelly Shroom //////////
	recipes[5].output = getItem(Jelly_Shroom);
	recipes[5].countCombos = 2;
	recipes[5].combos = malloc(sizeof(struct ItemCombination) * recipes[5].countCombos);
	recipes[5].combos[0] = 	parseCombo(2, 	getItem(Mushroom), 	getItem(Jammin_Jelly));
	recipes[5].combos[1] = 	parseCombo(2, 	getItem(Volt_Shroom), 	getItem(Jammin_Jelly));
	
	////////// Honey Super //////////
	recipes[6].output = getItem(Honey_Super);
	recipes[6].countCombos = 2;
	recipes[6].combos = malloc(sizeof(struct ItemCombination) * recipes[6].countCombos);
	recipes[6].combos[0] = 	parseCombo(2, 	getItem(Life_Shroom), 	getItem(Honey_Syrup));
	recipes[6].combos[1] = 	parseCombo(2, 	getItem(Super_Shroom), getItem(Honey_Syrup));
	
	////////// Maple Super //////////
	recipes[7].output = getItem(Maple_Super);
	recipes[7].countCombos = 3;
	recipes[7].combos = malloc(sizeof(struct ItemCombination) * recipes[7].countCombos);
	recipes[7].combos[0] = 	parseCombo(2, 	getItem(Super_Shroom),	getItem(Maple_Syrup));
	recipes[7].combos[1] = 	parseCombo(2, 	getItem(Life_Shroom), 	getItem(Maple_Syrup));
	recipes[7].combos[2] = 	parseCombo(2, 	getItem(Jammin_Jelly), getItem(Slow_Shroom));
	
	////////// Jelly Super //////////
	recipes[8].output = getItem(Jelly_Super);
	recipes[8].countCombos = 2;
	recipes[8].combos = malloc(sizeof(struct ItemCombination) * recipes[8].countCombos);
	recipes[8].combos[0] = 	parseCombo(2, 	getItem(Life_Shroom), 	getItem(Jammin_Jelly));
	recipes[8].combos[1] = 	parseCombo(2,	getItem(Super_Shroom), getItem(Jammin_Jelly));

	////////// Honey Ultra //////////
	recipes[9].output = getItem(Honey_Ultra);
	recipes[9].countCombos = 1;
	recipes[9].combos = malloc(sizeof(struct ItemCombination) * recipes[9].countCombos);
	recipes[9].combos[0] = 	parseCombo(2, 	getItem(Ultra_Shroom), getItem(Honey_Syrup));
	
	////////// Maple Ultra //////////
	recipes[10].output = getItem(Maple_Ultra);
	recipes[10].countCombos = 1;
	recipes[10].combos = malloc(sizeof(struct ItemCombination) * recipes[10].countCombos);
	recipes[10].combos[0] = 	parseCombo(2, 	getItem(Ultra_Shroom),  getItem(Maple_Syrup));
	
	////////// Jelly Ultra //////////
	recipes[11].output = getItem(Jelly_Ultra);
	recipes[11].countCombos = 1;
	recipes[11].combos = malloc(sizeof(struct ItemCombination) * recipes[11].countCombos);
	recipes[11].combos[0] = 	parseCombo(2, 	getItem(Ultra_Shroom), getItem(Jammin_Jelly));
	
	////////// Zess Dinner //////////
	recipes[12].output = getItem(Zess_Dinner);
	recipes[12].countCombos = 1;
	recipes[12].combos = malloc(sizeof(struct ItemCombination) * recipes[12].countCombos);
	recipes[12].combos[0] = 	parseCombo(1, 	getItem(Mystery), 	(struct Item) {-1,-1});
	
	////////// Zess Special //////////
	recipes[13].output = getItem(Zess_Special);
	recipes[13].countCombos = 5;
	recipes[13].combos = malloc(sizeof(struct ItemCombination) * recipes[13].countCombos);
	recipes[13].combos[0] = 	parseCombo(2,	getItem(Ultra_Shroom),	getItem(Fire_Flower));
	recipes[13].combos[1] = 	parseCombo(2,	getItem(Ultra_Shroom),	getItem(Slow_Shroom));
	recipes[13].combos[2] = 	parseCombo(2, 	getItem(Healthy_Salad),getItem(Ink_Pasta));
	recipes[13].combos[3] = 	parseCombo(2,	getItem(Healthy_Salad),getItem(Shroom_Roast));
	recipes[13].combos[4] =	parseCombo(2,	getItem(Healthy_Salad),getItem(Spicy_Pasta));
	
	////////// Zess Deluxe //////////
	recipes[14].output = getItem(Zess_Deluxe);
	recipes[14].countCombos = 1;
	recipes[14].combos = malloc(sizeof(struct ItemCombination) * recipes[14].countCombos);
	recipes[14].combos[0] = 	parseCombo(2,	getItem(Shroom_Steak),	getItem(Healthy_Salad));
	
	////////// Spaghetti //////////
	recipes[15].output = getItem(Spaghetti);
	recipes[15].countCombos = 1;
	recipes[15].combos = malloc(sizeof(struct ItemCombination) * recipes[15].countCombos);
	recipes[15].combos[0] = 	parseCombo(1,	getItem(Mystery),	(struct Item) {-1,-1});
	
	////////// Koopasta //////////
	recipes[16].output = getItem(Koopasta);
	recipes[16].countCombos = 1;
	recipes[16].combos = malloc(sizeof(struct ItemCombination) * recipes[16].countCombos);
	recipes[16].combos[0] =	parseCombo(1,	getItem(Mystery),	(struct Item) {-1,-1});
	
	////////// Spicy Pasta //////////
	recipes[17].output = getItem(Spicy_Pasta);
	recipes[17].countCombos = 2;
	recipes[17].combos = malloc(sizeof(struct ItemCombination) * recipes[17].countCombos);
	recipes[17].combos[0] = 	parseCombo(2,	getItem(Hot_Sauce),	getItem(Spaghetti));
	recipes[17].combos[1] = 	parseCombo(2,	getItem(Hot_Sauce),	getItem(Koopasta));
	
	////////// Ink Pasta //////////
	recipes[18].output = getItem(Ink_Pasta);
	recipes[18].countCombos = 3;
	recipes[18].combos = malloc(sizeof(struct ItemCombination) * recipes[18].countCombos);
	recipes[18].combos[0] = 	parseCombo(2,	getItem(Inky_Sauce),	getItem(Spaghetti));
	recipes[18].combos[1] = 	parseCombo(2,	getItem(Inky_Sauce),	getItem(Koopasta));
	recipes[18].combos[2] = 	parseCombo(2,	getItem(Inky_Sauce),	getItem(Spicy_Pasta));
	
	////////// Spicy Soup //////////
	recipes[19].output = getItem(Spicy_Soup);
	recipes[19].countCombos = 4;
	recipes[19].combos = malloc(sizeof(struct ItemCombination) * recipes[19].countCombos);
	recipes[19].combos[0] = 	parseCombo(1,	getItem(Mystery),	(struct Item) {-1,-1});
	recipes[19].combos[1] = 	parseCombo(1,	getItem(Fire_Flower),	(struct Item) {-1,-1});
	recipes[19].combos[2] = 	parseCombo(1,	getItem(Snow_Bunny),	(struct Item) {-1,-1});
	recipes[19].combos[3] = 	parseCombo(1,	getItem(Dried_Bouquet),(struct Item) {-1,-1});
	
	////////// Fried Egg //////////
	recipes[20].output = getItem(Fried_Egg);
	recipes[20].countCombos = 2;
	recipes[20].combos = malloc(sizeof(struct ItemCombination) * recipes[20].countCombos);
	recipes[20].combos[0] = 	parseCombo(1,	getItem(Mystery),	(struct Item) {-1,-1});
	recipes[20].combos[1] = 	parseCombo(1,	getItem(Mystic_Egg),	(struct Item) {-1,-1});
	
	////////// Omelette Meal //////////
	recipes[21].output = getItem(Omelette_Meal);
	recipes[21].countCombos = 4;
	recipes[21].combos = malloc(sizeof(struct ItemCombination) * recipes[21].countCombos);
	recipes[21].combos[0] = 	parseCombo(2,	getItem(Mystic_Egg),	getItem(Mushroom));
	recipes[21].combos[1] = 	parseCombo(2,	getItem(Mystic_Egg),	getItem(Super_Shroom));
	recipes[21].combos[2] = 	parseCombo(2,	getItem(Mystic_Egg),	getItem(Life_Shroom));
	recipes[21].combos[3] = 	parseCombo(2,	getItem(Mystic_Egg),	getItem(Ultra_Shroom));
	
	////////// Koopa Bun //////////
	recipes[22].output = getItem(Koopa_Bun);
	recipes[22].countCombos = 1;
	recipes[22].combos = malloc(sizeof(struct ItemCombination) * recipes[22].countCombos);
	recipes[22].combos[0] = 	parseCombo(2,	getItem(Keel_Mango),	getItem(Turtley_Leaf));
	
	////////// Healthy Salad //////////
	recipes[23].output = getItem(Healthy_Salad);
	recipes[23].countCombos = 1;
	recipes[23].combos = malloc(sizeof(struct ItemCombination) * recipes[23].countCombos);
	recipes[23].combos[0] = 	parseCombo(2,	getItem(Turtley_Leaf),	getItem(Golden_Leaf));

	////////// Meteor Meal //////////
	recipes[24].output = getItem(Meteor_Meal);
	recipes[24].countCombos = 3;
	recipes[24].combos = malloc(sizeof(struct ItemCombination) * recipes[24].countCombos);
	recipes[24].combos[0] = 	parseCombo(2,	getItem(Shroom_Fry),	getItem(Shooting_Star));
	recipes[24].combos[1] = 	parseCombo(2,	getItem(Shroom_Roast),	getItem(Shooting_Star));
	recipes[24].combos[2] = 	parseCombo(2,	getItem(Shroom_Steak),	getItem(Shooting_Star));

	////////// Couples Cake //////////
	recipes[25].output = getItem(Couples_Cake);
	recipes[25].countCombos = 1;
	recipes[25].combos = malloc(sizeof(struct ItemCombination) * recipes[25].countCombos);
	recipes[25].combos[0] = 	parseCombo(2,	getItem(Snow_Bunny), 	getItem(Spicy_Soup));
	
	////////// Mousse Cake //////////
	recipes[26].output = getItem(Mousse_Cake);
	recipes[26].countCombos = 1;
	recipes[26].combos = malloc(sizeof(struct ItemCombination) * recipes[26].countCombos);
	recipes[26].combos[0] =	parseCombo(1,	getItem(Cake_Mix), 	(struct Item)  {-1,-1});
	
	////////// Shroom Cake //////////
	recipes[27].output = getItem(Shroom_Cake);
	recipes[27].countCombos = 4;
	recipes[27].combos = malloc(sizeof(struct ItemCombination) * recipes[27].countCombos);
	recipes[27].combos[0] = 	parseCombo(2,	getItem(Mushroom),	getItem(Cake_Mix));
	recipes[27].combos[1] = 	parseCombo(2,	getItem(Super_Shroom),	getItem(Cake_Mix));
	recipes[27].combos[2] = 	parseCombo(2,	getItem(Life_Shroom),	getItem(Cake_Mix));
	recipes[27].combos[3] = 	parseCombo(2,	getItem(Slow_Shroom),	getItem(Cake_Mix));
	
	////////// Choco Cake //////////
	recipes[28].output = getItem(Choco_Cake);
	recipes[28].countCombos = 2;
	recipes[28].combos = malloc(sizeof(struct ItemCombination) * recipes[28].countCombos);
	recipes[28].combos[0] = 	parseCombo(2,	getItem(Cake_Mix),	getItem(Inky_Sauce));
	recipes[28].combos[1] = 	parseCombo(2,	getItem(Mousse_Cake),	getItem(Inky_Sauce));
	
	////////// Heartful Cake //////////
	recipes[29].output = getItem(Heartful_Cake);
	recipes[29].countCombos = 2;
	recipes[29].combos = malloc(sizeof(struct ItemCombination) * recipes[29].countCombos);
	recipes[29].combos[0] = 	parseCombo(2,	getItem(Cake_Mix),	getItem(Ruin_Powder));
	recipes[29].combos[1] = 	parseCombo(2,	getItem(Peachy_Peach),	getItem(Ruin_Powder));
	
	////////// Fruit Parfait //////////
	recipes[30].output = getItem(Fruit_Parfait);
	recipes[30].countCombos = 7;
	recipes[30].combos = malloc(sizeof(struct ItemCombination) * recipes[30].countCombos);
	recipes[30].combos[0] = 	parseCombo(2,	getItem(Keel_Mango),	getItem(Honey_Syrup));
	recipes[30].combos[1] = 	parseCombo(2,	getItem(Keel_Mango),	getItem(Jammin_Jelly));
	recipes[30].combos[2] = 	parseCombo(2,	getItem(Keel_Mango),	getItem(Maple_Syrup));
	recipes[30].combos[3] = 	parseCombo(2,	getItem(Keel_Mango),	getItem(Peachy_Peach));
	recipes[30].combos[4] = 	parseCombo(2,	getItem(Peachy_Peach),	getItem(Honey_Syrup));
	recipes[30].combos[5] = 	parseCombo(2,	getItem(Peachy_Peach),	getItem(Jammin_Jelly));
	recipes[30].combos[6] = 	parseCombo(2,	getItem(Peachy_Peach),	getItem(Maple_Syrup));
	
	////////// Mango Delight //////////
	recipes[31].output = getItem(Mango_Delight);
	recipes[31].countCombos = 1;
	recipes[31].combos = malloc(sizeof(struct ItemCombination) * recipes[31].countCombos);
	recipes[31].combos[0] = 	parseCombo(2,	getItem(Keel_Mango),	getItem(Cake_Mix));
	
	////////// Love Pudding //////////
	recipes[32].output = getItem(Love_Pudding);
	recipes[32].countCombos = 1;
	recipes[32].combos = malloc(sizeof(struct ItemCombination) * recipes[32].countCombos);
	recipes[32].combos[0] = 	parseCombo(2,	getItem(Mystic_Egg),	getItem(Mango_Delight));
	
	////////// Zess Cookie //////////
	recipes[33].output = getItem(Zess_Cookie);
	recipes[33].countCombos = 1;
	recipes[33].combos = malloc(sizeof(struct ItemCombination) * recipes[33].countCombos);
	recipes[33].combos[0] = 	parseCombo(1,	getItem(Mystery),	(struct Item) {-1,-1});
	
	////////// Shroom Crepe //////////
	recipes[34].output = getItem(Shroom_Crepe);
	recipes[34].countCombos = 1;
	recipes[34].combos = malloc(sizeof(struct ItemCombination) * recipes[34].countCombos);
	recipes[34].combos[0] = 	parseCombo(2, 	getItem(Ultra_Shroom),	getItem(Cake_Mix));

	////////// Peach Tart //////////
	recipes[35].output = getItem(Peach_Tart);
	recipes[35].countCombos = 1;
	recipes[35].combos = malloc(sizeof(struct ItemCombination) * recipes[35].countCombos);
	recipes[35].combos[0] = 	parseCombo(2,	getItem(Peachy_Peach),	getItem(Cake_Mix));

	////////// Koopa Tea //////////
	recipes[36].output = getItem(Koopa_Tea);
	recipes[36].countCombos = 2;
	recipes[36].combos = malloc(sizeof(struct ItemCombination) * recipes[36].countCombos);
	recipes[36].combos[0] = 	parseCombo(1,	getItem(Mystery),	(struct Item) {-1,-1});
	recipes[36].combos[1] = 	parseCombo(1,	getItem(Turtley_Leaf),	(struct Item) {-1,-1});
	
	////////// Zess Tea //////////
	recipes[37].output = getItem(Zess_Tea);
	recipes[37].countCombos = 2;
	recipes[37].combos = malloc(sizeof(struct ItemCombination) * recipes[37].countCombos);
	recipes[37].combos[0] = 	parseCombo(1,	getItem(Mystery),	(struct Item) {-1,-1});
	recipes[37].combos[1] = 	parseCombo(1,	getItem(Golden_Leaf),	(struct Item) {-1,-1});
	
	////////// Shroom Broth //////////
	recipes[38].output = getItem(Shroom_Broth);
	recipes[38].countCombos = 3;
	recipes[38].combos = malloc(sizeof(struct ItemCombination) * recipes[38].countCombos);
	recipes[38].combos[0] = 	parseCombo(2,	getItem(Golden_Leaf),	getItem(Poison_Shroom));
	recipes[38].combos[1] = 	parseCombo(2,	getItem(Golden_Leaf),	getItem(Slow_Shroom));
	recipes[38].combos[2] = 	parseCombo(2,	getItem(Turtley_Leaf),	getItem(Slow_Shroom));
	
	////////// Fresh Juice //////////
	recipes[39].output = getItem(Fresh_Juice);
	recipes[39].countCombos = 6;
	recipes[39].combos = malloc(sizeof(struct ItemCombination) * recipes[39].countCombos);
	recipes[39].combos[0] = 	parseCombo(1,	getItem(Mystery),	(struct Item) {-1,-1});
	recipes[39].combos[1] = 	parseCombo(1,	getItem(Honey_Syrup),	(struct Item) {-1,-1});
	recipes[39].combos[2] = 	parseCombo(1,	getItem(Jammin_Jelly),	(struct Item) {-1,-1});
	recipes[39].combos[3] = 	parseCombo(1,	getItem(Keel_Mango),	(struct Item) {-1,-1});
	recipes[39].combos[4] = 	parseCombo(1,	getItem(Maple_Syrup),	(struct Item) {-1,-1});
	recipes[39].combos[5] = 	parseCombo(1,	getItem(Peachy_Peach),	(struct Item) {-1,-1});
	
	////////// Inky Sauce //////////
	recipes[40].output = getItem(Inky_Sauce);
	recipes[40].countCombos = 5;
	recipes[40].combos = malloc(sizeof(struct ItemCombination) * recipes[40].countCombos);
	recipes[40].combos[0] = 	parseCombo(2,	getItem(Hot_Sauce),	getItem(Fresh_Juice));
	recipes[40].combos[1] = 	parseCombo(2,	getItem(Hot_Sauce),	getItem(Koopa_Tea));
	recipes[40].combos[2] = 	parseCombo(2,	getItem(Hot_Sauce),	getItem(Turtley_Leaf));
	recipes[40].combos[3] = 	parseCombo(2,	getItem(Hot_Sauce),	getItem(Zess_Tea));
	recipes[40].combos[4] = 	parseCombo(2,	getItem(Hot_Sauce),	getItem(Tasty_Tonic));
	
	////////// Icicle Pop //////////
	recipes[41].output = getItem(Icicle_Pop);
	recipes[41].countCombos = 1;
	recipes[41].combos = malloc(sizeof(struct ItemCombination) * recipes[41].countCombos);
	recipes[41].combos[0] = 	parseCombo(2,	getItem(Honey_Syrup),	getItem(Ice_Storm));
	
	////////// Zess Frappe //////////
	recipes[42].output = getItem(Zess_Frappe);
	recipes[42].countCombos = 2;
	recipes[42].combos = malloc(sizeof(struct ItemCombination) * recipes[42].countCombos);
	recipes[42].combos[0] = 	parseCombo(2,	getItem(Ice_Storm),	getItem(Maple_Syrup));
	recipes[42].combos[1] =	parseCombo(2,	getItem(Ice_Storm),	getItem(Jammin_Jelly));
	
	////////// Snow Bunny //////////
	recipes[43].output = getItem(Snow_Bunny);
	recipes[43].countCombos = 1;
	recipes[43].combos = malloc(sizeof(struct ItemCombination) * recipes[43].countCombos);
	recipes[43].combos[0] = 	parseCombo(2,	getItem(Ice_Storm),	getItem(Golden_Leaf));
	
	////////// Coco Candy //////////
	recipes[44].output = getItem(Coco_Candy);
	recipes[44].countCombos = 1;
	recipes[44].combos = malloc(sizeof(struct  ItemCombination) * recipes[44].countCombos);
	recipes[44].combos[0] = 	parseCombo(2,	getItem(Cake_Mix),	getItem(Coconut));
	
	////////// Honey Candy //////////
	recipes[45].output = getItem(Honey_Candy);
	recipes[45].countCombos = 1;
	recipes[45].combos = malloc(sizeof(struct ItemCombination) * recipes[45].countCombos);
	recipes[45].combos[0] = 	parseCombo(2,	getItem(Honey_Syrup),	getItem(Cake_Mix));
	
	////////// Jelly Candy //////////
	recipes[46].output = getItem(Jelly_Candy);
	recipes[46].countCombos = 1;
	recipes[46].combos = malloc(sizeof(struct ItemCombination) * recipes[46].countCombos);
	recipes[46].combos[0] = 	parseCombo(2,	getItem(Jammin_Jelly),	getItem(Cake_Mix));
	
	////////// Electro Pop //////////
	recipes[47].output = getItem(Electro_Pop);
	recipes[47].countCombos = 1;
	recipes[47].combos = malloc(sizeof(struct ItemCombination) * recipes[47].countCombos);
	recipes[47].combos[0] = 	parseCombo(2,	getItem(Cake_Mix),	getItem(Volt_Shroom));
	
	////////// Fire Pop //////////
	recipes[48].output = getItem(Fire_Pop);
	recipes[48].countCombos = 2;
	recipes[48].combos = malloc(sizeof(struct ItemCombination) * recipes[48].countCombos);
	recipes[48].combos[0] = 	parseCombo(2,	getItem(Cake_Mix),	getItem(Fire_Flower));
	recipes[48].combos[1] = 	parseCombo(2,	getItem(Cake_Mix),	getItem(Hot_Sauce));
	
	////////// Space Food //////////
	recipes[49].output = getItem(Space_Food);
	recipes[49].countCombos = 67;
	recipes[49].combos = malloc(sizeof(struct ItemCombination) * recipes[49].countCombos);
	recipes[49].combos[0] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Cake_Mix));
	recipes[49].combos[1] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Choco_Cake));
	recipes[49].combos[2] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Coco_Candy));
	recipes[49].combos[3] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Coconut));
	recipes[49].combos[4] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Couples_Cake));
	recipes[49].combos[5] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Dried_Shroom));
	recipes[49].combos[6] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Egg_Bomb));
	recipes[49].combos[7] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Electro_Pop));
	recipes[49].combos[8] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Fire_Pop));
	recipes[49].combos[9] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Fresh_Juice));
	recipes[49].combos[10] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Fried_Egg));
	recipes[49].combos[11] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Fruit_Parfait));
	recipes[49].combos[12] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Golden_Leaf));
	recipes[49].combos[13] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Healthy_Salad));
	recipes[49].combos[14] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Heartful_Cake));
	recipes[49].combos[15] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Honey_Candy));
	recipes[49].combos[16] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Honey_Shroom));
	recipes[49].combos[17] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Honey_Super));
	recipes[49].combos[18] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Honey_Ultra));
	recipes[49].combos[19] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Hot_Dog));
	recipes[49].combos[20] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Icicle_Pop));
	recipes[49].combos[21] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Ink_Pasta));
	recipes[49].combos[22] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Inky_Sauce));
	recipes[49].combos[23] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Jammin_Jelly));
	recipes[49].combos[24] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Jelly_Candy));
	recipes[49].combos[25] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Jelly_Shroom));
	recipes[49].combos[26] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Jelly_Super));
	recipes[49].combos[27] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Jelly_Ultra));
	recipes[49].combos[28] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Keel_Mango));
	recipes[49].combos[29] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Koopa_Bun));
	recipes[49].combos[30] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Koopa_Tea));
	recipes[49].combos[31] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Koopasta));
	recipes[49].combos[32] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Life_Shroom));
	recipes[49].combos[33] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Love_Pudding));
	recipes[49].combos[34] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Mango_Delight));
	recipes[49].combos[35] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Maple_Shroom));
	recipes[49].combos[36] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Maple_Super));
	recipes[49].combos[37] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Maple_Syrup));
	recipes[49].combos[38] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Maple_Ultra));
	recipes[49].combos[39] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Meteor_Meal));
	recipes[49].combos[40] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Mistake));
	recipes[49].combos[41] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Mousse_Cake));
	recipes[49].combos[42] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Mushroom));
	recipes[49].combos[43] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Mystic_Egg));
	recipes[49].combos[44] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Omelette_Meal));
	recipes[49].combos[45] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Peach_Tart));
	recipes[49].combos[46] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Peachy_Peach));
	recipes[49].combos[47] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Poison_Shroom));
	recipes[49].combos[48] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Shroom_Cake));
	recipes[49].combos[49] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Shroom_Crepe));
	recipes[49].combos[50] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Shroom_Fry));
	recipes[49].combos[51] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Shroom_Roast));
	recipes[49].combos[52] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Shroom_Steak));
	recipes[49].combos[53] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Slow_Shroom));
	recipes[49].combos[54] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Snow_Bunny));
	recipes[49].combos[55] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Spaghetti));
	recipes[49].combos[56] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Spicy_Pasta));
	recipes[49].combos[57] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Spicy_Soup));
	recipes[49].combos[58] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Super_Shroom));
	recipes[49].combos[59] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Turtley_Leaf));
	recipes[49].combos[60] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Ultra_Shroom));
	recipes[49].combos[61] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Zess_Cookie));
	recipes[49].combos[62] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Zess_Deluxe));
	recipes[49].combos[63] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Zess_Dinner));
	recipes[49].combos[64] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Zess_Frappe));
	recipes[49].combos[65] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Zess_Special));
	recipes[49].combos[66] = 	parseCombo(2,	getItem(Dried_Bouquet),getItem(Zess_Tea));
	
	////////// Poison Shroom //////////
	recipes[50].output = getItem(Poison_Shroom);
	recipes[50].countCombos = 2;
	recipes[50].combos = malloc(sizeof(struct ItemCombination) * recipes[50].countCombos);
	recipes[50].combos[0] = 	parseCombo(2,	getItem(Inky_Sauce),	getItem(Slow_Shroom));
	recipes[50].combos[1] = 	parseCombo(2,	getItem(Trial_Stew),	getItem(Dried_Bouquet));
	
	////////// Trial Stew //////////
	recipes[51].output = getItem(Trial_Stew);
	recipes[51].countCombos = 2;
	recipes[51].combos = malloc(sizeof(struct ItemCombination) * recipes[51].countCombos);
	recipes[51].combos[0] = 	parseCombo(2,	getItem(Poison_Shroom),getItem(Couples_Cake));
	recipes[51].combos[1] = 	parseCombo(2,	getItem(Thunder_Rage),	getItem(Thunder_Bolt));
	
	////////// Courage Meal //////////
	recipes[52].output = getItem(Courage_Meal);
	recipes[52].countCombos = 3;
	recipes[52].combos = malloc(sizeof(struct ItemCombination) * recipes[52].countCombos);
	recipes[52].combos[0] = 	parseCombo(2,	getItem(Courage_Shell),getItem(Zess_Dinner));
	recipes[52].combos[1] = 	parseCombo(2,	getItem(Courage_Shell),getItem(Zess_Deluxe));
	recipes[52].combos[2] = 	parseCombo(2,	getItem(Courage_Shell),getItem(Zess_Special));
	
	////////// Coconut Bomb //////////
	recipes[53].output = getItem(Coconut_Bomb);
	recipes[53].countCombos = 1;
	recipes[53].combos = malloc(sizeof(struct ItemCombination) * recipes[53].countCombos);
	recipes[53].combos[0] = 	parseCombo(2,	getItem(Coconut),	getItem(Fire_Flower));
	
	////////// Egg Bomb //////////
	recipes[54].output = getItem(Egg_Bomb);
	recipes[54].countCombos = 1;
	recipes[54].combos = malloc(sizeof(struct ItemCombination) * recipes[54].countCombos);
	recipes[54].combos[0] = 	parseCombo(1,	getItem(Mystery), 	(struct Item) {-1,-1});
	
	////////// Zess Dynamite //////////
	recipes[55].output = getItem(Zess_Dynamite);
	recipes[55].countCombos = 1;
	recipes[55].combos = malloc(sizeof(struct ItemCombination) * recipes[55].countCombos);
	recipes[55].combos[0] = 	parseCombo(2,	getItem(Egg_Bomb), 	getItem(Coconut_Bomb));
	
	////////// Dried Bouquet //////////
	recipes[56].output = getItem(Dried_Bouquet);
	recipes[56].countCombos = 1;
	recipes[56].combos = malloc(sizeof(struct ItemCombination) * recipes[56].countCombos);
	recipes[56].combos[0] = 	parseCombo(2,	getItem(Hot_Dog),	getItem(Mousse_Cake));
	
	////////// Mistake //////////
	recipes[57].output = getItem(Mistake);
	recipes[57].countCombos = 63;
	recipes[57].combos = malloc(sizeof(struct ItemCombination) * recipes[57].countCombos);
	recipes[57].combos[0] = 	parseCombo(1,	getItem(Shroom_Roast),	(struct Item) {-1,-1});
	recipes[57].combos[1] = 	parseCombo(1,	getItem(Shroom_Steak), (struct Item) {-1,-1});
	recipes[57].combos[2] = 	parseCombo(1,	getItem(Honey_Shroom), (struct Item) {-1,-1});
	recipes[57].combos[3] = 	parseCombo(1,	getItem(Maple_Shroom), (struct Item) {-1,-1});
	recipes[57].combos[4] = 	parseCombo(1,	getItem(Jelly_Super), 	(struct Item) {-1,-1});
	recipes[57].combos[5] = 	parseCombo(1,	getItem(Honey_Ultra),	(struct Item) {-1,-1});
	recipes[57].combos[6] = 	parseCombo(1,	getItem(Maple_Ultra), 	(struct Item) {-1,-1});
	recipes[57].combos[7] = 	parseCombo(1,	getItem(Jelly_Ultra), 	(struct Item) {-1,-1});
	recipes[57].combos[8] = 	parseCombo(1,	getItem(Honey_Ultra), 	(struct Item) {-1,-1});
	recipes[57].combos[9] = 	parseCombo(1,	getItem(Maple_Ultra), 	(struct Item) {-1,-1});
	recipes[57].combos[10] = 	parseCombo(1,	getItem(Jelly_Ultra), 	(struct Item) {-1,-1});
	recipes[57].combos[11] = 	parseCombo(1,	getItem(Zess_Dinner), 	(struct Item) {-1,-1});
	recipes[57].combos[12] = 	parseCombo(1,	getItem(Zess_Special), (struct Item) {-1,-1});
	recipes[57].combos[13] = 	parseCombo(1,	getItem(Zess_Deluxe), 	(struct Item) {-1,-1});
	recipes[57].combos[14] = 	parseCombo(1,	getItem(Spaghetti), 	(struct Item) {-1,-1});
	recipes[57].combos[15] = 	parseCombo(1,	getItem(Koopasta), 	(struct Item) {-1,-1});
	recipes[57].combos[16] = 	parseCombo(1,	getItem(Spicy_Pasta), 	(struct Item) {-1,-1});
	recipes[57].combos[17] = 	parseCombo(1,	getItem(Ink_Pasta), 	(struct Item) {-1,-1});
	recipes[57].combos[18] = 	parseCombo(1,	getItem(Spicy_Soup), 	(struct Item) {-1,-1});
	recipes[57].combos[19] = 	parseCombo(1,	getItem(Fried_Egg), 	(struct Item) {-1,-1});
	recipes[57].combos[20] = 	parseCombo(1,	getItem(Omelette_Meal),(struct Item) {-1,-1});
	recipes[57].combos[21] = 	parseCombo(1,	getItem(Koopa_Bun), 	(struct Item) {-1,-1});
	recipes[57].combos[22] = 	parseCombo(1,	getItem(Healthy_Salad),(struct Item) {-1,-1});
	recipes[57].combos[23] = 	parseCombo(1,	getItem(Meteor_Meal), 	(struct Item) {-1,-1});
	recipes[57].combos[24] = 	parseCombo(1,	getItem(Couples_Cake), (struct Item) {-1,-1});
	recipes[57].combos[25] = 	parseCombo(1,	getItem(Mousse_Cake), 	(struct Item) {-1,-1});
	recipes[57].combos[26] = 	parseCombo(1,	getItem(Shroom_Cake), 	(struct Item) {-1,-1});
	recipes[57].combos[27] = 	parseCombo(1,	getItem(Choco_Cake), 	(struct Item) {-1,-1});
	recipes[57].combos[28] = 	parseCombo(1,	getItem(Heartful_Cake),(struct Item) {-1,-1});
	recipes[57].combos[29] = 	parseCombo(1,	getItem(Fruit_Parfait),(struct Item) {-1,-1});
	recipes[57].combos[30] = 	parseCombo(1,	getItem(Mango_Delight),(struct Item) {-1,-1});
	recipes[57].combos[31] = 	parseCombo(1,	getItem(Love_Pudding), (struct Item) {-1,-1});
	recipes[57].combos[32] = 	parseCombo(1,	getItem(Zess_Cookie), 	(struct Item) {-1,-1});
	recipes[57].combos[33] = 	parseCombo(1,	getItem(Shroom_Crepe), (struct Item) {-1,-1});
	recipes[57].combos[34] = 	parseCombo(1,	getItem(Peach_Tart), 	(struct Item) {-1,-1});
	recipes[57].combos[35] = 	parseCombo(1,	getItem(Koopa_Tea), 	(struct Item) {-1,-1});
	recipes[57].combos[36] = 	parseCombo(1,	getItem(Zess_Tea), 	(struct Item) {-1,-1});
	recipes[57].combos[37] = 	parseCombo(1,	getItem(Shroom_Broth), (struct Item) {-1,-1});
	recipes[57].combos[38] = 	parseCombo(1,	getItem(Fresh_Juice), 	(struct Item) {-1,-1});
	recipes[57].combos[39] = 	parseCombo(1,	getItem(Inky_Sauce), 	(struct Item) {-1,-1});
	recipes[57].combos[40] = 	parseCombo(1,	getItem(Icicle_Pop), 	(struct Item) {-1,-1});
	recipes[57].combos[41] = 	parseCombo(1,	getItem(Zess_Frappe), 	(struct Item) {-1,-1});
	recipes[57].combos[42] = 	parseCombo(1,	getItem(Coco_Candy), 	(struct Item) {-1,-1});
	recipes[57].combos[43] = 	parseCombo(1,	getItem(Honey_Candy), 	(struct Item) {-1,-1});
	recipes[57].combos[44] = 	parseCombo(1,	getItem(Jelly_Candy), 	(struct Item) {-1,-1});
	recipes[57].combos[45] = 	parseCombo(1,	getItem(Electro_Pop), 	(struct Item) {-1,-1});
	recipes[57].combos[46] = 	parseCombo(1,	getItem(Fire_Pop), 	(struct Item) {-1,-1});
	recipes[57].combos[47] = 	parseCombo(1,	getItem(Space_Food), 	(struct Item) {-1,-1});
	recipes[57].combos[48] = 	parseCombo(1,	getItem(Trial_Stew), 	(struct Item) {-1,-1});
	recipes[57].combos[49] = 	parseCombo(1,	getItem(Courage_Meal), (struct Item) {-1,-1});
	recipes[57].combos[50] = 	parseCombo(1,	getItem(Coconut_Bomb), (struct Item) {-1,-1});
	recipes[57].combos[51] = 	parseCombo(1,	getItem(Egg_Bomb), 	(struct Item) {-1,-1});
	recipes[57].combos[52] = 	parseCombo(1,	getItem(Zess_Dynamite),(struct Item) {-1,-1});
	recipes[57].combos[53] = 	parseCombo(1,	getItem(Courage_Shell),(struct Item) {-1,-1});
	recipes[57].combos[54] = 	parseCombo(1,	getItem(Hot_Dog), 	(struct Item) {-1,-1});
	recipes[57].combos[55] = 	parseCombo(1,	getItem(Ice_Storm), 	(struct Item) {-1,-1});
	recipes[57].combos[56] = 	parseCombo(1,	getItem(Mystery), 	(struct Item) {-1,-1});
	recipes[57].combos[57] = 	parseCombo(1,	getItem(Ruin_Powder), 	(struct Item) {-1,-1});
	recipes[57].combos[58] = 	parseCombo(1,	getItem(Shooting_Star),(struct Item) {-1,-1});
	recipes[57].combos[59] = 	parseCombo(1,	getItem(Shroom_Fry), 	(struct Item) {-1,-1});
	recipes[57].combos[60] = 	parseCombo(1,	getItem(Tasty_Tonic), 	(struct Item) {-1,-1});
	recipes[57].combos[61] = 	parseCombo(1,	getItem(Thunder_Bolt), (struct Item) {-1,-1});
	recipes[57].combos[62] = 	parseCombo(1,	getItem(Thunder_Rage), (struct Item) {-1,-1});

	// TODO: Add log call
	return recipes;
}

void clearDependentIndices(int *dependentIndices, int length) {
	for (int j = 0; j < length; j++) {
		dependentIndices[j] = -1;
	}
}

int getIndexOfRecipe(struct Item item, struct Recipe *recipeList) {
	for (int i = 0; i < NUM_RECIPES; i++) {
		if (recipeList[i].output.a_key == item.a_key)
			return i;
	}
	return -1;
}

int itemInMakeableItems(struct Item item, struct Item *makeableItems, int makeableItemsLength) {
	for (int j = 0; j < makeableItemsLength; j++) {
		if (item.a_key == makeableItems[j].a_key) {
			return 1;
		}
	}
	return 0;
}

void copyDependentIndices(int *newDependentIndices, int *dependentIndices, int numDependentIndices) {
	for (int i = 0; i < numDependentIndices; i++) {
		newDependentIndices[i] = dependentIndices[i];
	}
}

// Returns 1 if true, 0 if false
int checkRecipe(struct ItemCombination combo, struct Item *makeableItems, int *outputsCreated, int *dependentIndices, int numDependentIndices, struct Recipe *recipeList, int makeableItemsLength) {
	// Determine if the recipe items can still be fulfilled
	for (int i = 0; i < 2; i++) {
		// If this is a 1-item recipe, ignore the second item
		if (i == 1 && combo.numItems == 1)
			continue;
		
		// Check if we already have the item or know we can make it
		if ((i == 0 && itemInMakeableItems(combo.item1, makeableItems, makeableItemsLength)) || (i == 1 && itemInMakeableItems(combo.item2, makeableItems, makeableItemsLength))) {
			continue;
		}
		
		int recipeIndex;
		if (i == 0)
			recipeIndex = getIndexOfRecipe(combo.item1, recipeList);
		else
			recipeIndex = getIndexOfRecipe(combo.item2, recipeList);

		if (recipeIndex == -1)
			// The item cannot ever be created
			return 0;
		
		// Check if it hasn't been made and doesn't depend on any item
		if (outputsCreated[recipeIndex] || itemInDependentIndices(recipeIndex, dependentIndices, numDependentIndices)) {
			// The item cannot be produced due to the current history
			return 0;
		}
		
		// Anything made for this item cannot depend on the item
		// Copy the dependentIndices array as to not cause lasting changes when we go back to remainingOutputsCanBeFulfilled
		int *newDependentIndices = malloc(sizeof(int) * 200);
		copyDependentIndices(newDependentIndices, dependentIndices, numDependentIndices);
		newDependentIndices[numDependentIndices] = recipeIndex;
		numDependentIndices++;
		
		// Recurse on all recipes that can make this item
		int canBeProduced = 0;
		for (int j = 0; j < recipeList[recipeIndex].countCombos; j++) {
			struct ItemCombination newRecipe = recipeList[recipeIndex].combos[j];
			if (checkRecipe(newRecipe, makeableItems, outputsCreated, newDependentIndices, numDependentIndices, recipeList, makeableItemsLength)) {
				if (i == 0)
					makeableItems[makeableItemsLength] = combo.item1;
				else
					makeableItems[makeableItemsLength] = combo.item2;
				makeableItemsLength++;
				canBeProduced = 1;
				break;
			}
		}
		
		free(newDependentIndices);
		if (!canBeProduced)
			// The item cannot be produced with the current inventory
			return 0;
	}
	
	return 1;
}

int remainingOutputsCanBeFulfilled(struct Item *inventory, int *outputsCreated, struct Recipe *recipeList) {
	// With the given inventory, can the remaining recipes be fulfilled?
	struct Item *makeableItems = malloc(sizeof(int) * (200));
	copyInventory(makeableItems, inventory);
	int numMakeableItems = 20;
	// If Chapter 5 has not been done, add the items it gives
	if (outputsCreated[getIndexOfRecipe(getItem(Dried_Bouquet), recipeList)] == 0) {
		makeableItems[20] = getItem(Keel_Mango);
		makeableItems[21] = getItem(Coconut);
		makeableItems[22] = getItem(Dried_Bouquet);
		makeableItems[23] = getItem(Courage_Shell);
		numMakeableItems+=4;
	}
	
	// Iterate through all output items that haven't been created
	int *dependentIndices = malloc(sizeof(int) * 200);
	int numDependentIndices;
	for (int i = 0; i < NUM_RECIPES; i++) {
		if (outputsCreated[i] == 1)
			continue;
			
		// List of items to not try to make
		// Clear the dependentIndices array and set [0] = i
		clearDependentIndices(dependentIndices, 200);
		dependentIndices[0] = i;
		numDependentIndices = 1;
		// Check if any recipe to make the item can be fulfilled
		int makeable = 0;
		for (int j = 0; j < recipeList[i].countCombos; j++) {
			if (checkRecipe(recipeList[i].combos[j], makeableItems, outputsCreated, dependentIndices, numDependentIndices, recipeList, numMakeableItems) == 1) {
				// Stop looking for recipes to make the item
				makeableItems[numMakeableItems] = recipeList[i].output;
				numMakeableItems++;
				makeable = 1;
				break;
			}
		}
		
		// The item cannot be fulfilled
		if (makeable == 0) {
			free(makeableItems);
			free(dependentIndices);
			return 0;
		}
	}
	// All remaining outputs can still be fulfilled
	free(makeableItems);
	free(dependentIndices);
	return 1;
}
