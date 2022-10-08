#ifndef START_H
#define START_H

#include "types.h"

int getLocalRecord(); // May get a value <0 if local record was corrupt.
void setLocalRecord(int frames);
const char* getLocalVersion();

int main(); // Main method for entire algorithm

#endif
