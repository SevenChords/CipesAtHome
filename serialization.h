#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdint.h>
#include <stdio.h>

#include "types.h"

uint32_t	indexToInsert(Serial serial, int low, int high, int threadID);
void		insertIntoCache(Serial serial, uint32_t index, uint32_t deletedChildren, int threadID);
uint32_t	deleteAndFreeChildSerials(Serial serial, uint32_t index, int threadID);
int			searchVisitedNodes(Serial serial, int low, int high, int threadID);
void 		cacheSerial(BranchPath* node);
void 		serializeNode(BranchPath* node);
uint8_t 	serializeCookNode(BranchPath* node, void** data);
uint8_t 	serializeSortNode(BranchPath* node, void** data);
uint8_t 	serializeCH5Node(BranchPath* node, void** data);
bool		legalMoveHasBeenTraversed(BranchPath* newLegalMove);
void		writeVisitedNodesToDisk(int threadID);
uint32_t	readVisitedNodesFromDisk(int threadID, Serial** arr);
void		initializeVisitedNodes(int workerCount);
uint32_t	writeSerialsToDisk(FILE* fp, int threadID);
uint32_t	readSerialsFromDisk(FILE* fp, Serial* arr, uint32_t numVisited);
uint32_t	mergeThreadSerials(Serial** combined, uint32_t combinedLen, Serial* threadSerials, uint32_t threadLen);

#endif