#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct memory {
	char *data;
	size_t size;
};

// Function to write data from HTTP request
static size_t write_data(char *contents, size_t size, size_t nmemb, void *userdata) {
	size_t realsize = size * nmemb;
	struct memory *mem = (struct memory *)userdata;
	
	char *ptr = realloc(mem->data, mem->size + realsize + 1);
	if (ptr == NULL)
		return 0;
	mem->data = ptr;
	memcpy(&mem->data[mem->size], contents, realsize);
	mem->size += realsize;
	mem->data[mem->size] = 0;
	return realsize;
}

// Main curl function
char *handle_url(char* url) {
	CURL *curl;
	struct memory chunk;
	chunk.data = NULL;
	chunk.size = 0;

	CURLcode res;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			printf("cURL ERROR!");
		}
		
		curl_easy_cleanup(curl);
	}

	return chunk.data;
}

int getFastestRecordOnBlob() {
	char* data;

	data = handle_url("https://hundorecipes.blob.core.windows.net/foundpaths/fastestFrames.txt");

	if(data) {
		int record = atoi(data);
		free(data);
		return record;
	}
	
	return 0;
}

int main() {
	getFastestRecordOnBlob();
}
