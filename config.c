#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include "config.h"

/*-------------------------------------------------------------------
 * Function 	: getConfig
 * Inputs	: 
 * Outputs	: config_t config
 *
 * This uses the libconfig library to parse a user-modifiable config file.
 * The config structure can be accessed to obtain specific config settings.
 -------------------------------------------------------------------*/
config_t *getConfig() {
	config_t *config = malloc(sizeof(config_t));
	config_init(config);
	if (config_read_file(config, "config.txt") == CONFIG_FALSE) {
		printf("Could not read config file! Please assure that config.txt is in the root directory and is formatted correctly.\n");
		return NULL;
	}

	return config;
}
