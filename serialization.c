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
 * Walk across visitedBranches and store the serial length and data
 * to visitedNodes.dat. This assumes we've already written numVisitedBranches
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
 * Create/overwrite visitedNodes.dat and populate with visitedBranches data
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
 * Function : void readSerialsFromDisk
 *
 * Loop so long as we can continue reading bytes up to the expected
 * numVisitedBranches serials in the binary file.
 * CAUTION: This function assumes that fp seek position is at the
 * first serial, i.e. we've already fread'd numVisitedBranches
 -------------------------------------------------------------------*/
uint32_t readSerialsFromDisk(FILE* fp, Serial* arr, uint32_t numVisited) {
	size_t ret = 0;
	uint32_t i = 0;
	for (i = 0; i < numVisited; i++) {
		Serial serial = (Serial){ 0, NULL };
		ret = fread(&serial.length, sizeof(uint8_t), 1, fp);
		if (ret != 1)
			break;

		serial.data = malloc(serial.length * sizeof(char));
		checkMallocFailed(serial.data);

		ret = fread(serial.data, sizeof(char), serial.length, fp);
		if (ret != serial.length) {
			free(serial.data);
			break;
		}
		arr[i] = serial;
	}

	return i;
}

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

void initializeVisitedNodes(int workerCount)
{
	recipeLog(2, "Startup", "Cache", "Visited Nodes", "Reading visited nodes from disk... This may take a few seconds.");

	// Initialize array of arrays, one for each thread
	visitedBranches = malloc(workerCount * sizeof(Serial*));
	checkMallocFailed(visitedBranches);
	numVisitedBranches = calloc(workerCount, sizeof(uint32_t));
	checkMallocFailed(numVisitedBranches);

	// Maintain one array of serials, which we will constantly add to for every thread
	// This way on start-up, all threads will start with the same combined list of visited nodes
	Serial* combined = NULL;

	uint32_t sumVisitedBranches = 0;
	for (int i = 0; i < workerCount; i++)
	{
		Serial* temp = NULL;
		uint32_t threadVisited = readVisitedNodesFromDisk(i, &temp);
		if (temp != NULL)
		{
			// Now merge temp with combined
			sumVisitedBranches = mergeThreadSerials(&combined, sumVisitedBranches, temp, threadVisited);
		}
	}

	// TODO: When a serial is skipped over, we did not necessarily add threadVisited serials
	// TODO: Can children accidentally exist in this merged array? Does it really matter?

	// Now that all thread files have been consolidated to one in-memory array,
	// perform a deep copy on the array and assign to each of the thread-specific in-memory arrays

	// First thread can just use the array
	visitedBranches[0] = combined;

	for (int i = 1; i < workerCount; i++)
	{
		visitedBranches[i] = deepCopy(combined, sumVisitedBranches);
		numVisitedBranches[i] = sumVisitedBranches;
	}

	if (sumVisitedBranches == 0)
		recipeLog(2, "Startup", "Cache", "Visited Nodes", "No cached visited nodes on disk.");

	char result[100];
	sprintf(result, "Found %d serials from disk", sumVisitedBranches);
	recipeLog(2, "Startup", "Cache", "Visited Nodes", result);
}

/*-------------------------------------------------------------------
 * Function : void readVisitedNodesFromDisk
 *
 * Check if results/visitedNodes.dat exists.
 * If so, populate visitedBranches with the data
 -------------------------------------------------------------------*/
uint32_t readVisitedNodesFromDisk(int threadID, Serial** arr) {
	char filename[50] = { 0 };
	sprintf(filename, "results/visitedNodes_%d.dat", threadID);

	FILE* fp = fopen(filename, "rb");
	if (fp == NULL)
		return 0;

	// The first 4 bytes represent the number of visited nodes to expect. malloc enough space to contain this
	uint32_t threadVisited = 0;
	size_t ret = fread(&threadVisited, sizeof(uint32_t), 1, fp);
	if (ret != 1) {
		recipeLog(2, "Startup", "Cache", filename, "There was an error reading the cache file. Size not recognized");
		fclose(fp);
		return 0;
	}

	*arr = malloc(threadVisited * sizeof(Serial));
	checkMallocFailed(*arr);

	uint32_t serialsRead = readSerialsFromDisk(fp, *arr, threadVisited);

	fclose(fp);

	if (serialsRead < threadVisited) {
		// We reached EOF before expected. Keep what was read thus far
		char result[100];
		sprintf(result, "WARNING: Expected %d cached serials, only found %d", threadVisited, serialsRead);
		recipeLog(2, "Startup", "Cache", filename, result);
		*arr = realloc(*arr, serialsRead * sizeof(Serial));
	}

	return serialsRead;
}

uint32_t mergeThreadSerials(Serial** combined, uint32_t combinedLen, Serial* threadSerials, uint32_t threadLen)
{
	// If this is the first array we're "combining" then there's nothing to do
	if (combinedLen == 0)
	{
		*combined = threadSerials;
		return threadLen;
	}

	uint32_t i = 0, j = 0;

	while (i < threadLen && j < combinedLen)
	{
		Serial threadSerial = threadSerials[i];
		int ret = serialcmp(threadSerial, (*combined)[j]);

		if (ret == 0)
		{
			// These are identical, so just free this node and skip the insert
			free(threadSerials[i++].data);
		}
		else if (ret < 0)
		{
			// Copy serial to index i in the combined array
			*combined = realloc(*combined, (combinedLen + 1) * sizeof(Serial));
			// Only need to move if we're not inserting at the end of the array
			if (j < combinedLen - 1)
			{
				Serial* dest = *combined + j + 1;
				Serial* src = *combined + j;
				uint32_t serialsToMove = combinedLen - j;
				memmove(dest, src, serialsToMove * sizeof(Serial));
			}

			(*combined)[j] = threadSerial;
			i++;
			combinedLen++;
		}

		j++;
	}

	// Copy any leftovers from thread array to end of consolidated array
	if (i < threadLen)
	{
		uint32_t serialsToCopy = threadLen - i;
		combinedLen += serialsToCopy;
		*combined = realloc(*combined, combinedLen * sizeof(Serial));
		memcpy(*combined + j, threadSerials + i, serialsToCopy * sizeof(Serial));
	}

	return combinedLen;
}

/*-------------------------------------------------------------------
 * Function : indexToInsert
 *
 * Perform a binary search to determine
 * where we should insert the given serial.
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