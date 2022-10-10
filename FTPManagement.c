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

const char* local_ver;

/*-------------------------------------------------------------------
 * Function : setLocalVer
 * 
 * Stores the version of Recipes@Home the user is running, to compare
 * against the Github repo for future updates.
 -------------------------------------------------------------------*/
void setLocalVer(const char* ver)
{
	local_ver = ver;
}

/*-------------------------------------------------------------------
 * Function 	: write_data
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
 *
 * This is the main cURL GET function. Communicate with GitHub
 * or the Blob server to obtain the latest program version or
 * the current fastest frame record respectively.
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
 * Function : getFastestRecordOnBlob
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
 * Function : handle_post
 *
 * Used to submit a user's roadmap to the blob server.
 -------------------------------------------------------------------*/
void handle_post(char* url, FILE *fp, int localRecord, char *nickname) {
	struct memory wt;
	struct memory rt;
	rt.data = NULL;
	rt.size = 0;

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	wt.data = malloc(fsize + strlen(nickname) + 50); // Offer enough padding for postfields

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
 * Function : testRecord
 *
 * Read the user's fastest record and submit to the Blob server
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
 * Function : checkForUpdates
 *
 * Compare the user's local version versus the most recent Github release.
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

int checkGithubVer()
{
	int update = checkForUpdates(local_ver);
	if (update == -1) {
		printf("Could not check version on Github. Please check your internet connection.\n" \
			"Otherwise, we can't submit completed roadmaps to the server!\n" \
			"Alternatively you may have been rate-limited. Please wait a while and try again.\n");
	}
	else if (update == 1) {
		printf("There is a newer version of Recipes@Home.\n" \
			"To continue, please visit https://github.com/SevenChords/CipesAtHome/releases to download the newest version of this program!\n" \
			"Press ENTER to quit.\n");
		awaitKeyFromUser();
	}

	return update;
}