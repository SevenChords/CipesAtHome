inventorymake: inventory.c
	gcc -o inventory inventory.c -g -I.

ftpmake: FTPManagement.c
	gcc FTPManagement.c cJSON.c -lcurl -o FTPManagement -g

logmake: logger.c
	gcc -o logger logger.c -g

configmake: config.c
	gcc -o config config.c -lconfig -g

recipemake: recipes.c
	gcc -o recipes recipes.c inventory.c -g -Wall
