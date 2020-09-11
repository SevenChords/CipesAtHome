enum request {
	GET,
	POST
};

char *handle_get(char* url);
int handle_post(char* url, FILE *fp, int localRecord, char *nickname);
int getFastestRecordOnBlob();
int testRecord(int localRecord);
int checkForUpdates(char *ver);
