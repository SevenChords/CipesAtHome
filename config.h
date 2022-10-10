#include <libconfig.h>

void initConfig();

const char* getConfigStr(char* str);
int getConfigInt(char* str);

void shutdownConfig();