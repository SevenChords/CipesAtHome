#include "serialization.h"

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <types.h>

#include "base.h"
#include "logger.h"
#include "recipes.h"

#ifdef _IS_WINDOWS
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

Serial** visitedBranches = NULL;
uint32_t* numVisitedBranches = NULL;
extern Recipe* recipeList;

// Used to uniquely identify a particular item combination for serialization purposes
static int recipeOffsetLookup[57] = {
	0, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15, 16, 17, 18, 23, 24, 25,
	26, 28, 31, 35, 37, 39, 40, 41, 44, 45, 46, 48, 50, 52,
	59, 60, 61, 62, 63, 64, 66, 68, 71, 77, 82, 83, 85, 86,
	87, 88, 89, 90, 92, 147, 148, 149, 152, 153, 154, 155 };

/*-------------------------------------------------------------------
 * Function : serialcmp
 *
 * Perform a memcmp between two serials, with care taken to observe
 * difference in serial length.
 -------------------------------------------------------------------*/
ABSL_ATTRIBUTE_ALWAYS_INLINE
inline int serialcmp(Serial s1, Serial s2)
{
	int diff = memcmp(s1.data, s2.data, min(s1.length, s2.length));

	// if the prefix is identical, then we are dealing with a parent-child
	// in this case, we want to keep moving left if s2 is larger
	if (diff != 0)
		return diff;

	if (s1.length != s2.length)
		return (s1.length > s2.length) ? 1 : -1;

	return 0;
}

/*-------------------------------------------------------------------
 * Function : consolidatedFileExists
 *
 * This helper function just checks to see if the consolidated cache file exists.
 -------------------------------------------------------------------*/
ABSL_ATTRIBUTE_ALWAYS_INLINE
static inline bool consolidatedFileExists()
{
	return (access("results/visitedNodes.dat", F_OK) == 0);
}

/*-------------------------------------------------------------------
 * Function : isChild
 *
 * Helper function that determines if a Serial represents a node further
 * down in the tree from the parent, in which case we won't want to
 * insert the child in the global array of visited nodes.
 -------------------------------------------------------------------*/
static inline bool isChild(Serial parent, Serial child)
{
	if (child.length < parent.length)
		return false;

	return (memcmp(parent.data, child.data, parent.length) == 0);
}

/*-------------------------------------------------------------------
 * Function : initializeVisitedNodes
 *
 * This is the top-level function that is called during start-up.
 * Allocates arrays for each thread, and attempts to read cache file
 * for each thread. Afterwards, merges all serials from disk into one
 * in-memory array, which is then deep-copied so that all threads
 * start with the conjoined list of visited nodes.
 -------------------------------------------------------------------*/
void initializeVisitedNodes(int workerCount)
{
	recipeLog(2, "Startup", "Cache", "Visited Nodes", "Reading visited nodes from disk... This may take a few seconds.");

	// Serials from all thread files will be combined into one array
	Serial* combined = NULL;
	uint32_t combinedLen = 0;

	// Check to see if the consolidated file is present for patient users who let shutdown finish properly
	if (consolidatedFileExists())
		combinedLen = ReadConsolidatedFile(&combined);
	else
	{
		// Obtain file pointers to the file for each of the workerCount threads
		FILE** fp = getCacheFilePtrs(workerCount);

		// Read in data from file for each thread and merge to a combined array
		combinedLen = mergeThreadSerials(&combined, fp, workerCount);

		closeCacheFilePtrs(fp, workerCount);
	}

	// Initialize global array of arrays, one for each thread
	visitedBranches = malloc(workerCount * sizeof(Serial*));
	checkMallocFailed(visitedBranches);
	numVisitedBranches = calloc(workerCount, sizeof(uint32_t));
	checkMallocFailed(numVisitedBranches);

	// Perform deep copies so each thread can manage visited nodes independently
	// First thread can just use the array
	visitedBranches[0] = combined;
	numVisitedBranches[0] = combinedLen;

	for (int i = 1; i < workerCount; i++)
	{
		visitedBranches[i] = deepCopy(combined, combinedLen);
		numVisitedBranches[i] = combinedLen;
	}

	char result[100];
	sprintf(result, "Found %d serials from disk", combinedLen);
	recipeLog(2, "Startup", "Cache", "Visited Nodes", result);
}

/*-------------------------------------------------------------------
 * Function : ReadConsolidatedFile
 *
 * If the user was patient and waited for complete shutdown,
 * then they're rewarded with less disk space consumption and faster
 * bootup times on subsequent launches.
 -------------------------------------------------------------------*/
uint32_t ReadConsolidatedFile(Serial** combined)
{
	uint32_t readSerials = 0;

	FILE* fp = NULL;
	fp = fopen("results/visitedNodes.dat", "rb");
	if (fp == NULL)
		return 0;

	uint32_t expectedSerialsToRead;
	size_t ret = fread(&expectedSerialsToRead, sizeof(uint32_t), 1, fp);
	if (ret == 0)
	{
		fclose(fp);
		return 0;
	}

	*combined = malloc(expectedSerialsToRead * sizeof(Serial));

	Serial nextSerial = (Serial){ 0, NULL };
	while ((nextSerial = readNextSerial(fp)).length > 0)
		(*combined)[readSerials++] = nextSerial;

	fclose(fp);

	return readSerials;
}

/*-------------------------------------------------------------------
 * Function : readNextSerial
 *
 * Given a file ptr, read in the next serial in the file.
 -------------------------------------------------------------------*/
Serial readNextSerial(FILE* fp)
{
	Serial nextSerial = (Serial){ 0, NULL };
	size_t ret = fread(&nextSerial.length, sizeof(uint8_t), 1, fp);
	if (ret == 0)
	{
		nextSerial.length = 0;
		return nextSerial;
	}

	nextSerial.data = malloc(nextSerial.length * sizeof(char));
	checkMallocFailed(nextSerial.data);

	ret = fread(nextSerial.data, sizeof(char), nextSerial.length, fp);
	if (ret == 0)
	{
		free(nextSerial.data);
		nextSerial = (Serial){ 0, NULL };
	}

	return nextSerial;
}

/*-------------------------------------------------------------------
 * Function : getCacheFilePtrs
 *
 * Get file pointers to each thread-specific cache file on disk.
 -------------------------------------------------------------------*/
FILE** getCacheFilePtrs(int workerCount)
{
	FILE** fp = malloc(workerCount * sizeof(FILE*));
	checkMallocFailed(fp);

	for (int i = 0; i < workerCount; i++)
	{
		char filename[50] = { 0 };
		sprintf(filename, "results/visitedNodes_%d.dat", i);

		fp[i] = fopen(filename, "rb");
	}

	return fp;
}

/*-------------------------------------------------------------------
 * Function : mergeThreadSerials
 *
 * Referencing file pointers, merge all serials into one combined arr.
 -------------------------------------------------------------------*/
uint32_t mergeThreadSerials(Serial** combined, FILE** fp, int workerCount)
{
	// For each thread, store the number of expected serials
	uint32_t* numThreadVisited = calloc(workerCount, sizeof(uint32_t));
	checkMallocFailed(numThreadVisited);

	// Keep track of the most recently read serial from each fp so we can determine which one to insert next
	Serial* curSerials = malloc(workerCount * sizeof(Serial));
	checkMallocFailed(curSerials);

	// For each file, read in number of nodes visited AND the first serial in the file
	for (int i = 0; i < workerCount; i++)
	{
		// Set to empty Serial in case something goes wrong in this loop
		curSerials[i] = (Serial){ 0, NULL };

		if (fp[i] == NULL)
			continue;

		size_t ret = fread(numThreadVisited + i, sizeof(uint32_t), 1, fp[i]);
		if (ret == 0)
			continue;

		Serial serial = (Serial){ 0, NULL };
		ret = fread(&serial.length, sizeof(uint8_t), 1, fp[i]);
		if (ret == 0)
		{
			numThreadVisited[i] = 0;
			continue;
		}

		serial.data = malloc(serial.length * sizeof(char));
		checkMallocFailed(serial.data);

		ret = fread(serial.data, sizeof(char), serial.length, fp[i]);
		if (ret != serial.length) {
			free(serial.data);
			numThreadVisited[i] = 0;
			continue;
		}

		curSerials[i] = serial;
	}

	// Sum the number of visited nodes so we can malloc the combined array
	// We will certainly never use this full length as there will be duplicate nodes,
	// but this will prevent constant reallocs
	uint32_t totalNodesExpected = sumVisited(numThreadVisited, workerCount);
	*combined = malloc(totalNodesExpected * sizeof(Serial));
	checkMallocFailed(*combined);

	// Used to track if we are about to insert a child node based off what we have previously inserted
	Serial prevSerial = (Serial){ 0, NULL };
	uint32_t totalNodesCombined = 0;

	// Keep reading serials in so long as there are serials present in curSerials
	Serial nextSerial = (Serial){ 0, NULL };
	while ((nextSerial = nextInsert(curSerials, fp, workerCount)).length > 0)
	{
		// We only want to add the serial if we are inserting a new parent node
		// Detect based off of the prevSerial to see if this new node is a child
		if (prevSerial.length > 0 && isChild(prevSerial, nextSerial))
		{
			free(nextSerial.data);
			continue;
		}

		prevSerial = nextSerial;
		(*combined)[totalNodesCombined++] = nextSerial;
	}

	// realloc if we read in fewer nodes than expected
	if (totalNodesCombined < totalNodesExpected)
		*combined = realloc(*combined, totalNodesCombined * sizeof(Serial));

	free(numThreadVisited);
	free(curSerials);

	return totalNodesCombined;
}

/*-------------------------------------------------------------------
 * Function : nextInsert
 *
 * Given workerCount number of serial arrays pointed to by threadVisitedArr,
 * determine which Serial should be inserted into the combined array next.
 * To keep track of our walk along each thread-specific array, we keep
 * track of an array of indexes, incrementing a thread's index when we've
 * determined the next Serial to be added will be from that thread.
 * We return the threadID but also set the value pointed to by nextIndex
 * so that the caller can retrieve the value at the index from the given threadID.
 -------------------------------------------------------------------*/
Serial nextInsert(Serial* curSerials, FILE** fp, int workerCount)
{
	int nextWorker = -1;
	Serial lowestSerial = (Serial){ 0, NULL };

	for (int i = 0; i < workerCount; i++)
	{
		Serial curSerial = curSerials[i];

		if (curSerial.length == 0)
			continue;

		// If this is our first serial in loop, then just set this as lowest
		if (lowestSerial.length == 0)
		{
			nextWorker = i;
			lowestSerial = curSerial;
			continue;
		}

		// Otherwise, perform a serialcmp to see if it's lower than the others so far
		int cmp = serialcmp(curSerial, lowestSerial);

		if (cmp < 0)
		{
			// New lowest
			nextWorker = i;
			lowestSerial = curSerial;
		}
		else if (cmp == 0)
		{
			// This is a duplicate node, free the duplicate data
			free(curSerial.data);

			// Read in the next Serial for next time this function is called
			curSerials[i] = readNextSerial(fp[i]);
		}
	}

	if (nextWorker >= 0)
		curSerials[nextWorker] = readNextSerial(fp[nextWorker]);

	return lowestSerial;
}

/*-------------------------------------------------------------------
 * Function : closeCacheFilePtrs
 *
 * Close all thread-specific cache file pointers.
 -------------------------------------------------------------------*/
void closeCacheFilePtrs(FILE** fp, int workerCount)
{
	for (int i = 0; i < workerCount; i++)
	{
		if (fp[i] != NULL)
			fclose(fp[i]);
	}
}

/*-------------------------------------------------------------------
 * Function : deepCopy
 *
 * Copy src to dest, using a new heap ptr for the serial data.
 -------------------------------------------------------------------*/
Serial* deepCopy(Serial* src, uint32_t len)
{
	Serial* dest = malloc(len * sizeof(Serial));
	for (uint32_t i = 0; i < len; i++)
	{
		Serial serial = src[i];
		void* dataCpy = malloc(serial.length * sizeof(char));
		checkMallocFailed(dataCpy);
		memcpy(dataCpy, serial.data, serial.length * sizeof(char));
		serial.data = dataCpy;
		dest[i] = serial;
	}

	return dest;
}

/*-------------------------------------------------------------------
 * Function : legalMoveHasBeenTraversed
 *
 * Perform a binary search on the global visitedBranches array,
 * and observe if the provided node's serial is present.
 -------------------------------------------------------------------*/
bool legalMoveHasBeenTraversed(BranchPath* newLegalMove)
{
	// Handle the Mistake case
	if (newLegalMove->serial.data == NULL)
		return false;

	int threadID = omp_get_thread_num();
	int index = searchVisitedNodes(newLegalMove->serial, 0, numVisitedBranches[threadID] - 1, threadID);
	return (index >= 0);
}

/*-------------------------------------------------------------------
 * Function : searchVisitedNodes
 *
 * Perform a binary search to determine
 * if the provided serial exists in the global array.
 * If it exists, this means we have already visited the node
 * that owns the provided serial.
 -------------------------------------------------------------------*/
int searchVisitedNodes(Serial serial, int low, int high, int threadID)
{
	while (high > low)
	{
		int mid = (low + high) / 2;

		Serial arrSerial = visitedBranches[threadID][mid];

		bool bLengthEqual = (arrSerial.length == serial.length);
		int cmpRet = serialcmp(arrSerial, serial);
		if (bLengthEqual && cmpRet == 0)
			return mid;

		if (cmpRet > 0)
			high = mid - 1;
		else
			low = mid + 1;
	}

	return -1;
}

/*-------------------------------------------------------------------
 * Function : getRecipeIndex
 *
 * Finds a unique identifier for a particular recipe combination.
 * For a particular recipe combination, sum all of the combinations
 * of cooking lower-index outputs, then add all earlier combinations
 * of cooking the output in question. This utilizes recipeOffSetLookup
 * to prevent complex lookup times.
 -------------------------------------------------------------------*/
uint8_t getRecipeIndex(Cook* pCook)
{
	int i = getOutputIndex(pCook->output);
	int offset = recipeOffsetLookup[i];

	for (int j = 0; j < recipeList[i].countCombos; j++)
	{
		ItemCombination listCombo = recipeList[i].combos[j];

		if (listCombo.numItems != pCook->numItems)
			continue;

		bool bFirstItemMatch = listCombo.item1 == pCook->item1 || (pCook->numItems == 2 && listCombo.item1 == pCook->item2);
		bool bSecondItemMatch = listCombo.item2 == pCook->item2 || (pCook->numItems == 2 && listCombo.item2 == pCook->item1);

		if (bFirstItemMatch && (pCook->numItems == 1 || bSecondItemMatch))
			return offset + j;
	}

	return UINT8_MAX;
}

/*-------------------------------------------------------------------
 * Function : serializeCookNode
 *
 * Generates a unique set of bytes to represent the cook action performed.
 * Returns the length of bytes required to represent the action.
 * Note that we skip representing the Mistake, as it's always the last recipe,
 * and the Dried Bouquet, as the that is considered part of the CH5 sequence.
 -------------------------------------------------------------------*/
uint8_t serializeCookNode(BranchPath* node, void** data)
{
	uint8_t dataLen = 0;
	Cook* pCook = (Cook*)node->description.data;
	Serial parentSerial = node->prev->serial;

	// Don't really need to hash Mistake, and removing all recipe combos for mistakes saves space for our serialization
	if (pCook->output == Mistake)
	{
		*data = NULL;
		return 0;
	}
	else if (pCook->output == Dried_Bouquet)
	{
		// This shouldn't happen
		exit(1);
	}

	uint8_t recipeIdx = getRecipeIndex(pCook);
	assert(recipeIdx != UINT8_MAX);

	dataLen = 1;

	int8_t outputPlace = pCook->indexToss;
	bool bAutoplace = (outputPlace == -1);

	if (!bAutoplace)
		dataLen = 2;

	if (parentSerial.length == 0)
	{
		// We have nothing to copy. Just malloc 3 bytes
		*data = malloc(dataLen);
		checkMallocFailed(*data);
	}
	else
	{
		*data = malloc(parentSerial.length + dataLen);
		checkMallocFailed(*data);

		memcpy(*data, parentSerial.data, parentSerial.length);
	}

	memset((char*)*data + parentSerial.length, recipeIdx, 1);

	if (!bAutoplace)
		memset((char*)*data + parentSerial.length + 1, (uint8_t)outputPlace, 1);

	return dataLen;
}

/*-------------------------------------------------------------------
 * Function : serializeSortNode
 *
 * Generates a unique set of bytes to represent the sort performed.
 * Returns the length of bytes required to represent the action.
 -------------------------------------------------------------------*/
uint8_t serializeSortNode(BranchPath* node, void** data)
{
	uint8_t dataLen = 1;
	Serial parentSerial = node->prev->serial;

	if (parentSerial.length == 0)
	{
		*data = malloc(dataLen);
		checkMallocFailed(*data);
	}
	else
	{
		*data = malloc(parentSerial.length + dataLen);
		checkMallocFailed(*data);
		memcpy(*data, parentSerial.data, parentSerial.length);
	}

	uint8_t actionValue = (node->description.action - 2) + recipeOffsetLookup[56];
	memset((char*)*data + parentSerial.length, actionValue, 1);

	return dataLen;
}

/*-------------------------------------------------------------------
 * Function : serializeCH5Node
 *
 * Generates a unique set of bytes to represent the CH5 sequence.
 * Returns the length of bytes required to represent the action.
 -------------------------------------------------------------------*/
uint8_t serializeCH5Node(BranchPath* node, void** data)
{
	uint8_t actionValue = recipeOffsetLookup[56] + 4;
	Serial parentSerial = node->prev->serial;

	CH5* pCH5 = (CH5*)node->description.data;
	uint8_t lateSort = (uint8_t)pCH5->lateSort;
	uint8_t sort = (uint8_t)pCH5->ch5Sort - 2;
	uint8_t uCS = (uint8_t)pCH5->indexCourageShell;
	int iDB = pCH5->indexDriedBouquet;
	int iCO = pCH5->indexCoconut;
	int iKM = pCH5->indexKeelMango;

	if (lateSort == 1)
		actionValue += 40;

	actionValue += (sort * 10);
	actionValue += uCS;

	uint8_t dataLen = 1;

	// Tack on additional bits for each item that is not autoplaced
	uint8_t optionalByte1 = 0;
	uint8_t optionalByte2 = 0;

	if (iDB > -1)
	{
		dataLen = 2;
		optionalByte1 = ((uint8_t)(iDB)) << 4;
	}
	if (iCO > -1)
	{
		dataLen = 2;
		if (iDB > -1)
			optionalByte1 |= (uint8_t)(iCO);
		else
			optionalByte1 = ((uint8_t)(iCO)) << 4;
	}
	if (iKM > -1)
	{
		dataLen = 2;
		if (iDB > -1 && iCO > -1)
		{
			dataLen = 3;
			optionalByte2 = ((uint8_t)(iKM)) << 4;
		}
		else if (iDB > -1 || iCO > -1)
			optionalByte1 |= (uint8_t)(iKM);
		else
			optionalByte1 = ((uint8_t)(iKM)) << 4;
	}

	*data = malloc(parentSerial.length + dataLen);
	checkMallocFailed(*data);
	if (parentSerial.length > 0)
		memcpy(*data, parentSerial.data, parentSerial.length);

	memset((char*)*data + parentSerial.length, actionValue, 1);

	if (dataLen >= 2)
		memset((char*)*data + parentSerial.length + 1, optionalByte1, 1);
	if (dataLen == 3)
		memset((char*)*data + parentSerial.length + 2, optionalByte2, 1);

	return dataLen;
}

/*-------------------------------------------------------------------
 * Function : serializeNode
 *
 * Observes the action taken in this node, generates unique bytes to
 * represent it, and saves it within the node struct to be referenced later.
 -------------------------------------------------------------------*/
void serializeNode(BranchPath* node)
{
	if (node->moves == 0)
	{
		node->serial = (Serial){ 0, NULL };
		return;
	}

	void* data = NULL;
	uint8_t dataLen = 0; // in bytes
	Serial parentSerial = node->prev->serial;
	Action nodeAction = node->description.action;

	switch (nodeAction)
	{
	case EBegin:

	case ECook:
		dataLen = serializeCookNode(node, &data);
		break;
	case ESort_Alpha_Asc:
	case ESort_Alpha_Des:
	case ESort_Type_Asc:
	case ESort_Type_Des:
		dataLen = serializeSortNode(node, &data);
		break;
	case ECh5:
		dataLen = serializeCH5Node(node, &data);
		break;
	default:
		exit(1);
	}

	if (dataLen == 0)
		node->serial = (Serial){ 0, NULL };
	else
		node->serial = (Serial){ parentSerial.length + dataLen, data };
}

/*-------------------------------------------------------------------
 * Function : cacheSerial
 *
 * Insert the given node's serial into the global sorted array.
 -------------------------------------------------------------------*/
void cacheSerial(BranchPath* node)
{
	// ignore root node and Mistake
	if (node->serial.length == 0 || node->serial.data == NULL)
		return;

	if (node->serial.data == NULL)
		exit(2);

	void* cachedData = malloc(node->serial.length);
	memcpy(cachedData, node->serial.data, node->serial.length);

	Serial cachedSerial = (Serial){ node->serial.length, cachedData };

	uint32_t index = 0;

	int threadID = omp_get_thread_num();
	if (numVisitedBranches[threadID] > 0)
		index = indexToInsert(cachedSerial, 0, numVisitedBranches[threadID] - 1, threadID);

	// Free and note how many children we are freeing
	int childrenFreed = deleteAndFreeChildSerials(cachedSerial, index, threadID);
	insertIntoCache(cachedSerial, index, childrenFreed, threadID);
}

/*-------------------------------------------------------------------
 * Function : indexToInsert
 *
 * Perform a binary search to determine
 * where we should insert the given serial.
 * Used an iterative approach because of concerns over stack overflow
 * with a sufficiently large enough array.
 -------------------------------------------------------------------*/
uint32_t indexToInsert(Serial serial, int low, int high, int threadID)
{
	while (high > low)
	{
		int mid = (low + high) / 2;

		int cmpMid = serialcmp(serial, visitedBranches[threadID][mid]);
		if (cmpMid == 0)
			return mid + 1;

		if (cmpMid > 0)
			low = mid + 1;
		else
			high = mid - 1;
	}

	return (serialcmp(serial, visitedBranches[threadID][low]) > 0) ? low + 1 : low;
}

/*-------------------------------------------------------------------
 * Function : deleteAndFreeChildSerials
 *
 * If we are caching a serial, then this means we have visited all child nodes.
 * Observe if there are any cached child node serials and free them.
 * Returns the number of free'd child serials. Global array is shifted
 * on insert to avoid an additional realloc call.
 -------------------------------------------------------------------*/
uint32_t deleteAndFreeChildSerials(Serial serial, uint32_t index, int threadID)
{
	uint32_t deletedChildren = 0;

	for (uint32_t i = index; i < numVisitedBranches[threadID]; i++)
	{
		Serial arrSerial = visitedBranches[threadID][i];
		// If the next serial is shorter than the current serial, then we know that
		// the next serial is higher up in the tree. Thus, we can terminate the loop
		if (arrSerial.length < serial.length)
			break;

		int diff = memcmp(arrSerial.data, serial.data, serial.length);

		// Does this serial correspond with some other parent actions than the current serial?
		if (diff != 0)
			break;

		// if diff == 0, then the ith serial is a child of the current serial, so delete ith serial
		if (diff == 0)
		{
			++deletedChildren;

			// Free the child!
			free(arrSerial.data);
			arrSerial.length = 0;
			arrSerial.data = NULL;
		}
	}

	return deletedChildren;
}

/*-------------------------------------------------------------------
 * Function : insertIntoCache
 *
 * Insert the given serial into the global array at index provided,
 * while factoring in how many children we have free'd via deleteAndFreeChildSerials
 -------------------------------------------------------------------*/
void insertIntoCache(Serial serial, uint32_t index, uint32_t deletedChildren, int threadID)
{
	// Handle the case where our array is empty
	if (numVisitedBranches[threadID] == 0)
	{
		visitedBranches[threadID] = malloc((++numVisitedBranches[threadID]) * sizeof(Serial)); // numVisitedBranches should be 0 here
		visitedBranches[threadID][index] = serial;
		return;
	}

	// Handle the case where we are deleting children
	if (deletedChildren > 0)
	{
		visitedBranches[threadID][index] = serial;

		// We're just replacing the child with the parent
		if (deletedChildren == 1)
			return;

		// We will realloc with a smaller space, but we need to perform the memmove first
		memmove(visitedBranches[threadID] + index + 1, visitedBranches[threadID] + index + deletedChildren, (numVisitedBranches[threadID] - index - deletedChildren) * sizeof(Serial));
		numVisitedBranches[threadID] = numVisitedBranches[threadID] - deletedChildren + 1;
		visitedBranches[threadID] = realloc(visitedBranches[threadID], numVisitedBranches[threadID] * sizeof(Serial));
		return;
	}

	// We are increasing the arr size
	visitedBranches[threadID] = realloc(visitedBranches[threadID], (++numVisitedBranches[threadID]) * sizeof(Serial));

	if (index < numVisitedBranches[threadID] - 1)
		memmove(&visitedBranches[threadID][index + 1], &visitedBranches[threadID][index], (numVisitedBranches[threadID] - index - 1) * sizeof(Serial));

	visitedBranches[threadID][index] = serial;
}

/*-------------------------------------------------------------------
 * Function : writeVisitedNodesToDisk
 *
 * Create/overwrite thread-specific cache file and populate with visitedBranches data
 -------------------------------------------------------------------*/
void writeVisitedNodesToDisk(int threadID) {

	recipeLog(3, "Serialization", "Cache", "Visited Nodes", "Saving visited nodes to disk... This may take a few seconds.");

	char filename[50] = { 0 };
	sprintf(filename, "results/visitedNodes_%d.dat", threadID);

	FILE* fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		recipeLog(3, "Serialization", "Cache", filename, "Error opening results/visitedNodes.dat for writing.");
		return;
	}

	uint32_t serialsWritten = 0;

	// First write the number of serials so we know how much memory to malloc when we read in the file
	fwrite(&numVisitedBranches[threadID], sizeof(uint32_t), 1, fp);

	serialsWritten = writeSerialsToDisk(fp, threadID);

	char result[100];
	if (serialsWritten == numVisitedBranches[threadID])
		sprintf(result, "Successfully wrote %d serials to disk", serialsWritten);
	else
		sprintf(result, "Only able to write %d of %d serials to disk", serialsWritten, numVisitedBranches[threadID]);
	recipeLog(3, "Serialization", "Cache", filename, result);

	fclose(fp);
}

/*-------------------------------------------------------------------
 * Function : writeSerialsToDisk
 *
 * Walk across a given thread's visitedBranches and store the serial
 * length and data to file. This assumes we've already written numVisitedBranches
 -------------------------------------------------------------------*/
uint32_t writeSerialsToDisk(FILE* fp, int threadID) {
	uint32_t i;
	size_t ret;
	for (i = 0; i < numVisitedBranches[threadID]; i++) {
		Serial serial = visitedBranches[threadID][i];
		ret = fwrite(&serial.length, sizeof(uint8_t), 1, fp);
		if (ret != 1)
			break;
		ret = fwrite(serial.data, sizeof(char), serial.length, fp);
		if (ret != serial.length)
			break;
	}

	return i;
}

/*-------------------------------------------------------------------
 * Function : consolidateThreadSerialsOnShutdown
 *
 * Take all visited nodes in memory and consolidate them into one
 * disk file, taking care to not include duplicates or children.
 -------------------------------------------------------------------*/
void consolidateThreadSerialsOnShutdown(int workerCount)
{
	const char* tempFileName = "results/visitedNodes.temp";
	const char* finalFileName = "results/visitedNodes.dat";

	FILE* fp = NULL;
	fp = fopen(tempFileName, "wb");
	if (fp == NULL)
	{
		recipeLog(2, "Shutdown", "Cache", "Visited Nodes", "Unable to open results/visitedNodes.temp for writing");
		return;
	}

	// Sum the number of visited nodes so we can malloc the combined array
	// We will certainly never use this full length as there will be duplicate nodes,
	// but this will prevent constant reallocs
	uint32_t totalNodesExpected = sumVisited(numVisitedBranches, workerCount);
	Serial* combined = malloc(totalNodesExpected * sizeof(Serial));
	checkMallocFailed(combined);

	// Keep track of which indices we are looking at for each thread
	uint32_t* indices = calloc(workerCount, sizeof(uint32_t));
	checkMallocFailed(indices);

	// Used to track if we are about to insert a child node based off what we have previously inserted
	Serial prevSerial = (Serial){ 0, NULL };
	uint32_t totalNodesCombined = 0;

	Serial nextSerial = (Serial){ 0, NULL };
	while ((nextSerial = nextInsertFromMemory(indices, workerCount)).length > 0)
	{
		if (prevSerial.length > 0 && isChild(prevSerial, nextSerial))
		{
			free(nextSerial.data);
			continue;
		}

		prevSerial = nextSerial;
		combined[totalNodesCombined++] = nextSerial;
	}

	// Free the global arrays now that everything has been consolidated into one array
	free(visitedBranches);
	free(numVisitedBranches);

	// Begin writing to file
	size_t ret = fwrite(&totalNodesCombined, sizeof(uint32_t), 1, fp);

	for (uint32_t i = 0; i < totalNodesCombined; i++) {
		Serial serial = combined[i];
		ret = fwrite(&serial.length, sizeof(uint8_t), 1, fp);
		if (ret != 1)
			break;
		ret = fwrite(serial.data, sizeof(char), serial.length, fp);
		if (ret != serial.length)
			break;

		free(serial.data);
	}

	fclose(fp);

	free(combined);
	free(indices);

	// Delete the previous consolidated file, if it exists
	remove(finalFileName);

	// Now rename the temp file and nuke the thread-specific files
	int renameRet = rename(tempFileName, finalFileName);
	if (renameRet != 0)
	{
		recipeLog(2, "Shutdown", "Cache", "Visited Nodes", "Failed to rename temp file.");
		return;
	}

	// Now nuke the thread-specific files
	for (int i = 0; i < workerCount; i++)
	{
		char filename[50];
		sprintf(filename, "results/visitedNodes_%d.dat", i);

		remove(filename);
	}

	char result[100];
	sprintf(result, "Successfully consolidated %d unique nodes to disk.", totalNodesCombined);
	recipeLog(2, "Shutdown", "Cache", "Visited Nodes", result);
}

/*-------------------------------------------------------------------
 * Function : sumVisited
 *
 * Helper function to sum the number of nodes visited by all threads.
 -------------------------------------------------------------------*/
int sumVisited(uint32_t* visitedArr, int workerCount)
{
	int sum = 0;
	for (int i = 0; i < workerCount; i++)
		sum += visitedArr[i];

	return sum;
}

Serial nextInsertFromMemory(uint32_t* indices, int workerCount)
{
	Serial lowestSerial = (Serial){ 0, NULL };
	int lowestSerialThread = -1;

	for (int i = 0; i < workerCount; i++)
	{
		uint32_t curIdx = indices[i];
		if (curIdx == numVisitedBranches[i])
			continue;

		Serial curSerial = visitedBranches[i][curIdx];

		if (lowestSerial.length == 0)
		{
			lowestSerial = curSerial;
			lowestSerialThread = i;
			continue;
		}

		int cmp = serialcmp(curSerial, lowestSerial);

		if (cmp < 0)
		{
			lowestSerial = curSerial;
			lowestSerialThread = i;
		}
		else if (cmp == 0)
		{
			// This is a duplicate node, free the duplicate data
			free(curSerial.data);
			indices[i]++;
		}
	}

	if (lowestSerialThread >= 0)
		indices[lowestSerialThread]++;

	return lowestSerial;
}