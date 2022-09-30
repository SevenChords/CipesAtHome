#include "calculator.h"

void printCh5Data(const BranchPath* curNode, const MoveDescription desc, FILE* fp);
void printCh5Sort(const CH5* ch5Data, FILE* fp);
void printCookData(const BranchPath* curNode, const MoveDescription desc, FILE* fp);
void printFileHeader(FILE* fp);
void printInventoryData(const BranchPath* curNode, FILE* fp);
void printOutputsCreated(const BranchPath* curNode, FILE* fp);
void printNodeDescription(const BranchPath * curNode, FILE * fp);
void printResults(const char* filename, const BranchPath* path);
void printSortData(FILE* fp, enum Action curNodeAction);