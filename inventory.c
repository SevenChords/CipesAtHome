#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inventory.h>

#define INVENTORY_MAX_SIZE 21
#define NUM_ITEMS 107

static const char *ITEM_STRING[] = {
	"POW_Block",
	"Icicle_Pop",
	"Fright_Mask",
	"Spicy_Soup",
	"Ink_Pasta",
	"Couples_Cake",
	"Point_Swap",
	"Space_Food",
	"Ultra_Shroom",
	"Golden_Leaf",
	"Cake_Mix",
	"Courage_Shell",
	"Courage_Meal",
	"Thunder_Bolt",
	"Thunder_Rage",
	"Koopa_Tea",
	"Turtley_Leaf",
	"Koopasta",
	"Koopa_Bun",
	"Spicy_Pasta",
	"Omelette_Meal",
	"Mushroom",
	"Shroom_Fry",
	"Shroom_Crepe",
	"Shroom_Cake",
	"Shroom_Steak",
	"Shroom_Roast",
	"Shooting_Star",
	"Gold_Bar",
	"Gold_Bar_x_3",
	"Life_Shroom",
	"Dizzy_Dial",
	"Shroom_Broth",
	"Ice_Storm",
	"Coconut_Bomb",
	"Coco_Candy",
	"Spite_Pouch",
	"Mistake",
	"Dried_Shroom",
	"Inn_Coupon",
	"Choco_Cake",
	"Trial_Stew",
	"Slow_Shroom",
	"Gradual_Syrup",
	"Super_Shroom",
	"HP_Drain",
	"Tasty_Tonic",
	"Stopwatch",
	"Spaghetti",
	"Inky_Sauce",
	"Whacka_Bump",
	"Horsetail",
	"Repel_Cape",
	"Boos_Sheet",
	"Power_Punch",
	"Keel_Mango",
	"Poison_Shroom",
	"Dried_Bouquet",
	"Mystery",
	"Zess_Cookie",
	"Zess_Special",
	"Zess_Dynamite",
	"Zess_Tea",
	"Zess_Dinner",
	"Zess_Deluxe",
	"Zess_Frappe",
	"Sleepy_Sheep",
	"Love_Pudding",
	"Honey_Candy",
	"Honey_Shroom",
	"Honey_Super",
	"Honey_Ultra",
	"Honey_Syrup",
	"Egg_Bomb",
	"Volt_Shroom",
	"Electro_Pop",
	"Peach_Tart",
	"Peachy_Peach",
	"Fire_Pop",
	"Fire_Flower",
	"Mystic_Egg",
	"Mr_Softener",
	"Fruit_Parfait",
	"Fresh_Juice",
	"Healthy_Salad",
	"Meteor_Meal",
	"Hot_Dog",
	"Ruin_Powder",
	"Mango_Delight",
	"Mini_Mr_Mini",
	"Mousse_Cake",
	"Maple_Shroom",
	"Maple_Super",
	"Maple_Ultra",
	"Maple_Syrup",
	"Fried_Egg",
	"Heartful_Cake",
	"Coconut",
	"Snow_Bunny",
	"Earth_Quake",
	"Hot_Sauce",
	"Jelly_Shroom",
	"Jelly_Super",
	"Jelly_Ultra",
	"Jelly_Candy",
	"Jammin_Jelly",
	"Fresh_Pasta"
};

struct Items items [] = {
	{POW_Block_a, POW_Block_t, "POW_Block"},
	{Icicle_Pop_a, Icicle_Pop_t, "Icicle_Pop"},
	{Fright_Mask_a, Fright_Mask_t, "Fright_Mask"},
	{Spicy_Soup_a, Spicy_Soup_t, "Spicy_Soup"},
	{Ink_Pasta_a, Ink_Pasta_t, "Ink_Pasta"},
	{Couples_Cake_a, Couples_Cake_t, "Couples_Cake"},
	{Point_Swap_a, Point_Swap_t, "Point_Swap"},
	{Space_Food_a, Space_Food_t, "Space_Food"},
	{Ultra_Shroom_a, Ultra_Shroom_t, "Ultra_Shroom"},
	{Golden_Leaf_a, Golden_Leaf_t, "Golden_Leaf"},
	{Cake_Mix_a, Cake_Mix_t, "Cake_Mix"},
	{Courage_Shell_a, Courage_Shell_t, "Courage_Shell"},
	{Courage_Meal_a, Courage_Meal_t, "Courage_Meal"},
	{Thunder_Bolt_a, Thunder_Bolt_t, "Thunder_Bolt"},
	{Thunder_Rage_a, Thunder_Rage_t, "Thunder_Rage"},
	{Koopa_Tea_a, Koopa_Tea_t, "Koopa_Tea"},
	{Turtley_Leaf_a, Turtley_Leaf_t, "Turtley_Leaf"},
	{Koopasta_a, Koopasta_t, "Koopasta"},
	{Koopa_Bun_a, Koopa_Bun_t, "Koopa_Bun"},
	{Spicy_Pasta_a, Spicy_Pasta_t, "Spicy_Pasta"},
	{Omelette_Meal_a, Omelette_Meal_t, "Omelette_Meal"},
	{Mushroom_a, Mushroom_t, "Mushroom"},
	{Shroom_Fry_a, Shroom_Fry_t, "Shroom_Fry"},
	{Shroom_Crepe_a, Shroom_Crepe_t, "Shroom_Crepe"},
	{Shroom_Cake_a, Shroom_Cake_t, "Shroom_Cake"},
	{Shroom_Steak_a, Shroom_Steak_t, "Shroom_Steak"},
	{Shroom_Roast_a, Shroom_Roast_t, "Shroom_Roast"},
	{Shooting_Star_a, Shooting_Star_t, "Shooting_Star"},
	{Gold_Bar_a, Gold_Bar_t, "Gold_Bar"},
	{Gold_Bar_x_3_a, Gold_Bar_x_3_t, "Gold_Bar_x_3"},
	{Life_Shroom_a, Life_Shroom_t, "Life_Shroom"},
	{Dizzy_Dial_a, Dizzy_Dial_t, "Dizzy_Dial"},
	{Shroom_Broth_a, Shroom_Broth_t, "Shroom_Broth"},
	{Ice_Storm_a, Ice_Storm_t, "Ice_Storm"},
	{Coconut_Bomb_a, Coconut_Bomb_t, "Coconut_Bomb"},
	{Coco_Candy_a, Coco_Candy_t, "Coco_Candy"},
	{Spite_Pouch_a, Spite_Pouch_t, "Spite_Pouch"},
	{Mistake_a, Mistake_t, "Mistake"},
	{Dried_Shroom_a, Dried_Shroom_t, "Dried_Shroom"},
	{Inn_Coupon_a, Inn_Coupon_t, "Inn_Coupon"},
	{Choco_Cake_a, Choco_Cake_t, "Choco_Cake"},
	{Trial_Stew_a, Trial_Stew_t, "Trial_Stew"},
	{Slow_Shroom_a, Slow_Shroom_t, "Slow_Shroom"},
	{Gradual_Syrup_a, Gradual_Syrup_t, "Gradual_Syrup"},
	{Super_Shroom_a, Super_Shroom_t, "Super_Shroom"},
	{HP_Drain_a, HP_Drain_t, "HP_Drain"},
	{Tasty_Tonic_a, Tasty_Tonic_t, "Tasty_Tonic"},
	{Stopwatch_a, Stopwatch_t, "Stopwatch"},
	{Spaghetti_a, Spaghetti_t, "Spaghetti"},
	{Inky_Sauce_a, Inky_Sauce_t, "Inky_Sauce"},
	{Whacka_Bump_a, Whacka_Bump_t, "Whacka_Bump"},
	{Horsetail_a, Horsetail_t, "Horsetail"},
	{Repel_Cape_a, Repel_Cape_t, "Repel_Cape"},
	{Boos_Sheet_a, Boos_Sheet_t, "Boos_Sheet"},
	{Power_Punch_a, Power_Punch_t, "Power_Punch"},
	{Keel_Mango_a, Keel_Mango_t, "Keel_Mango"},
	{Poison_Shroom_a, Poison_Shroom_t, "Poison_Shroom"},
	{Dried_Bouquet_a, Dried_Bouquet_t, "Dried_Bouquet"},
	{Mystery_a, Mystery_t, "Mystery"},
	{Zess_Cookie_a, Zess_Cookie_t, "Zess_Cookie"},
	{Zess_Special_a, Zess_Special_t, "Zess_Special"},
	{Zess_Dynamite_a, Zess_Dynamite_t, "Zess_Dynamite"},
	{Zess_Tea_a, Zess_Tea_t, "Zess_Tea"},
	{Zess_Dinner_a, Zess_Dinner_t, "Zess_Dinner"},
	{Zess_Deluxe_a, Zess_Deluxe_t, "Zess_Deluxe"},
	{Zess_Frappe_a, Zess_Frappe_t, "Zess_Frappe"},
	{Sleepy_Sheep_a, Sleepy_Sheep_t, "Sleepy_Sheep"},
	{Love_Pudding_a, Love_Pudding_t, "Love_Pudding"},
	{Honey_Candy_a, Honey_Candy_t, "Honey_Candy"},
	{Honey_Shroom_a, Honey_Shroom_t, "Honey_Shroom"},
	{Honey_Super_a, Honey_Super_t, "Honey_Super"},
	{Honey_Ultra_a, Honey_Ultra_t, "Honey_Ultra"},
	{Honey_Syrup_a, Honey_Syrup_t, "Honey_Syrup"},
	{Egg_Bomb_a, Egg_Bomb_t, "Egg_Bomb"},
	{Volt_Shroom_a, Volt_Shroom_t, "Volt_Shroom"},
	{Electro_Pop_a, Electro_Pop_t, "Electro_Pop"},
	{Peach_Tart_a, Peach_Tart_t, "Peach_Tart"},
	{Peachy_Peach_a, Peachy_Peach_t, "Peachy_Peach"},
	{Fire_Pop_a, Fire_Pop_t, "Fire_Pop"},
	{Fire_Flower_a, Fire_Flower_t, "Fire_Flower"},
	{Mystic_Egg_a, Mystic_Egg_t, "Mystic_Egg"},
	{Mr_Softener_a, Mr_Softener_t, "Mr_Softener"},
	{Fruit_Parfait_a, Fruit_Parfait_t, "Fruit_Parfait"},
	{Fresh_Juice_a, Fresh_Juice_t, "Fresh_Juice"},
	{Healthy_Salad_a, Healthy_Salad_t, "Healthy_Salad"},
	{Meteor_Meal_a, Meteor_Meal_t, "Meteor_Meal"},
	{Hot_Dog_a, Hot_Dog_t, "Hot_Dog"},
	{Ruin_Powder_a, Ruin_Powder_t, "Ruin_Powder"},
	{Mango_Delight_a, Mango_Delight_t, "Mango_Delight"},
	{Mini_Mr_Mini_a, Mini_Mr_Mini_t, "Mini_Mr_Mini"},
	{Mousse_Cake_a, Mousse_Cake_t, "Mousse_Cake"},
	{Maple_Shroom_a, Maple_Shroom_t, "Maple_Shroom"},
	{Maple_Super_a, Maple_Super_t, "Maple_Super"},
	{Maple_Ultra_a, Maple_Ultra_t, "Maple_Ultra"},
	{Maple_Syrup_a, Maple_Syrup_t, "Maple_Syrup"},
	{Fried_Egg_a, Fried_Egg_t, "Fried_Egg"},
	{Heartful_Cake_a, Heartful_Cake_t, "Heartful_Cake"},
	{Coconut_a, Coconut_t, "Coconut"},
	{Snow_Bunny_a, Snow_Bunny_t, "Snow_Bunny"},
	{Earth_Quake_a, Earth_Quake_t, "Earth_Quake"},
	{Hot_Sauce_a, Hot_Sauce_t, "Hot_Sauce"},
	{Jelly_Shroom_a, Jelly_Shroom_t, "Jelly_Shroom"},
	{Jelly_Super_a, Jelly_Super_t, "Jelly_Super"},
	{Jelly_Ultra_a, Jelly_Ultra_t, "Jelly_Ultra"},
	{Jelly_Candy_a, Jelly_Candy_t, "Jelly_Candy"},
	{Jammin_Jelly_a, Jammin_Jelly_t, "Jammin_Jelly"},
	{Fresh_Pasta_a, Fresh_Pasta_t, "Fresh_Pasta"}
};


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

int main()
{
	/* FOR TESTING getInventoryFrames
	int **inv_frames;
	inv_frames = getInventoryFrames();

	for (int i = 0; i <= INVENTORY_MAX_SIZE; i++) {
		for (int j = 0; j < i; j++) {
			int *frames = inv_frames[i];
			printf("Inventory size %d:\tIndex %d:\t%d\n", i, j, frames[j]);
		}
	}*/
	
	printf("Name\t\tAlpha\tType\n");
	
	/* FOR TESTING ITEM SORT RETRIEVAL
	for (int i = 0; i < NUM_ITEMS; i++) {
		char *name = items[i].name;
		enum Alpha_Sort a_key = items[i].a_key;
		enum Type_Sort t_key = items[i].t_key;
		
		printf("%s\t%d\t%d\n", name, a_key, t_key);
	}*/
	
	return 0;
}
