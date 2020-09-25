CC=gcc
CFLAGS=-lcurl -lconfig -fopenmp -Wall
DEPS = start.c inventory.c recipes.c config.c FTPManagement.c cJSON.c calculator.c logger.c

recipesmake: start.c
	$(CC) -o start $(DEPS) $(CFLAGS)
