#include <stdlib.h>
#include <stdio.h>
#include "recipes.h"
#include <string.h>
#include "inventory.h"

#define NUM_RECIPES 58 // Including Dried Bouquet trade
#define NUM_ITEMS 107 // All listed items

/*-------------------------------------------------------------------
 * Function : parseCombo
 * Inputs	: int itemCount
 *			  enum Type_Sort		 item1
 *			  enum Type_Sort		 item2
 * Outputs	: struct ItemCombination combo
 *
 * Small helper function which takes two items and creates a combo struct.
 -------------------------------------------------------------------*/
struct ItemCombination parseCombo(int itemCount, enum Type_Sort item1, enum Type_Sort item2) {
	struct ItemCombination combo;
	combo = (struct ItemCombination) {itemCount, item1, item2};
	return combo;
}

/*-------------------------------------------------------------------
 * Function : getRecipeList
 * Inputs	:
 * Outputs	: struct Recipe *recipeList
 *
 * Hard-coded array which tracks all recipes, the number of combinations
 * for each recipe, and the items that make up each of those combos.
 -------------------------------------------------------------------*/
struct Recipe* getRecipeList() {
	struct Recipe* recipes = malloc(sizeof(struct Recipe) * NUM_RECIPES);

	// Begin hard-coding recipes

	////////// Shroom Fry //////////
	recipes[0].output = Shroom_Fry;
	recipes[0].countCombos = 3;
	recipes[0].combos = malloc(sizeof(struct ItemCombination) * recipes[0].countCombos);
	recipes[0].combos[0] = parseCombo(1, Mystery, -1);
	recipes[0].combos[1] = parseCombo(1, Poison_Shroom, -1);
	recipes[0].combos[2] = parseCombo(1, Volt_Shroom, -1);

	////////// Shroom Roast //////////
	recipes[1].output = Shroom_Roast;
	recipes[1].countCombos = 2;
	recipes[1].combos = malloc(sizeof(struct ItemCombination) * recipes[1].countCombos);
	recipes[1].combos[0] = parseCombo(1, Slow_Shroom, -1);
	recipes[1].combos[1] = parseCombo(1, Life_Shroom, -1);

	////////// Shroom Steak //////////
	recipes[2].output = Shroom_Steak;
	recipes[2].countCombos = 1;
	recipes[2].combos = malloc(sizeof(struct ItemCombination) * recipes[2].countCombos);
	recipes[2].combos[0] = parseCombo(1, Ultra_Shroom, -1);

	////////// Honey Shroom //////////
	recipes[3].output = Honey_Shroom;
	recipes[3].countCombos = 1;
	recipes[3].combos = malloc(sizeof(struct ItemCombination) * recipes[3].countCombos);
	recipes[3].combos[0] = parseCombo(1, Mystery, -1);

	////////// Maple Shroom //////////
	recipes[4].output = Maple_Shroom;
	recipes[4].countCombos = 2;
	recipes[4].combos = malloc(sizeof(struct ItemCombination) * recipes[4].countCombos);
	recipes[4].combos[0] = parseCombo(2, Volt_Shroom, Maple_Syrup);
	recipes[4].combos[1] = parseCombo(2, Slow_Shroom, Maple_Syrup);

	////////// Jelly Shroom //////////
	recipes[5].output = Jelly_Shroom;
	recipes[5].countCombos = 1;
	recipes[5].combos = malloc(sizeof(struct ItemCombination) * recipes[5].countCombos);
	recipes[5].combos[0] = parseCombo(2, Volt_Shroom, Jammin_Jelly);

	////////// Honey Super //////////
	recipes[6].output = Honey_Super;
	recipes[6].countCombos = 1;
	recipes[6].combos = malloc(sizeof(struct ItemCombination) * recipes[6].countCombos);
	recipes[6].combos[0] = parseCombo(2, Life_Shroom, Honey_Syrup);

	////////// Maple Super //////////
	recipes[7].output = Maple_Super;
	recipes[7].countCombos = 2;
	recipes[7].combos = malloc(sizeof(struct ItemCombination) * recipes[7].countCombos);
	recipes[7].combos[0] = parseCombo(2, Life_Shroom, Maple_Syrup);
	recipes[7].combos[1] = parseCombo(2, Jammin_Jelly, Slow_Shroom);

	////////// Jelly Super //////////
	recipes[8].output = Jelly_Super;
	recipes[8].countCombos = 1;
	recipes[8].combos = malloc(sizeof(struct ItemCombination) * recipes[8].countCombos);
	recipes[8].combos[0] = parseCombo(2, Life_Shroom, Jammin_Jelly);

	////////// Honey Ultra //////////
	recipes[9].output = Honey_Ultra;
	recipes[9].countCombos = 1;
	recipes[9].combos = malloc(sizeof(struct ItemCombination) * recipes[9].countCombos);
	recipes[9].combos[0] = parseCombo(2, Ultra_Shroom, Honey_Syrup);

	////////// Maple Ultra //////////
	recipes[10].output = Maple_Ultra;
	recipes[10].countCombos = 1;
	recipes[10].combos = malloc(sizeof(struct ItemCombination) * recipes[10].countCombos);
	recipes[10].combos[0] = parseCombo(2, Ultra_Shroom, Maple_Syrup);

	////////// Jelly Ultra //////////
	recipes[11].output = Jelly_Ultra;
	recipes[11].countCombos = 1;
	recipes[11].combos = malloc(sizeof(struct ItemCombination) * recipes[11].countCombos);
	recipes[11].combos[0] = parseCombo(2, Ultra_Shroom, Jammin_Jelly);

	////////// Zess Dinner //////////
	recipes[12].output = Zess_Dinner;
	recipes[12].countCombos = 1;
	recipes[12].combos = malloc(sizeof(struct ItemCombination) * recipes[12].countCombos);
	recipes[12].combos[0] = parseCombo(1, Mystery, -1);

	////////// Zess Special //////////
	recipes[13].output = Zess_Special;
	recipes[13].countCombos = 5;
	recipes[13].combos = malloc(sizeof(struct ItemCombination) * recipes[13].countCombos);
	recipes[13].combos[0] = parseCombo(2, Ultra_Shroom, Fire_Flower);
	recipes[13].combos[1] = parseCombo(2, Ultra_Shroom, Slow_Shroom);
	recipes[13].combos[2] = parseCombo(2, Healthy_Salad, Ink_Pasta);
	recipes[13].combos[3] = parseCombo(2, Healthy_Salad, Shroom_Roast);
	recipes[13].combos[4] = parseCombo(2, Healthy_Salad, Spicy_Pasta);

	////////// Zess Deluxe //////////
	recipes[14].output = Zess_Deluxe;
	recipes[14].countCombos = 1;
	recipes[14].combos = malloc(sizeof(struct ItemCombination) * recipes[14].countCombos);
	recipes[14].combos[0] = parseCombo(2, Shroom_Steak, Healthy_Salad);

	////////// Spaghetti //////////
	recipes[15].output = Spaghetti;
	recipes[15].countCombos = 1;
	recipes[15].combos = malloc(sizeof(struct ItemCombination) * recipes[15].countCombos);
	recipes[15].combos[0] = parseCombo(1, Mystery, -1);

	////////// Koopasta //////////
	recipes[16].output = Koopasta;
	recipes[16].countCombos = 1;
	recipes[16].combos = malloc(sizeof(struct ItemCombination) * recipes[16].countCombos);
	recipes[16].combos[0] = parseCombo(1, Mystery, -1);

	////////// Spicy Pasta //////////
	recipes[17].output = Spicy_Pasta;
	recipes[17].countCombos = 2;
	recipes[17].combos = malloc(sizeof(struct ItemCombination) * recipes[17].countCombos);
	recipes[17].combos[0] = parseCombo(2, Hot_Sauce, Spaghetti);
	recipes[17].combos[1] = parseCombo(2, Hot_Sauce, Koopasta);

	////////// Ink Pasta //////////
	recipes[18].output = Ink_Pasta;
	recipes[18].countCombos = 3;
	recipes[18].combos = malloc(sizeof(struct ItemCombination) * recipes[18].countCombos);
	recipes[18].combos[0] = parseCombo(2, Inky_Sauce, Spaghetti);
	recipes[18].combos[1] = parseCombo(2, Inky_Sauce, Koopasta);
	recipes[18].combos[2] = parseCombo(2, Inky_Sauce, Spicy_Pasta);

	////////// Spicy Soup //////////
	recipes[19].output = Spicy_Soup;
	recipes[19].countCombos = 4;
	recipes[19].combos = malloc(sizeof(struct ItemCombination) * recipes[19].countCombos);
	recipes[19].combos[0] = parseCombo(1, Mystery, -1);
	recipes[19].combos[1] = parseCombo(1, Fire_Flower, -1);
	recipes[19].combos[2] = parseCombo(1, Snow_Bunny, -1);
	recipes[19].combos[3] = parseCombo(1, Dried_Bouquet, -1);

	////////// Fried Egg //////////
	recipes[20].output = Fried_Egg;
	recipes[20].countCombos = 2;
	recipes[20].combos = malloc(sizeof(struct ItemCombination) * recipes[20].countCombos);
	recipes[20].combos[0] = parseCombo(1, Mystery, -1);
	recipes[20].combos[1] = parseCombo(1, Mystic_Egg, -1);

	////////// Omelette Meal //////////
	recipes[21].output = Omelette_Meal;
	recipes[21].countCombos = 2;
	recipes[21].combos = malloc(sizeof(struct ItemCombination) * recipes[21].countCombos);
	recipes[21].combos[0] = parseCombo(2, Mystic_Egg, Life_Shroom);
	recipes[21].combos[1] = parseCombo(2, Mystic_Egg, Ultra_Shroom);

	////////// Koopa Bun //////////
	recipes[22].output = Koopa_Bun;
	recipes[22].countCombos = 1;
	recipes[22].combos = malloc(sizeof(struct ItemCombination) * recipes[22].countCombos);
	recipes[22].combos[0] = parseCombo(2, Keel_Mango, Turtley_Leaf);

	////////// Healthy Salad //////////
	recipes[23].output = Healthy_Salad;
	recipes[23].countCombos = 1;
	recipes[23].combos = malloc(sizeof(struct ItemCombination) * recipes[23].countCombos);
	recipes[23].combos[0] = parseCombo(2, Turtley_Leaf, Golden_Leaf);

	////////// Meteor Meal //////////
	recipes[24].output = Meteor_Meal;
	recipes[24].countCombos = 3;
	recipes[24].combos = malloc(sizeof(struct ItemCombination) * recipes[24].countCombos);
	recipes[24].combos[0] = parseCombo(2, Shroom_Fry, Shooting_Star);
	recipes[24].combos[1] = parseCombo(2, Shroom_Roast, Shooting_Star);
	recipes[24].combos[2] = parseCombo(2, Shroom_Steak, Shooting_Star);

	////////// Couples Cake //////////
	recipes[25].output = Couples_Cake;
	recipes[25].countCombos = 1;
	recipes[25].combos = malloc(sizeof(struct ItemCombination) * recipes[25].countCombos);
	recipes[25].combos[0] = parseCombo(2, Snow_Bunny, Spicy_Soup);

	////////// Mousse Cake //////////
	recipes[26].output = Mousse_Cake;
	recipes[26].countCombos = 1;
	recipes[26].combos = malloc(sizeof(struct ItemCombination) * recipes[26].countCombos);
	recipes[26].combos[0] = parseCombo(1, Cake_Mix, -1);

	////////// Shroom Cake //////////
	recipes[27].output = Shroom_Cake;
	recipes[27].countCombos = 2;
	recipes[27].combos = malloc(sizeof(struct ItemCombination) * recipes[27].countCombos);
	recipes[27].combos[0] = parseCombo(2, Life_Shroom, Cake_Mix);
	recipes[27].combos[1] = parseCombo(2, Slow_Shroom, Cake_Mix);

	////////// Choco Cake //////////
	recipes[28].output = Choco_Cake;
	recipes[28].countCombos = 2;
	recipes[28].combos = malloc(sizeof(struct ItemCombination) * recipes[28].countCombos);
	recipes[28].combos[0] = parseCombo(2, Cake_Mix, Inky_Sauce);
	recipes[28].combos[1] = parseCombo(2, Mousse_Cake, Inky_Sauce);

	////////// Heartful Cake //////////
	recipes[29].output = Heartful_Cake;
	recipes[29].countCombos = 2;
	recipes[29].combos = malloc(sizeof(struct ItemCombination) * recipes[29].countCombos);
	recipes[29].combos[0] = parseCombo(2, Cake_Mix, Ruin_Powder);
	recipes[29].combos[1] = parseCombo(2, Peachy_Peach, Ruin_Powder);

	////////// Fruit Parfait //////////
	recipes[30].output = Fruit_Parfait;
	recipes[30].countCombos = 7;
	recipes[30].combos = malloc(sizeof(struct ItemCombination) * recipes[30].countCombos);
	recipes[30].combos[0] = parseCombo(2, Keel_Mango, Honey_Syrup);
	recipes[30].combos[1] = parseCombo(2, Keel_Mango, Jammin_Jelly);
	recipes[30].combos[2] = parseCombo(2, Keel_Mango, Maple_Syrup);
	recipes[30].combos[3] = parseCombo(2, Keel_Mango, Peachy_Peach);
	recipes[30].combos[4] = parseCombo(2, Peachy_Peach, Honey_Syrup);
	recipes[30].combos[5] = parseCombo(2, Peachy_Peach, Jammin_Jelly);
	recipes[30].combos[6] = parseCombo(2, Peachy_Peach, Maple_Syrup);

	////////// Mango Delight //////////
	recipes[31].output = Mango_Delight;
	recipes[31].countCombos = 1;
	recipes[31].combos = malloc(sizeof(struct ItemCombination) * recipes[31].countCombos);
	recipes[31].combos[0] = parseCombo(2, Keel_Mango, Cake_Mix);

	////////// Love Pudding //////////
	recipes[32].output = Love_Pudding;
	recipes[32].countCombos = 1;
	recipes[32].combos = malloc(sizeof(struct ItemCombination) * recipes[32].countCombos);
	recipes[32].combos[0] = parseCombo(2, Mystic_Egg, Mango_Delight);

	////////// Zess Cookie //////////
	recipes[33].output = Zess_Cookie;
	recipes[33].countCombos = 1;
	recipes[33].combos = malloc(sizeof(struct ItemCombination) * recipes[33].countCombos);
	recipes[33].combos[0] = parseCombo(1, Mystery, -1);

	////////// Shroom Crepe //////////
	recipes[34].output = Shroom_Crepe;
	recipes[34].countCombos = 1;
	recipes[34].combos = malloc(sizeof(struct ItemCombination) * recipes[34].countCombos);
	recipes[34].combos[0] = parseCombo(2, Ultra_Shroom, Cake_Mix);

	////////// Peach Tart //////////
	recipes[35].output = Peach_Tart;
	recipes[35].countCombos = 1;
	recipes[35].combos = malloc(sizeof(struct ItemCombination) * recipes[35].countCombos);
	recipes[35].combos[0] = parseCombo(2, Peachy_Peach, Cake_Mix);

	////////// Koopa Tea //////////
	recipes[36].output = Koopa_Tea;
	recipes[36].countCombos = 2;
	recipes[36].combos = malloc(sizeof(struct ItemCombination) * recipes[36].countCombos);
	recipes[36].combos[0] = parseCombo(1, Mystery, -1);
	recipes[36].combos[1] = parseCombo(1, Turtley_Leaf, -1);

	////////// Zess Tea //////////
	recipes[37].output = Zess_Tea;
	recipes[37].countCombos = 2;
	recipes[37].combos = malloc(sizeof(struct ItemCombination) * recipes[37].countCombos);
	recipes[37].combos[0] = parseCombo(1, Mystery, -1);
	recipes[37].combos[1] = parseCombo(1, Golden_Leaf, -1);

	////////// Shroom Broth //////////
	recipes[38].output = Shroom_Broth;
	recipes[38].countCombos = 3;
	recipes[38].combos = malloc(sizeof(struct ItemCombination) * recipes[38].countCombos);
	recipes[38].combos[0] = parseCombo(2, Golden_Leaf, Poison_Shroom);
	recipes[38].combos[1] = parseCombo(2, Golden_Leaf, Slow_Shroom);
	recipes[38].combos[2] = parseCombo(2, Turtley_Leaf, Slow_Shroom);

	////////// Fresh Juice //////////
	recipes[39].output = Fresh_Juice;
	recipes[39].countCombos = 6;
	recipes[39].combos = malloc(sizeof(struct ItemCombination) * recipes[39].countCombos);
	recipes[39].combos[0] = parseCombo(1, Mystery, -1);
	recipes[39].combos[1] = parseCombo(1, Honey_Syrup, -1);
	recipes[39].combos[2] = parseCombo(1, Jammin_Jelly, -1);
	recipes[39].combos[3] = parseCombo(1, Keel_Mango, -1);
	recipes[39].combos[4] = parseCombo(1, Maple_Syrup, -1);
	recipes[39].combos[5] = parseCombo(1, Peachy_Peach, -1);

	////////// Inky Sauce //////////
	recipes[40].output = Inky_Sauce;
	recipes[40].countCombos = 5;
	recipes[40].combos = malloc(sizeof(struct ItemCombination) * recipes[40].countCombos);
	recipes[40].combos[0] = parseCombo(2, Hot_Sauce, Fresh_Juice);
	recipes[40].combos[1] = parseCombo(2, Hot_Sauce, Koopa_Tea);
	recipes[40].combos[2] = parseCombo(2, Hot_Sauce, Turtley_Leaf);
	recipes[40].combos[3] = parseCombo(2, Hot_Sauce, Zess_Tea);
	recipes[40].combos[4] = parseCombo(2, Hot_Sauce, Shroom_Broth);

	////////// Icicle Pop //////////
	recipes[41].output = Icicle_Pop;
	recipes[41].countCombos = 1;
	recipes[41].combos = malloc(sizeof(struct ItemCombination) * recipes[41].countCombos);
	recipes[41].combos[0] = parseCombo(2, Honey_Syrup, Ice_Storm);

	////////// Zess Frappe //////////
	recipes[42].output = Zess_Frappe;
	recipes[42].countCombos = 2;
	recipes[42].combos = malloc(sizeof(struct ItemCombination) * recipes[42].countCombos);
	recipes[42].combos[0] = parseCombo(2, Ice_Storm, Maple_Syrup);
	recipes[42].combos[1] = parseCombo(2, Ice_Storm, Jammin_Jelly);

	////////// Snow Bunny //////////
	recipes[43].output = Snow_Bunny;
	recipes[43].countCombos = 1;
	recipes[43].combos = malloc(sizeof(struct ItemCombination) * recipes[43].countCombos);
	recipes[43].combos[0] = parseCombo(2, Ice_Storm, Golden_Leaf);

	////////// Coco Candy //////////
	recipes[44].output = Coco_Candy;
	recipes[44].countCombos = 1;
	recipes[44].combos = malloc(sizeof(struct  ItemCombination) * recipes[44].countCombos);
	recipes[44].combos[0] = parseCombo(2, Cake_Mix, Coconut);

	////////// Honey Candy //////////
	recipes[45].output = Honey_Candy;
	recipes[45].countCombos = 1;
	recipes[45].combos = malloc(sizeof(struct ItemCombination) * recipes[45].countCombos);
	recipes[45].combos[0] = parseCombo(2, Honey_Syrup, Cake_Mix);

	////////// Jelly Candy //////////
	recipes[46].output = Jelly_Candy;
	recipes[46].countCombos = 1;
	recipes[46].combos = malloc(sizeof(struct ItemCombination) * recipes[46].countCombos);
	recipes[46].combos[0] = parseCombo(2, Jammin_Jelly, Cake_Mix);

	////////// Electro Pop //////////
	recipes[47].output = Electro_Pop;
	recipes[47].countCombos = 1;
	recipes[47].combos = malloc(sizeof(struct ItemCombination) * recipes[47].countCombos);
	recipes[47].combos[0] = parseCombo(2, Cake_Mix, Volt_Shroom);

	////////// Fire Pop //////////
	recipes[48].output = Fire_Pop;
	recipes[48].countCombos = 2;
	recipes[48].combos = malloc(sizeof(struct ItemCombination) * recipes[48].countCombos);
	recipes[48].combos[0] = parseCombo(2, Cake_Mix, Fire_Flower);
	recipes[48].combos[1] = parseCombo(2, Cake_Mix, Hot_Sauce);

	////////// Space Food //////////
	recipes[49].output = Space_Food;
	recipes[49].countCombos = 55;
	recipes[49].combos = malloc(sizeof(struct ItemCombination) * recipes[49].countCombos);
	recipes[49].combos[0] = parseCombo(2, Dried_Bouquet, Cake_Mix);
	recipes[49].combos[1] = parseCombo(2, Dried_Bouquet, Choco_Cake);
	recipes[49].combos[2] = parseCombo(2, Dried_Bouquet, Coco_Candy);
	recipes[49].combos[3] = parseCombo(2, Dried_Bouquet, Coconut);
	recipes[49].combos[4] = parseCombo(2, Dried_Bouquet, Couples_Cake);
	recipes[49].combos[5] = parseCombo(2, Dried_Bouquet, Dried_Shroom);
	recipes[49].combos[6] = parseCombo(2, Dried_Bouquet, Egg_Bomb);
	recipes[49].combos[7] = parseCombo(2, Dried_Bouquet, Electro_Pop);
	recipes[49].combos[8] = parseCombo(2, Dried_Bouquet, Fire_Pop);
	recipes[49].combos[9] = parseCombo(2, Dried_Bouquet, Fruit_Parfait);
	recipes[49].combos[10] = parseCombo(2, Dried_Bouquet, Golden_Leaf);
	recipes[49].combos[11] = parseCombo(2, Dried_Bouquet, Healthy_Salad);
	recipes[49].combos[12] = parseCombo(2, Dried_Bouquet, Heartful_Cake);
	recipes[49].combos[13] = parseCombo(2, Dried_Bouquet, Honey_Candy);
	recipes[49].combos[14] = parseCombo(2, Dried_Bouquet, Honey_Shroom);
	recipes[49].combos[15] = parseCombo(2, Dried_Bouquet, Honey_Super);
	recipes[49].combos[16] = parseCombo(2, Dried_Bouquet, Honey_Ultra);
	recipes[49].combos[17] = parseCombo(2, Dried_Bouquet, Hot_Dog);
	recipes[49].combos[18] = parseCombo(2, Dried_Bouquet, Ink_Pasta);
	recipes[49].combos[19] = parseCombo(2, Dried_Bouquet, Jelly_Candy);
	recipes[49].combos[20] = parseCombo(2, Dried_Bouquet, Jelly_Shroom);
	recipes[49].combos[21] = parseCombo(2, Dried_Bouquet, Jelly_Super);
	recipes[49].combos[22] = parseCombo(2, Dried_Bouquet, Jelly_Ultra);
	recipes[49].combos[23] = parseCombo(2, Dried_Bouquet, Keel_Mango);
	recipes[49].combos[24] = parseCombo(2, Dried_Bouquet, Koopa_Bun);
	recipes[49].combos[25] = parseCombo(2, Dried_Bouquet, Koopasta);
	recipes[49].combos[26] = parseCombo(2, Dried_Bouquet, Life_Shroom);
	recipes[49].combos[27] = parseCombo(2, Dried_Bouquet, Love_Pudding);
	recipes[49].combos[28] = parseCombo(2, Dried_Bouquet, Mango_Delight);
	recipes[49].combos[29] = parseCombo(2, Dried_Bouquet, Maple_Shroom);
	recipes[49].combos[30] = parseCombo(2, Dried_Bouquet, Maple_Super);
	recipes[49].combos[31] = parseCombo(2, Dried_Bouquet, Maple_Ultra);
	recipes[49].combos[32] = parseCombo(2, Dried_Bouquet, Meteor_Meal);
	recipes[49].combos[33] = parseCombo(2, Dried_Bouquet, Mistake);
	recipes[49].combos[34] = parseCombo(2, Dried_Bouquet, Mousse_Cake);
	recipes[49].combos[35] = parseCombo(2, Dried_Bouquet, Mystic_Egg);
	recipes[49].combos[36] = parseCombo(2, Dried_Bouquet, Omelette_Meal);
	recipes[49].combos[37] = parseCombo(2, Dried_Bouquet, Peach_Tart);
	recipes[49].combos[38] = parseCombo(2, Dried_Bouquet, Peachy_Peach);
	recipes[49].combos[39] = parseCombo(2, Dried_Bouquet, Poison_Shroom);
	recipes[49].combos[40] = parseCombo(2, Dried_Bouquet, Shroom_Cake);
	recipes[49].combos[41] = parseCombo(2, Dried_Bouquet, Shroom_Crepe);
	recipes[49].combos[42] = parseCombo(2, Dried_Bouquet, Shroom_Fry);
	recipes[49].combos[43] = parseCombo(2, Dried_Bouquet, Shroom_Roast);
	recipes[49].combos[44] = parseCombo(2, Dried_Bouquet, Shroom_Steak);
	recipes[49].combos[45] = parseCombo(2, Dried_Bouquet, Slow_Shroom);
	recipes[49].combos[46] = parseCombo(2, Dried_Bouquet, Spaghetti);
	recipes[49].combos[47] = parseCombo(2, Dried_Bouquet, Spicy_Pasta);
	recipes[49].combos[48] = parseCombo(2, Dried_Bouquet, Super_Shroom);
	recipes[49].combos[49] = parseCombo(2, Dried_Bouquet, Turtley_Leaf);
	recipes[49].combos[50] = parseCombo(2, Dried_Bouquet, Ultra_Shroom);
	recipes[49].combos[51] = parseCombo(2, Dried_Bouquet, Zess_Cookie);
	recipes[49].combos[52] = parseCombo(2, Dried_Bouquet, Zess_Deluxe);
	recipes[49].combos[53] = parseCombo(2, Dried_Bouquet, Zess_Dinner);
	recipes[49].combos[54] = parseCombo(2, Dried_Bouquet, Zess_Special);

	////////// Poison Shroom //////////
	recipes[50].output = Poison_Shroom;
	recipes[50].countCombos = 1;
	recipes[50].combos = malloc(sizeof(struct ItemCombination) * recipes[50].countCombos);
	recipes[50].combos[0] = parseCombo(2, Inky_Sauce, Slow_Shroom);

	////////// Trial Stew //////////
	recipes[51].output = Trial_Stew;
	recipes[51].countCombos = 1;
	recipes[51].combos = malloc(sizeof(struct ItemCombination) * recipes[51].countCombos);
	recipes[51].combos[0] = parseCombo(2, Poison_Shroom, Couples_Cake);

	////////// Courage Meal //////////
	recipes[52].output = Courage_Meal;
	recipes[52].countCombos = 3;
	recipes[52].combos = malloc(sizeof(struct ItemCombination) * recipes[52].countCombos);
	recipes[52].combos[0] = parseCombo(2, Courage_Shell, Zess_Dinner);
	recipes[52].combos[1] = parseCombo(2, Courage_Shell, Zess_Deluxe);
	recipes[52].combos[2] = parseCombo(2, Courage_Shell, Zess_Special);

	////////// Coconut Bomb //////////
	recipes[53].output = Coconut_Bomb;
	recipes[53].countCombos = 1;
	recipes[53].combos = malloc(sizeof(struct ItemCombination) * recipes[53].countCombos);
	recipes[53].combos[0] = parseCombo(2, Coconut, Fire_Flower);

	////////// Egg Bomb //////////
	recipes[54].output = Egg_Bomb;
	recipes[54].countCombos = 1;
	recipes[54].combos = malloc(sizeof(struct ItemCombination) * recipes[54].countCombos);
	recipes[54].combos[0] = parseCombo(1, Mystery, -1);

	////////// Zess Dynamite //////////
	recipes[55].output = Zess_Dynamite;
	recipes[55].countCombos = 1;
	recipes[55].combos = malloc(sizeof(struct ItemCombination) * recipes[55].countCombos);
	recipes[55].combos[0] = parseCombo(2, Egg_Bomb, Coconut_Bomb);

	////////// Dried Bouquet //////////
	recipes[56].output = Dried_Bouquet;
	recipes[56].countCombos = 1;
	recipes[56].combos = malloc(sizeof(struct ItemCombination) * recipes[56].countCombos);
	recipes[56].combos[0] = parseCombo(2, Hot_Dog, Mousse_Cake);

	////////// Mistake //////////
	recipes[57].output = Mistake;
	recipes[57].countCombos = 63;
	recipes[57].combos = malloc(sizeof(struct ItemCombination) * recipes[57].countCombos);
	recipes[57].combos[0] = parseCombo(1, Shroom_Roast, -1);
	recipes[57].combos[1] = parseCombo(1, Shroom_Steak, -1);
	recipes[57].combos[2] = parseCombo(1, Honey_Shroom, -1);
	recipes[57].combos[3] = parseCombo(1, Maple_Shroom, -1);
	recipes[57].combos[4] = parseCombo(1, Jelly_Super, -1);
	recipes[57].combos[5] = parseCombo(1, Honey_Ultra, -1);
	recipes[57].combos[6] = parseCombo(1, Maple_Ultra, -1);
	recipes[57].combos[7] = parseCombo(1, Jelly_Ultra, -1);
	recipes[57].combos[8] = parseCombo(1, Honey_Ultra, -1);
	recipes[57].combos[9] = parseCombo(1, Maple_Ultra, -1);
	recipes[57].combos[10] = parseCombo(1, Jelly_Ultra, -1);
	recipes[57].combos[11] = parseCombo(1, Zess_Dinner, -1);
	recipes[57].combos[12] = parseCombo(1, Zess_Special, -1);
	recipes[57].combos[13] = parseCombo(1, Zess_Deluxe, -1);
	recipes[57].combos[14] = parseCombo(1, Spaghetti, -1);
	recipes[57].combos[15] = parseCombo(1, Koopasta, -1);
	recipes[57].combos[16] = parseCombo(1, Spicy_Pasta, -1);
	recipes[57].combos[17] = parseCombo(1, Ink_Pasta, -1);
	recipes[57].combos[18] = parseCombo(1, Spicy_Soup, -1);
	recipes[57].combos[19] = parseCombo(1, Fried_Egg, -1);
	recipes[57].combos[20] = parseCombo(1, Omelette_Meal, -1);
	recipes[57].combos[21] = parseCombo(1, Koopa_Bun, -1);
	recipes[57].combos[22] = parseCombo(1, Healthy_Salad, -1);
	recipes[57].combos[23] = parseCombo(1, Meteor_Meal, -1);
	recipes[57].combos[24] = parseCombo(1, Couples_Cake, -1);
	recipes[57].combos[25] = parseCombo(1, Mousse_Cake, -1);
	recipes[57].combos[26] = parseCombo(1, Shroom_Cake, -1);
	recipes[57].combos[27] = parseCombo(1, Choco_Cake, -1);
	recipes[57].combos[28] = parseCombo(1, Heartful_Cake, -1);
	recipes[57].combos[29] = parseCombo(1, Fruit_Parfait, -1);
	recipes[57].combos[30] = parseCombo(1, Mango_Delight, -1);
	recipes[57].combos[31] = parseCombo(1, Love_Pudding, -1);
	recipes[57].combos[32] = parseCombo(1, Zess_Cookie, -1);
	recipes[57].combos[33] = parseCombo(1, Shroom_Crepe, -1);
	recipes[57].combos[34] = parseCombo(1, Peach_Tart, -1);
	recipes[57].combos[35] = parseCombo(1, Koopa_Tea, -1);
	recipes[57].combos[36] = parseCombo(1, Zess_Tea, -1);
	recipes[57].combos[37] = parseCombo(1, Shroom_Broth, -1);
	recipes[57].combos[38] = parseCombo(1, Fresh_Juice, -1);
	recipes[57].combos[39] = parseCombo(1, Inky_Sauce, -1);
	recipes[57].combos[40] = parseCombo(1, Icicle_Pop, -1);
	recipes[57].combos[41] = parseCombo(1, Zess_Frappe, -1);
	recipes[57].combos[42] = parseCombo(1, Coco_Candy, -1);
	recipes[57].combos[43] = parseCombo(1, Honey_Candy, -1);
	recipes[57].combos[44] = parseCombo(1, Jelly_Candy, -1);
	recipes[57].combos[45] = parseCombo(1, Electro_Pop, -1);
	recipes[57].combos[46] = parseCombo(1, Fire_Pop, -1);
	recipes[57].combos[47] = parseCombo(1, Space_Food, -1);
	recipes[57].combos[48] = parseCombo(1, Trial_Stew, -1);
	recipes[57].combos[49] = parseCombo(1, Courage_Meal, -1);
	recipes[57].combos[50] = parseCombo(1, Coconut_Bomb, -1);
	recipes[57].combos[51] = parseCombo(1, Egg_Bomb, -1);
	recipes[57].combos[52] = parseCombo(1, Zess_Dynamite, -1);
	recipes[57].combos[53] = parseCombo(1, Courage_Shell, -1);
	recipes[57].combos[54] = parseCombo(1, Hot_Dog, -1);
	recipes[57].combos[55] = parseCombo(1, Ice_Storm, -1);
	recipes[57].combos[56] = parseCombo(1, Mystery, -1);
	recipes[57].combos[57] = parseCombo(1, Ruin_Powder, -1);
	recipes[57].combos[58] = parseCombo(1, Shooting_Star, -1);
	recipes[57].combos[59] = parseCombo(1, Shroom_Fry, -1);
	recipes[57].combos[60] = parseCombo(1, Tasty_Tonic, -1);
	recipes[57].combos[61] = parseCombo(1, Thunder_Bolt, -1);
	recipes[57].combos[62] = parseCombo(1, Thunder_Rage, -1);

	return recipes;
}

/*-------------------------------------------------------------------
 * Function : getRecipeList
 * Inputs	: enum Type_Sort item
 * Outputs	: int			 index
 *
 * Triple ternary operators to determine the index of a particular recipe
 * in the recipeList. This exploits the fact that all recipes (besides
 * the fake Dried Bouquet recipe) are adjacent to one another in the
 * order listed in recipeList.
 -------------------------------------------------------------------*/
int getIndexOfRecipe(enum Type_Sort item) {
	// Conveniently, all recipes are stored as the same "type",
	// so their t_key's are adjacent to one another
	return item == Dried_Bouquet ? 56
		: item == Mistake ? 57
		: item  < Shroom_Fry ? -1
		: item - Shroom_Fry;
}

/*-------------------------------------------------------------------
 * Function : copyDependentRecipes
 * Inputs	: int *newDependentRecipes
 *			  int *dependentRecipes
 *
 * A simple memcpy to duplicate a previous instance of dependentRecipes.
 -------------------------------------------------------------------*/
void copyDependentRecipes(int *newDependentRecipes, int *dependentRecipes) {
	memcpy((void *)newDependentRecipes, (void *)dependentRecipes, sizeof(int) * NUM_RECIPES);
}

/*-------------------------------------------------------------------
 * Function : checkRecipe
 * Inputs	: struct itemCombination combo
 *			  int					 *makeableItems
 *			  int					 *outputsCreated
 *			  int					 *dependentRecipes
 *			  struct Recipe			 *recipeList
 * Outputs	: 1 if the recipe can be fulfilled, 0 otherwise.
 *
 * A recursive function which checks to see if a particular recipe can
 * still be created based off of an array of items which are known to
 * be creatable.
 -------------------------------------------------------------------*/
// Returns 1 if true, 0 if false
int checkRecipe(struct ItemCombination combo, int *makeableItems, const int * const outputsCreated, int *dependentRecipes, struct Recipe *recipeList) {
	// Determine if the recipe items can still be fulfilled
	for (int i = 0; i < combo.numItems; i++) {
		enum Type_Sort ingredient = i == 0 ? combo.item1 : combo.item2;
		// Check if we already have the item or know we can make it
		if (makeableItems[ingredient]) {
			continue;
		}

		int recipeIndex = getIndexOfRecipe(ingredient);

		if (recipeIndex == -1) {
			// The item cannot ever be created
			return 0;
		}

		// Check if it hasn't been made and doesn't depend on any item
		if (outputsCreated[recipeIndex] || dependentRecipes[recipeIndex]) {
			// The item cannot be produced due to the current history
			return 0;
		}

		dependentRecipes[recipeIndex] = 1;

		// Recurse on all recipes that can make this item
		for (int j = 0; j < recipeList[recipeIndex].countCombos; ++j) {
			struct ItemCombination newRecipe = recipeList[recipeIndex].combos[j];
			if (checkRecipe(newRecipe, makeableItems, outputsCreated, dependentRecipes, recipeList)) {
				// Don't explore this item in the future.
				makeableItems[ingredient] = 1;
				break;
			}
		}

		dependentRecipes[recipeIndex] = 0;
		if (!makeableItems[ingredient]) {
			// The item cannot be produced with the current inventory
			return 0;
		}
	}

	return 1;
}

/*-------------------------------------------------------------------
 * Function : placeInventoryInMakeableItems
 * Inputs	: int			   *makeableItems
 *			  struct Inventory inventory
 *
 * This function iterates over the entire inventory and marks those items
 * as makeable in the makeableItems array.
 -------------------------------------------------------------------*/
 void placeInventoryInMakeableItems(int *makeableItems, struct Inventory inventory) {
	for (size_t i = inventory.nulls; i < inventory.length; ++i) {
		makeableItems[inventory.inventory[i]] = 1;
	}
}

/*-------------------------------------------------------------------
 * Function : stateOK
 * Inputs	: struct Inventory inventory
 *			  int			   *outputsCreated
 *			  struct Recipe	   *recipeList
 * Outputs	: 1 if we can still make all remaining recipes with the
 *			  current inventory. Else, return 0
 *
 * This function iterates over all recipes, skips over any that have
 * been already created, and calls checkRecipe to see if each remaining
 * recipe can still be fulfilled at some point in the roadmap.
 -------------------------------------------------------------------*/
int stateOK(struct Inventory inventory, const int * const outputsCreated, struct Recipe *recipeList) {
	// With the given inventory, can the remaining recipes be fulfilled?

	// If Chapter 5 has not been done, verify that Thunder Rage is in the inventory
	if (!outputsCreated[getIndexOfRecipe(Dried_Bouquet)] && indexOfItemInInventory(inventory, Thunder_Rage) == -1) {
		return 0;
	}

	int makeableItems[NUM_ITEMS] = {0};

	placeInventoryInMakeableItems(makeableItems, inventory);

	// If Chapter 5 has not been done, add the items it gives
	if (outputsCreated[getIndexOfRecipe(Dried_Bouquet)] == 0) {
		makeableItems[Keel_Mango] = 1;
		makeableItems[Coconut] = 1;
	 	makeableItems[Dried_Bouquet] = 1;
		makeableItems[Courage_Shell] = 1;
	}

	// List of items to not try to make
	// Once we're done exploring the current recipe, unset it in the array
	int dependentRecipes[NUM_RECIPES] = {0};

	int outputsLeft[NUM_RECIPES];

	int startRecipe = 0;
	int endRecipe = 0;

	// Build a list of only those recipes we have not yet made
	// This loop is unrolled, treat the comments as if they apply to all
	// iterations. With a loop this short, saving the add and cmp instructions
	// saves a huge amount of time.
	// Set the current end+1th member of the list to the current index
	// This one won't get read yet
	outputsLeft[endRecipe] = 0;
	// If outputsCreated at the current index is 0, this will advance the end
	// of the list to the next index, preventing the previously set value
	// from being overwritten and allowing it to be read at the end
	// If it's 1, then this will be a no-op and the index at end+1 will be
	// overwritten again
	endRecipe += 1 ^ outputsCreated[0];
	// Loop continues
	outputsLeft[endRecipe] = 1;
	endRecipe += 1 ^ outputsCreated[1];
	outputsLeft[endRecipe] = 2;
	endRecipe += 1 ^ outputsCreated[2];
	outputsLeft[endRecipe] = 3;
	endRecipe += 1 ^ outputsCreated[3];
	outputsLeft[endRecipe] = 4;
	endRecipe += 1 ^ outputsCreated[4];
	outputsLeft[endRecipe] = 5;
	endRecipe += 1 ^ outputsCreated[5];
	outputsLeft[endRecipe] = 6;
	endRecipe += 1 ^ outputsCreated[6];
	outputsLeft[endRecipe] = 7;
	endRecipe += 1 ^ outputsCreated[7];
	outputsLeft[endRecipe] = 8;
	endRecipe += 1 ^ outputsCreated[8];
	outputsLeft[endRecipe] = 9;
	endRecipe += 1 ^ outputsCreated[9];
	outputsLeft[endRecipe] = 10;
	endRecipe += 1 ^ outputsCreated[10];
	outputsLeft[endRecipe] = 11;
	endRecipe += 1 ^ outputsCreated[11];
	outputsLeft[endRecipe] = 12;
	endRecipe += 1 ^ outputsCreated[12];
	outputsLeft[endRecipe] = 13;
	endRecipe += 1 ^ outputsCreated[13];
	outputsLeft[endRecipe] = 14;
	endRecipe += 1 ^ outputsCreated[14];
	outputsLeft[endRecipe] = 15;
	endRecipe += 1 ^ outputsCreated[15];
	outputsLeft[endRecipe] = 16;
	endRecipe += 1 ^ outputsCreated[16];
	outputsLeft[endRecipe] = 17;
	endRecipe += 1 ^ outputsCreated[17];
	outputsLeft[endRecipe] = 18;
	endRecipe += 1 ^ outputsCreated[18];
	outputsLeft[endRecipe] = 19;
	endRecipe += 1 ^ outputsCreated[19];
	outputsLeft[endRecipe] = 20;
	endRecipe += 1 ^ outputsCreated[20];
	outputsLeft[endRecipe] = 21;
	endRecipe += 1 ^ outputsCreated[21];
	outputsLeft[endRecipe] = 22;
	endRecipe += 1 ^ outputsCreated[22];
	outputsLeft[endRecipe] = 23;
	endRecipe += 1 ^ outputsCreated[23];
	outputsLeft[endRecipe] = 24;
	endRecipe += 1 ^ outputsCreated[24];
	outputsLeft[endRecipe] = 25;
	endRecipe += 1 ^ outputsCreated[25];
	outputsLeft[endRecipe] = 26;
	endRecipe += 1 ^ outputsCreated[26];
	outputsLeft[endRecipe] = 27;
	endRecipe += 1 ^ outputsCreated[27];
	outputsLeft[endRecipe] = 28;
	endRecipe += 1 ^ outputsCreated[28];
	outputsLeft[endRecipe] = 29;
	endRecipe += 1 ^ outputsCreated[29];
	outputsLeft[endRecipe] = 30;
	endRecipe += 1 ^ outputsCreated[30];
	outputsLeft[endRecipe] = 31;
	endRecipe += 1 ^ outputsCreated[31];
	outputsLeft[endRecipe] = 32;
	endRecipe += 1 ^ outputsCreated[32];
	outputsLeft[endRecipe] = 33;
	endRecipe += 1 ^ outputsCreated[33];
	outputsLeft[endRecipe] = 34;
	endRecipe += 1 ^ outputsCreated[34];
	outputsLeft[endRecipe] = 35;
	endRecipe += 1 ^ outputsCreated[35];
	outputsLeft[endRecipe] = 36;
	endRecipe += 1 ^ outputsCreated[36];
	outputsLeft[endRecipe] = 37;
	endRecipe += 1 ^ outputsCreated[37];
	outputsLeft[endRecipe] = 38;
	endRecipe += 1 ^ outputsCreated[38];
	outputsLeft[endRecipe] = 39;
	endRecipe += 1 ^ outputsCreated[39];
	outputsLeft[endRecipe] = 40;
	endRecipe += 1 ^ outputsCreated[40];
	outputsLeft[endRecipe] = 41;
	endRecipe += 1 ^ outputsCreated[41];
	outputsLeft[endRecipe] = 42;
	endRecipe += 1 ^ outputsCreated[42];
	outputsLeft[endRecipe] = 43;
	endRecipe += 1 ^ outputsCreated[43];
	outputsLeft[endRecipe] = 44;
	endRecipe += 1 ^ outputsCreated[44];
	outputsLeft[endRecipe] = 45;
	endRecipe += 1 ^ outputsCreated[45];
	outputsLeft[endRecipe] = 46;
	endRecipe += 1 ^ outputsCreated[46];
	outputsLeft[endRecipe] = 47;
	endRecipe += 1 ^ outputsCreated[47];
	outputsLeft[endRecipe] = 48;
	endRecipe += 1 ^ outputsCreated[48];
	outputsLeft[endRecipe] = 49;
	endRecipe += 1 ^ outputsCreated[49];
	outputsLeft[endRecipe] = 50;
	endRecipe += 1 ^ outputsCreated[50];
	outputsLeft[endRecipe] = 51;
	endRecipe += 1 ^ outputsCreated[51];
	outputsLeft[endRecipe] = 52;
	endRecipe += 1 ^ outputsCreated[52];
	outputsLeft[endRecipe] = 53;
	endRecipe += 1 ^ outputsCreated[53];
	outputsLeft[endRecipe] = 54;
	endRecipe += 1 ^ outputsCreated[54];
	outputsLeft[endRecipe] = 55;
	endRecipe += 1 ^ outputsCreated[55];
	outputsLeft[endRecipe] = 56;
	endRecipe += 1 ^ outputsCreated[56];
	outputsLeft[endRecipe] = 57;
	endRecipe += 1 ^ outputsCreated[57];

	// Iterate through all output items that haven't been created
	for (int currentRecipe = startRecipe; currentRecipe < endRecipe; currentRecipe++) {

		// Clear the dependentIndices array, specify that recipe #i is dependent
		dependentRecipes[outputsLeft[currentRecipe]] = 1;
		// Check if any recipe to make the item can be fulfilled
		int makeable = 0;
		for (int j = 0; j < recipeList[outputsLeft[currentRecipe]].countCombos; j++) {
			if (checkRecipe(recipeList[outputsLeft[currentRecipe]].combos[j], makeableItems, outputsCreated, dependentRecipes, recipeList) == 1) {
				// Stop looking for recipes to make the item
				makeableItems[recipeList[outputsLeft[currentRecipe]].output] = 1;
				makeable = 1;
				break;
			}
		}

		// The item cannot be fulfilled
		if (makeable == 0) {
			return 0;
		}

		dependentRecipes[outputsLeft[currentRecipe]] = 0;
	}

	// All remaining outputs can still be fulfilled
	return 1;
}
