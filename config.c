#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include "config.h"

config_t *getConfig() {
	config_t *config = malloc(sizeof(config_t));
	config_init(config);
	if (config_read_file(config, "config.txt") == CONFIG_FALSE) {
		printf("Could not read config file! Please assure that config.txt is in the root directory and is formatted correctly.\n");
		return NULL;
	}

	return config;
}

int main() {
	// Example of extracting config information
	/*config_t *config = getConfig();
	const char *buf = malloc(32);
	int value;

	if (config_lookup_int(config, "select", &value))
		printf("Select = %d\n", value);
	if (config_lookup_int(config, "randomise", &value))
		printf("Randomise = %d\n", value);
	if (config_lookup_int(config, "logLevel", &value))
		printf("logLevel = %d\n", value);
	if (config_lookup_int(config, "workerCount", &value))
		printf("workerCount = %d\n", value);
	if (config_lookup_string(config, "Username", &buf))
		printf("Username = %s\n", buf);
	if (config_lookup_string(config, "Version", &buf))
		printf("Version = %s\n", buf);

	return 1;*/
}
