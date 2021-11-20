#ifndef RECIPES_H
#define RECIPES_H

#include <stdbool.h>

// These model Paper Mario TTYD game behavior. Do not edit these.
#define NUM_RECIPES 58      // Including Chapter 5 representation and Dried Bouquet trade

typedef bool outputCreatedArray_t[NUM_RECIPES];

void copyDependentRecipes(int *newDependentRecipes, const int *dependentRecipes);

#endif
