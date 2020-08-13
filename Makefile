inventorymake: inventory.c
	gcc -o inventory inventory.c -g -I.

ftpmake: FTPManagement.c
	gcc FTPManagement.c cJSON.c -lcurl -o FTPManagement -g
