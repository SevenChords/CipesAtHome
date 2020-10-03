# CipesAtHome

## Overview
Welcome to CipesAtHome! This is a depth-first-search brute-forcer which implements different RNG-based methodologies to iterate down unique paths in the recipe tree in an attempt to discover the fastest known order of recipes. Upon discovery of the fastest known recipe roadmap, a generated roadmap file is submitted to a Blob server that we maintain. Upon reaching the first Zess T section in the Paper Mario: TTYD 100% TAS, the search will be over and the TAS will implement the currently known fastest recipe roadmap. Below are a set of rules and assumptions regarding a glitch we will be making use of called Inventory Overload. Given these rules and assumptions, we have constructed an algorithm which mimics how the game handles the inventory while under the effect of this glitch, as well as determine whether or not certain moves can be performed, which may include: cooking a particular recipe, using a particular item combination to cook a particular recipe, and tossing a particular item in the inventory to make room for the output.

## Rules and Assumptions
- A glitch is being exploited that allows one to set the number of available inventory slots in the inventory to ten after it was already expanded to twenty and had items in those slots.
- Using an item in the first ten slots removes the item, and the slot becomes unfilled, as happens normally.
- However, using an item in the last ten slots does not remove it, effectively allowing for duplication.
- Sorting of the inventory is still fully functional, so any item can be duplicated this way by sorting it to a slot between 11-20.
- While the glitch is active, unfilled slots in the first ten are moved to what the game thinks is the end of the inventory. So a newly vacated slot is moved to slot 10, with everything between the previous position of the slot and slot 10 being moved one slot towards the beginning of the inventory. In the pause menu, these slots are displayed as NULL  items, but in the menus for giving or replacing an item, they do not show up.
- When the game displays the inventory, it counts how many non-NULL items are in the inventory. It then displays the items in that many slots starting from the beginning. This is why NULL items are normally not displayed, since they are in the slots that are not displayed. However, due to the glitch placing the NULLs in the middle of the inventory, the slots that are not shown actually have items in them. So effectively, there are as many hidden items at the end of the inventory as there are NULLs. In order to use these items, the NULL slots must be either filled or moved to the actual end of the inventory with a sort.
- When there is at least one NULL slot, receiving an item causes the inventory items preceding the first NULL slot to be shifted one position towards the back of the inventory. The received item is then placed at the start. (This is what happens regardless of the inventory glitch.)
- When there are no NULL slots, receiving an item gives the player the option of tossing either the item produced or an item in the inventory. Due to the glitch, attempting to toss an item in the second half of the inventory will not replace the item, and the item received will be lost. Therefore, the second half of the inventory is never considered in this decision.
- When the inventory is sorted, the items are placed in the specified order starting from the beginning of the inventory. All NULL slots are therefore no longer available, since they are now all at the end of the inventory. The total number of slots available to work with can therefore decrease over time, and once that has occurred, it is permanent. The unavailable slots at the end of the inventory are referred to as BLOCKED.
- Due to the progression of Zess T.'s dialogue, one recipe is cooked before giving her the cookbook. This cookbook is what allows her to cook recipes with two ingredients, so the first recipe that is fulfilled can only use a single ingredient.
- The glitch greatly reduces the need for obtaining items to use to cook recipes. In fact, by placing most item uses before starting to cook recipes, it is almost possible to complete them all in one trip to Zess T. Unfortunately, the Dried Bouquet is a required item for one recipe, and in order to get it, the Mousse Cake, which is cooked by Zess T., must already be possessed. The Dried Bouquet is unique, so the inevitable conclusion is that there must be two trips to Zess T., with an intermission between them.
- In the intermission, two Hot Dogs and a Mousse Cake are traded for a Dried Bouquet. Then, a Coconut and a Keel Mango are collected, and the Coconut is traded to Flavio for the Chuckola Cola. The Courage Shell is collected, and a Thunder Rage is used to fight Smorg. All of the collected items (except for the Chuckola Cola, which is an Important Item) are required for making recipes, which means that a sort is required between obtaining the Coconut and giving it to Flavio in order to keep it using the glitch. Other sorts are possible during the intermission as well, but the only one that will be considered is the one that is required.

## Algorithm Description
- Start main loop.
- Iterate through all currently possible ways of making the recipes that have not been fulfilled.
- Iterate through all placement options of the output of the recipe.
- Calculate the frames to perform the action and add it to a list of "legal moves" for the current state, sorted ascending by frames.
- If the Chapter 5 intermission can be performed, add every way that it can be completed with one sort to the legal move list.
- If the last move was not a sort, add the four ways of sorting to the end of the legal move list, regardless of frame count.
- Use RNG-based methodologies to select a legal move from the list, set this legal move as the current state, and begin the main loop a level deeper.
- If there are no legal moves remaining, then go back up a level and pick the next legal move to explore.
- Periodically reset the search space to get a new random branch. This prevents the user from extensively searching a dead-end branch.

## Building
### Linux
To build on Linux, you will need to run the following commands to run this program:
1. `sudo apt-get install make libcurl4-openssl-dev libomp-dev libconfig-dev`
1. `make`
1. `./recipesAtHome`

Should there be any problems in this building process, please let us know by posting an issue on Github.

### Windows
To build on Windows, use the Visual Studio CipesAtHome.sln solution file. You will need to install libcurl and libconfig. This can be done by making use of vcpkg.

### macOS
To build on macOS (w/ Homebrew installed), you will need to run the following commands from the project root, in order to run this program:
1. `brew install llvm libomp libconfig`
1. `make`
1. `./recipesAtHome`

To run the pre-built unix executable on mac:
1. download and unzip the file
1. open a new terminal window
1. `cd ` followed by the path to the extracted folder
1. `./recipesAtHome`

Should there be any problems in the building process, please let us know by posting an issue on Github.

## Config Settings
Below are a set of config parameters which can be changed by the user. These will affect how the algorithm handles legal moves, as well as check for the current program version on Github.

- **select**: This is a methodology that chooses the <em>i</em>th legal move in the array with arbitrary probability (0.5)<sup><em>i</em></sup>. This is used to effectively generate a different roadmap final roadmap on every descent.
- **randomise**: This methodology randomises the entire list of legal moves. This will not generate fast roadmaps as efficiently as select will be. In the event that select and randomise are both chosen, the algorithm will prioritize the select methodology.
- **logLevel**: This parameter specifies the degree of detail to which the program will output log information both to stdout and to the log file. The higher the number, the more detail will be output. If logLevel is 0, no data will be output. Specific thresholds are listed in config.txt.
- **workerCount**: The number of threads to run simultaneously for the program. Generally, this can be as high as (# CPU Cores) - 2, though increasing the workerCount means that less CPU time can be dedicated to other programs running on your computer. If you are running any intensive program besides this program, then you should close the program, change the workerCount, and restart the program while using the other intensive program.
- **Username**: This name will be submitted to the server to specify who found the roadmap. If you would like to be known for finding the fastest roadmap, change this name to a username of your choice. This is limited by 19 characters. A Discord bot in the TTYD speedrunning server will alert us with this Username when a new fastest roadmap is found.
