#ifndef __PATHS_H_
#define __PATHS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_PATH_SIZE 200

typedef struct paths
{
    int num_paths;
    char** paths;
} paths;

char *give_entry_path(char *dir_path, char *entry_name);
paths* return_dir(char *command, int flag);

#endif