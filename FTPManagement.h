enum request {
	GET,
	POST
};

static size_t write_data(char *contents, size_t size, size_t nmemb, void *userdata);
char *handle_get(char* url);
int handle_post(char* url, FILE *fp, int localRecord, char *nickname);
int getFastestRecordOnBlob();
int testRecord(int localRecord);
int checkForUpdates();
