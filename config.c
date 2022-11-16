#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>
#include "config.h"
#include "base.h"
#include "types.h"

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
		awaitKeyFromUser();
		exit(1);
	}

	config = configInstance;
}

enum SettingType {
	intSetting,
	int64Setting,
	floatSetting,
	boolSetting,
	stringSetting
};

/*-------------------------------------------------------------------
 * Function: validateSetting
 *
 * For a given setting in the config file, check that it is present
 * and of the given type. If not, inform the user of the error and
 * increment the count parameter.
 -------------------------------------------------------------------*/
void validateSetting(const char *path, enum SettingType type, int *errorCount) {
	char *typeName = "";
	switch (type) {
	case intSetting:
	case int64Setting:
		typeName = "integer";
		break;
	case floatSetting:
		typeName = "floating point number";
		break;
	case boolSetting:
		typeName = "boolean";
		break;
	case stringSetting:
		typeName = "string";
	}
	// We look up the value in two different ways in order to give the user
	// a more specific error message. First, check that it is present at all.
	if (config_lookup(config, path) == NULL) {
		++*errorCount;
		printf("The %s setting %s is not present in config.txt.\n", typeName, path);
		return;
	}
	// Now, ignore that previous lookup and check that it is the correct type.
	int ret = CONFIG_FALSE;
	switch (type) {
		int intValue;
		long long llValue;
		double floatValue;
		int boolValue;
		char *stringValue;
	case intSetting:
		ret = config_lookup_int(config, path, &intValue);
		break;
	case int64Setting:
		ret = config_lookup_int64(config, path, &llValue);
		break;
	case floatSetting:
		ret = config_lookup_float(config, path, &floatValue);
		break;
	case boolSetting:
		ret = config_lookup_bool(config, path, &boolValue);
		break;
	case stringSetting:
		ret = config_lookup_string(config, path, &stringValue);
	}
	if (ret == CONFIG_FALSE) {
		++*errorCount;
		printf("The %s setting %s in config.txt is formatted incorrectly.\n", typeName, path);
		// Print out a message that may help the user with formatting.
		switch (type) {
		case intSetting:
		case int64Setting:
			printf("Ensure it is a number with no characters other than '-' and digits.\n");
			break;
		case floatSetting:
			printf("Ensure it is a number with no characters other than '+' or '-', "
			       "'.', 'e' (if using scientific notation), and digits.\n");
			break;
		case boolSetting:
			printf("Ensure it is either true or false.\n");
			break;
		case stringSetting:
			printf("Ensure it is surrounded by \" characters.\n");
		}
	}
}

/*-------------------------------------------------------------------
 * Function: validateIntSettingMin
 *
 * For a given int setting in the config file, check that it is
 * greater than or equal to minValue. If not, inform the user of the
 * error and increment the count parameter.
 -------------------------------------------------------------------*/
void validateIntSettingMin(const char *path, int minValue, int *errorCount) {
	int value;
	// If path isn't an int setting, there's nothing to check, as that is
	// handled elsewhere.
	if (config_lookup_int(config, path, &value) == CONFIG_TRUE && value < minValue) {
		++*errorCount;
		printf("The integer setting %s's value %d is less than the minimum of %d.\n",
		       path, value, minValue);
	}
}

/*-------------------------------------------------------------------
 * Function: validateIntSettingMax
 *
 * For a given int setting in the config file, check that it is
 * less than or equal to maxValue. If not, inform the user of the
 * error and increment the count parameter.
 -------------------------------------------------------------------*/
void validateIntSettingMax(const char *path, int maxValue, int *errorCount) {
	int value;
	// If path isn't an int setting, there's nothing to check, as that is
	// handled elsewhere.
	if (config_lookup_int(config, path, &value) == CONFIG_TRUE && value > maxValue) {
		++*errorCount;
		printf("The integer setting %s's value %d is more than the maximum of %d.\n",
			path, value, maxValue);
	}
}

/*-------------------------------------------------------------------
 * Function: validateConfig
 *
 * For each setting we expect in the config file, check that it is
 * there and that it has a valid value. If there are any errors,
 * inform the user and terminate the program.
 -------------------------------------------------------------------*/
void validateConfig() {
	int errors = 0;
	validateSetting("logLevel", intSetting, &errors);
	validateIntSettingMin("logLevel", 0, &errors);
	validateIntSettingMax("logLevel", 7, &errors);

	validateSetting("branchLogInterval", intSetting, &errors);
	validateIntSettingMin("branchLogInterval", 0, &errors);

	validateSetting("workerCount", intSetting, &errors);
	validateIntSettingMin("workerCount", 1, &errors);

	validateSetting("Username", stringSetting, &errors);
	// Warn the user (but do not count it as an error) if they haven't changed
	// their username from the default.
	if (strncmp(getConfigStr("Username"), "DefaultUser", 19) == 0)
		printf("Warning: You haven't set your username in config.txt. "
		       "You will not be identifiable on the leaderboards.\n");

	validateSetting("selectionMethod", intSetting, &errors);
	validateIntSettingMin("selectionMethod", InOrder, &errors);
	validateIntSettingMax("selectionMethod", Manual, &errors);

	if (errors) {
		printf("Press ENTER to exit.\n");
		awaitKeyFromUser();
		exit(1);
	}
}

const char *getConfigStr(char *str) {
	const char *temp = NULL;
	config_lookup_string(config, str, &temp);
	return temp;
}

int getConfigInt(char* str) {
	int temp = 0;
	config_lookup_int(config, str, &temp);
	return temp;
}

void shutdownConfig() {
	config_destroy(config);
	free(config);
}
