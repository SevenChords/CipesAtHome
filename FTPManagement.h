#include "absl/base/port.h"

ABSL_MUST_USE_RESULT_INCLUSIVE char *handle_get(char* url);
void handle_post(char* url, FILE *fp, int localRecord, char *nickname);
int getFastestRecordOnBlob();
int testRecord(int localRecord);
int checkForUpdates(const char *local_ver);
