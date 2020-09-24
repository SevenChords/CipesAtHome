#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include "config.h"

config_t *config;

/*-------------------------------------------------------------------
 * Function 	: initConfig
 * Inputs	: 
 * Outputs	: config_t config
 *
 * This uses the libconfig library to parse a user-modifiable config file.
 * The config structure can be accessed to obtain specific config settings.
 -------------------------------------------------------------------*/
void initConfig() {
	config_t *configInstance = malloc(sizeof(config_t));
	config_init(configInstance);
	if (config_read_file(configInstance, "config.txt") == CONFIG_FALSE) {
		printf("Could not read config file! Please assure that config.txt is in the root directory and is formatted correctly.\n");
		printf("Press ENTER to exit.");
		getchar();
		exit(1);
	}

	config = configInstance;
}

const char *getConfigStr(char *str) {
	const char *temp;
	config_lookup_string(config, str, &temp);
	return temp;
}

int getConfigInt(char* str) {
	int temp;
	config_lookup_int(config, str, &temp);
	return temp;
}
