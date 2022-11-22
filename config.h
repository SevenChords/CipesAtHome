#include <libconfig.h>

void initConfig();
void validateConfig();

const char* getConfigStr(char* str);
int getConfigInt(char* str);

void shutdownConfig();