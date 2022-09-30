#include <stdio.h>
#include <stdlib.h>
#include "node_print.h"
#include "base.h"
#include "calculator.h"
#include "logger.h"
#include "recipes.h"

extern Recipe *recipeList;

/*-------------------------------------------------------------------
 * Function 	: printCh5Data
 * Inputs	: BranchPath		*curNode
 *		  MoveDescription 	desc
 *		  FILE				*fp
 *
 * Print to a txt file the data which pertains to Chapter 5 evaluation
 * (where to place Dried Bouquet, Coconut, etc.)
 -------------------------------------------------------------------*/
void printCh5Data(const BranchPath *curNode, const MoveDescription desc, FILE *fp) {
	const CH5 *ch5Data = desc.data;

	// Determine how many nulls there are when allocations start
	int nulls = curNode->prev->inventory.nulls;
	if (indexOfItemInInventory(curNode->prev->inventory, Mousse_Cake) < 10) {
		++nulls;
	}

	fprintf(fp, "Ch.5 Break: ");
	if (nulls) {
		fprintf(fp, "DB filling null, ");
		--nulls;
	}
	else {
		fprintf(fp, "DB replacing #%d, ", ch5Data->indexDriedBouquet + 1);
	}

	if (nulls) {
		fprintf(fp, "CO filling null, ");
		--nulls;
	}
	else {
		fprintf(fp, "CO replacing #%d, ", ch5Data->indexCoconut + 1);
	}
	if (ch5Data->lateSort) {
		if (nulls) {
			fprintf(fp, "KM filling null, ");
		}
		else {
			fprintf(fp, "KM replacing #%d, ", ch5Data->indexKeelMango + 1);
		}
		printCh5Sort(ch5Data, fp);
	}
	else {
		printCh5Sort(ch5Data, fp);
		fprintf(fp, "KM replacing #%d, ", ch5Data->indexKeelMango + 1);
	}
	fprintf(fp, "CS replacing #%d, use TR in #%d",
		ch5Data->indexCourageShell + 1, ch5Data->indexThunderRage + 1);
}

/*-------------------------------------------------------------------
 * Function 	: printCh5Sort
 * Inputs	: CH5	*ch5Data
 *		  FILE		*fp
 *
 * Print to a txt file the data which pertains to Chapter 5 sorting
 -------------------------------------------------------------------*/
void printCh5Sort(const CH5 *ch5Data, FILE *fp) {
	fprintf(fp, "sort ");
	switch (ch5Data->ch5Sort) {
		case ESort_Alpha_Asc:
			fprintf(fp, "(Alpha), ");
			break;
		case ESort_Alpha_Des:
			fprintf(fp, "(Reverse-Alpha), ");
			break;
		case ESort_Type_Asc:
			fprintf(fp, "(Type), ");
			break;
		case ESort_Type_Des:
			fprintf(fp, "(Reverse-Type), ");
			break;
		default:
			fprintf(fp, "ERROR IN CH5SORT SWITCH CASE");
	};
}

/*-------------------------------------------------------------------
 * Function 	: printCookData
 * Inputs	: BranchPath 		*curNode
 *		  MoveDescription 	desc
 *		  FILE				*fp
 *
 * Print to a txt file the data which pertains to cooking a recipe,
 * which includes what items were used and what happens to the output.
 -------------------------------------------------------------------*/
void printCookData(const BranchPath *curNode, const MoveDescription desc, FILE *fp) {
	Cook *cookData = desc.data;
	int nulls = curNode->prev->inventory.nulls;
	fprintf(fp, "Use [%s] in slot %d ", getItemName(cookData->item1),
		cookData->itemIndex1 - (cookData->itemIndex1 < 10 ? nulls : 0) + 1);

	if (cookData->numItems == 2) {
		fprintf(fp, "and [%s] in slot %d ", getItemName(cookData->item2),
			cookData->itemIndex2 - (cookData->itemIndex2 < 10 ? nulls : 0) + 1);
	}

	fputs("to make ", fp);

	if (cookData->handleOutput == Toss) {
		fputs("(and toss) ", fp);
	}
	else if (cookData->handleOutput == Autoplace) {
		fputs("(and auto-place) ", fp);
	}

	fprintf(fp, "<%s>", getItemName(cookData->output));

	if (cookData->handleOutput == TossOther) {
		fprintf(fp, ", toss [%s] in slot %d", getItemName(cookData->toss), cookData->indexToss + 1);
	}

	if (curNode->numOutputsCreated == NUM_RECIPES) {
		if (((Cook *) curNode->description.data)->handleOutput == Autoplace) {
			fputs(" (No-Toss 5 Frame Penalty for Jump Storage)", fp);
		}
		else {
			fputs(" (Jump Storage on Tossed Item)", fp);
		}
	}
}

/*-------------------------------------------------------------------
 * Function 	: printFileHeader
 * Inputs	: FILE				*fp
 *
 * Print to a txt file the header information for the file.
 -------------------------------------------------------------------*/
void printFileHeader(FILE *fp) {
	fputs("Description\tFrames Taken\tTotal Frames", fp);
	for (int i = 0; i < 20; i++) {
		fprintf(fp, "\tSlot #%d", i+1);
	}
	for (int i = 0; i < NUM_RECIPES; i++) {
		fprintf(fp, "\t%s", getItemName(recipeList[i].output));
	}
	fprintf(fp, "\n");
	recipeLog(5, "Calculator", "File", "Write", "Header for new output written");
}

/*-------------------------------------------------------------------
 * Function 	: printInventoryData
 * Inputs	: BranchPath	*curNode
 *		  FILE			*fp
 *
 * Print to a txt file the header information for the file.
 -------------------------------------------------------------------*/
void printInventoryData(const BranchPath *curNode, FILE *fp) {
	int nulls = curNode->inventory.nulls;
	int i;
	for (i = nulls; i < 10; ++i) {
		fprintf(fp, "\t%s", getItemName(curNode->inventory.inventory[i]));
	}
	for (i = 0; i < nulls; ++i) {
		fprintf(fp, "\tNULL");
	}
	for (i = 10; i < curNode->inventory.length - nulls; ++i) {
		fprintf(fp, "\t%s", getItemName(curNode->inventory.inventory[i]));
	}
	for (; i < curNode->inventory.length; ++i) {
		fprintf(fp, "\t(%s)", getItemName(curNode->inventory.inventory[i]));
	}
	for (; i < 20; ++i) {
		fprintf(fp, "\tBLOCKED");
	}
}

/*-------------------------------------------------------------------
 * Function 	: printOutputsCreated
 * Inputs	: BranchPath	*curNode
 *		  FILE			*fp
 *
 * Print to a txt file data pertaining to which recipes
 * have been cooked thus far.
 -------------------------------------------------------------------*/
void printOutputsCreated(const BranchPath *curNode, FILE *fp) {
	for (int i = 0; i < NUM_RECIPES; i++) {
		if (curNode->outputCreated[i]) {
			fprintf(fp, "\tTrue");
		}
		else {
			fprintf(fp, "\tFalse");
		}
	}
}

void printNodeDescription(const BranchPath * curNode, FILE * fp)
{
	MoveDescription desc = curNode->description;
	enum Action curNodeAction = desc.action;
	switch (curNodeAction) {
	case ECook:
		printCookData(curNode, desc, fp);
		break;
	case ECh5:
		printCh5Data(curNode, desc, fp);
		break;
	case EBegin:
		fputs("Begin", fp);
		break;
	default:
		// Some type of sorting
		printSortData(fp, curNodeAction);
	}
}

/*-------------------------------------------------------------------
 * Function 	: printResults
 * Inputs	: char			*filename
 *		  BranchPath	*path
 *
 * Parent function for children print functions. This parent function
 * is called when a roadmap has been found which beats the current
 * local record.
 -------------------------------------------------------------------*/
void printResults(const char *filename, const BranchPath *path) {
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		printf("Could not locate %s... This is a bug.\n", filename);
		printf("Press ENTER to exit.\n");
		awaitKeyFromUser();
		exit(1);
	}
	// Write header information
	printFileHeader(fp);

	// Print data information
	const BranchPath *curNode = path;
	do {
		printNodeDescription(curNode, fp);

		// Print out frames taken
		fprintf(fp, "\t%d", curNode->description.framesTaken);
		// Print out total frames taken
		fprintf(fp, "\t%d", curNode->description.totalFramesTaken);

		// Print out inventory
		printInventoryData(curNode, fp);

		// Print out whether or not all 58 items were created
		printOutputsCreated(curNode, fp);

		// Add newline character to put next node on new line
		fprintf(fp, "\n");
	} while ((curNode = curNode->next) != NULL);

	fclose(fp);

	recipeLog(5, "Calculator", "File", "Write", "Data for roadmap written.");
}

/*-------------------------------------------------------------------
 * Function 	: printSortData
 * Inputs	: FILE 	*fp
 *		  enum Action 	curNodeAction
 *
 * Print to a file data which pertains to sorting the inventory.
 -------------------------------------------------------------------*/
void printSortData(FILE *fp, enum Action curNodeAction) {
	fprintf(fp, "Sort - ");
	switch (curNodeAction) {
		case ESort_Alpha_Asc:
			fputs("Alphabetical", fp);
			break;
		case ESort_Alpha_Des:
			fputs("Reverse Alphabetical", fp);
			break;
		case ESort_Type_Asc:
			fputs("Type", fp);
			break;
		case ESort_Type_Des:
			fputs("Reverse Type", fp);
			break;
		default:
			fputs("ERROR IN HANDLING OF SORT", fp);
	};
}