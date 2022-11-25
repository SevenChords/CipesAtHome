#include <stdio.h>
#include <stdlib.h>
#include "logger.h"
#include "config.h"
#include <time.h>

int level_cfg;

int init_level_cfg() {
	level_cfg = getConfigInt("logLevel");
	return 0;
}

int recipeLog(int level, char *process, char *subProcess, char *activity, char *entry) {
	if (level_cfg >= level) {
		int hours, mins, secs, month, day, year;
		time_t now;
		time(&now);
		struct tm* local = localtime(&now);
		hours = local->tm_hour;
		mins = local->tm_min;
		secs = local->tm_sec;
		day = local->tm_mday;
		month = local->tm_mon + 1;
		year = local->tm_year + 1900;
		char date[100];
		char data[200];
		sprintf(date, "[%d-%02d-%02d %02d:%02d:%02d]", year, month, day, hours, mins, secs);
		sprintf(data, "[%s][%s][%s][%s]\n", process, subProcess, activity, entry);
		printf("%s%s", date, data);

		FILE* fp = fopen("recipes.log", "a");
		fprintf(fp, "%s%s", date, data);
		fclose(fp);
	}
	return 0;
}
