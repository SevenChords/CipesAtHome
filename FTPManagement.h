#include "absl/base/port.h"

ABSL_MUST_USE_RESULT  // Output is newly allocated and needs to be freed at some point
char *handle_get(char* url);
void handle_post(char* url, FILE *fp, int localRecord, char *nickname);
int getFastestRecordOnBlob();
int testRecord(int localRecord);
void setLocalVer(const char* ver);
int checkForUpdates(const char *local_ver);
int checkGithubVer();
