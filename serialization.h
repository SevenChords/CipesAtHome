#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdint.h>
#include <stdio.h>

#include "types.h"

// Startup
void		initializeVisitedNodes(int workerCount);
uint32_t	ReadConsolidatedFile(Serial** combined);
Serial		readNextSerial(FILE* fp);
FILE**		getCacheFilePtrs(int workerCount);
uint32_t	mergeThreadSerials(Serial** combined, FILE** fp, int workerCount);
int			sumVisited(uint32_t* visitedArr, int workerCount);
Serial		nextInsert(Serial* curSerials, FILE** fp, int workerCount);
void		closeCacheFilePtrs(FILE** fp, int workerCount);
Serial*		deepCopy(Serial* src, uint32_t len);

// mid-run
void 		cacheSerial(BranchPath* node);
uint32_t	indexToInsert(Serial serial, int low, int high, int threadID);
uint32_t	deleteAndFreeChildSerials(Serial serial, uint32_t index, int threadID);
void		insertIntoCache(Serial serial, uint32_t index, uint32_t deletedChildren, int threadID);
bool		legalMoveHasBeenTraversed(BranchPath* newLegalMove);
int			searchVisitedNodes(Serial serial, int low, int high, int threadID);
void 		serializeNode(BranchPath* node);
uint8_t 	serializeCookNode(BranchPath* node, void** data);
uint8_t		getRecipeIndex(Cook* pCook);
uint8_t 	serializeSortNode(BranchPath* node, void** data);
uint8_t 	serializeCH5Node(BranchPath* node, void** data);
void		writeVisitedNodesToDisk(int threadID);
uint32_t	writeSerialsToDisk(FILE* fp, int threadID);

// Shutdown
void		consolidateThreadSerialsOnShutdown(int workerCount);
Serial		nextInsertFromMemory(uint32_t* indices, int workerCount);

#endif