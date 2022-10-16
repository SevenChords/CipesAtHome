#ifndef START_H
#define START_H

#include "types.h"

int getLocalRecord(); // May get a value <0 if local record was corrupt.
void setLocalRecord(int frames);

uint64_t getSysRNG();

int main(); // Main method for entire algorithm

#endif
