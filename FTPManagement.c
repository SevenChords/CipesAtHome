#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "FTPManagement.h"
#include <libconfig.h>
#include "config.h"
#include "logger.h"
#include "base.h"

struct memory {
	char *data;
	size_t size;
};

/*-------------------------------------------------------------------
 * Function 	: write_data
 * Inputs	: char 	 *contents
 *		  size_t 	 size
 *		  size_t 	 nmemb
 *		  void 	 *userdata
 * Outputs	: static size_t realsize
 *
 * This is a child function called by handle_get. This function writes
 * data to a buffer as data is received by a cURL request.
 -------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------
 * Function 	: handle_get
 * Inputs	: char	*url
 * Outputs	: char	*data
 *
 * This is the main cURL GET function. Communicate with either GitHub
 * or the Blob server to obtain either the latest program version or
 * the current fastest frame record respectively.
 * The returning value (if non-NULL) MUST be freed.
 -------------------------------------------------------------------*/
ABSL_MUST_USE_RESULT char *handle_get(char* url) {
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
			curl_easy_cleanup(curl);
			return NULL;
		}

		curl_easy_cleanup(curl);
	}

	return chunk.data;
}

/*-------------------------------------------------------------------
 * Function 	: getFastestRecordOnBlob
 * Inputs	:
 * Outputs	: int record
 *
 * Retrieve the server's current fastest roadmap length.
 -------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------
 * Function 	: handle_post
 * Inputs	: char	*url
 *		  FILE	*fp
 *		  int	localRecord
 *		  char	*nickname
 * Outputs	: int 	status
 *
 * Retrieve the server's current fastest roadmap length.
 -------------------------------------------------------------------*/
void handle_post(char* url, FILE *fp, int localRecord, char *nickname) {
	struct memory wt;
	struct memory rt;
	rt.data = NULL;
	rt.size = 0;

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	wt.data = malloc(fsize+ strlen(nickname) + 50); // Offer enough padding for postfields

	checkMallocFailed(wt.data);

	size_t bytes_written;
	bytes_written = sprintf(wt.data, "{\"frames\":\"%d\",\"userName\":\"%s\",\"routeContent\":\"", localRecord, nickname);
	size_t fileBytesRead = fread(wt.data + bytes_written, 1, fsize, fp);
	fclose(fp);
	if (fileBytesRead != fsize)
	{
		recipeLog(1, "Server", "Upload", "Error", "Unable to read from file!");
		free(wt.data);
		return;
	}
	sprintf(wt.data + fsize + bytes_written, "\"}");

	CURL *curl = curl_easy_init();
	if (curl) {
		cJSON *json = cJSON_Parse(wt.data);
		char *json_str = cJSON_PrintUnformatted(json);
		cJSON_Delete(json);
		checkMallocFailed(json_str);
		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "charset: utf-8");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rt);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_perform(curl);
		curl_slist_free_all(headers);
		free(json_str);

		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	free(wt.data);

	// Log the body of the return of the POST request
	recipeLog(1, "Server", "Upload", "Response", rt.data);
	free(rt.data);
}


/*-------------------------------------------------------------------
 * Function 	: testRecord
 * Inputs	: int localRecord
 * Outputs	: -1 - error locating the text file
 *		  0  - successful submission to the server
 *
 * Retrieve the server's current fastest roadmap length.
 -------------------------------------------------------------------*/
int testRecord(int localRecord) {
	if (localRecord < 0) {
		printf("Record submitted is invalid (less then 0). Likely due to corruption of the PB.txt file or unexpected error. Not submitting\n");
		return -2;
	}

	char filename[32];
	char *folder = "results/";
	char *extension = ".txt";
	sprintf(filename, "%s%d%s", folder, localRecord, extension);

	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("Error could not locate file: %s. Please submit an issue on github including your OS version.\n", filename);
		return -1;
	}

	const char* username = getConfigStr("Username");
	char nickname[20];
	strncpy(nickname, username, 19);
	nickname[19] = '\0';
	handle_post("https://hundorecipes.azurewebsites.net/api/uploadAndVerify", fp, localRecord, nickname);

	return 0;
}

/*-------------------------------------------------------------------
 * Function 	: checkForUpdates
 * Inputs	: const char *local_ver
 * Outputs	: 1  - outdated local version
 *		  -1 - unable to retrieve server version
 *		  0  - up-to-date
 *
 * Retrieve the server's current fastest roadmap length.
 -------------------------------------------------------------------*/
int checkForUpdates(const char *local_ver) {
	char *url = "https://api.github.com/repos/SevenChords/CipesAtHome/releases/latest";
	char *data = handle_get(url);
	if (data == NULL) {
		return -1;
	}
	cJSON *json = cJSON_Parse(data);
	cJSON *json_item = cJSON_GetObjectItemCaseSensitive(json, "tag_name");
	char *ver = cJSON_GetStringValue(json_item);
	free(data);

	if (ver == NULL) {
		cJSON_Delete(json);
		return -1;
	}

	// Compare local version with github version
	if (strncmp(local_ver, ver, 4) != 0) {
		cJSON_Delete(json);
		return 1;
	}

	// Add logs
	cJSON_Delete(json);
	return 0;
}
