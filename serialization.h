#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdint.h>
#include <stdio.h>

#include "types.h"

// Startup
void		initializeVisitedNodes(int workerCount);
uint32_t	readVisitedNodesFromDisk(int threadID, Serial** arr);
uint32_t	readSerialsFromDisk(FILE* fp, Serial* arr, uint32_t numVisited);
uint32_t	mergeThreadSerials(Serial** output, Serial** threadVisitedArr, uint32_t* numThreadVisited, int workerCount);
int			nextInsert(Serial** threadVisitedArr, uint32_t* numThreadVisited, uint32_t* indices, int workerCount, uint32_t* nextIndex);
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
uint8_t 	serializeSortNode(BranchPath* node, void** data);
uint8_t 	serializeCH5Node(BranchPath* node, void** data);
void		writeVisitedNodesToDisk(int threadID);
uint32_t	writeSerialsToDisk(FILE* fp, int threadID);

#endif