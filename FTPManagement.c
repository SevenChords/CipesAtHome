#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "FTPManagement.h"

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
char *handle_get(char* url) {
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
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.68.0");
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

	data = handle_get("https://hundorecipes.blob.core.windows.net/foundpaths/fastestFrames.txt");

	if(data) {
		int record = atoi(data);
		free(data);
		return record;
	}
	
	// Log
	
	return 0;
}

static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp) {
	struct memory *wt = (struct memory *)userp;
	size_t buffer_size = size * nmemb;
	
	if (wt->size > 0) {
		size_t copy_this_much = wt->size;
		if (copy_this_much > buffer_size)
			copy_this_much = buffer_size;
		memcpy(dest, wt->data, copy_this_much);

		wt->data += copy_this_much;
		wt->size -= copy_this_much;
		return copy_this_much; // We transferred this many bytes
	}

	return 0;
}

int handle_post(char* url, FILE *fp, int localRecord, char *nickname) {
	CURL *curl;
	CURLcode res;
	struct memory wt;
	
	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	wt.data = malloc(fsize+ strlen(nickname) + 49); // Offer enough padding for postfields
	size_t bytes_written;
	bytes_written = sprintf(wt.data, "{\"frames\":\"%d\",\"userName\":\"%s\",\"routeContent\":\"", localRecord, nickname);
	fread(wt.data + bytes_written, 1, fsize, fp);
	fclose(fp);
	sprintf(wt.data + fsize + bytes_written, "\"}");
	
	res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res != CURLE_OK) {
		printf("cURL ERROR!");
		return 1;
	}

	curl = curl_easy_init();
	if (curl) {
		cJSON *json = cJSON_Parse(wt.data);
		char *json_str = cJSON_PrintUnformatted(json);
		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "charset: utf-8");
		
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		res = curl_easy_perform(curl);
		
		if(res != CURLE_OK)
			printf("cURL FAILED!");
		
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	free(wt.data);
	
	// Add logs based on file upload success
	return 0;
}

// Returns 1 when local record is NOT faster than remote record
int testRecord(int localRecord) {
	int remoteRecord = getFastestRecordOnBlob();
	char filename[32];
	char *folder = "results/";
	char *extension = ".txt";
	sprintf(filename, "%s%d%s", folder, localRecord, extension);

	if (localRecord > remoteRecord) {
		return 1;
	}

	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("Error could not locate file: %s. Please submit an issue on github including your OS version.\n", filename);
		return -1;
	}

	char* data;
	handle_post("https://hundorecipes.azurewebsites.net/api/uploadAndVerify", fp, localRecord, "C_TEST");
	return 0;
}

int checkForUpdates() {
	char *url = "https://api.github.com/repos/SevenChords/CipesAtHome/releases/latest";
	char *data = handle_get(url);
	printf("Data: %s\n", data);
	cJSON *json = cJSON_Parse(data);
	json = cJSON_GetObjectItemCaseSensitive(json, "tag_name");
	char *tag_name = cJSON_Print(json);
	printf("Tag Name: %s\n", tag_name);
	free(tag_name);
	// Add logs
	
	// TODO: Use getConfig to compare tag name with config file
	return 0;
}

/*
int main() {
	//checkForUpdates();
}*/
