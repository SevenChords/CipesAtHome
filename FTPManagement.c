#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct url_data {
	size_t size;
	char* data;
};

char *handle_url(char* url) {
	CURL *curl;

	struct url_data data;
	data.size = 0;
	data.data = malloc(4096);
	
	data.data[0] = '\0';

	CURLcode res;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			printf("Error");
		}
		
		curl_easy_cleanup(curl);
	}

	return data.data;
}

int getFastestRecordOnBlob() {
	char* data;

	data = handle_url("https://hundorecipes.blob.core.windows.net/foundpaths/fastestFrames.txt");

	if(data) {
		printf("%s\n", data);
		int record = atoi(strtok(data, "\n")); // I'm trying to get rid of the newline char, but it's causing a segfault?
		free(data);
		return record;
	}	
	
	return 0;
}

int main() {
	printf("%d\n", getFastestRecordOnFTP());
}
