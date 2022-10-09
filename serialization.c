#include "serialization.h"

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"
#include "logger.h"

Serial** visitedBranches = NULL;
uint32_t* numVisitedBranches = NULL;

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
 * Function : void writeSerialsToDisk
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
 * Function : void writeVisitedNodesToDisk
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

/*-------------------------------------------------------------------
 * Function : mergeThreadSerials
 *
 * Referencing file pointers, merge all serials into one combined arr.
 -------------------------------------------------------------------*/
uint32_t mergeThreadSerials(Serial** combined, FILE** fp, int workerCount)
{
	// For each thread, store the number of expected serials
	uint32_t* numThreadVisited = malloc(workerCount * sizeof(uint32_t));
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

	// Obtain file pointers to the file for each of the workerCount threads
	FILE** fp = getCacheFilePtrs(workerCount);

	// Read in data from file for each thread and merge to a combined array
	uint32_t combinedLen = mergeThreadSerials(&combined, fp, workerCount);

	// Initialize global array of arrays, one for each thread
	visitedBranches = malloc(workerCount * sizeof(Serial*));
	checkMallocFailed(visitedBranches);
	numVisitedBranches = calloc(workerCount, sizeof(uint32_t));
	checkMallocFailed(numVisitedBranches);

	// Perform deep copies so each thread can manage visited nodes independently
	// First thread can just use the array
	visitedBranches[0] = combined;

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