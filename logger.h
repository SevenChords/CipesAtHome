#ifndef LOGGER_H
#define LOGGER_H
int init_level_cfg();
int recipeLog(int level, char *process, char *subProcess, char *activity, char *entry);
#endif
