#include "paths.h"

char *give_entry_path(char *dir_path, char *entry_name)
{
    char *entry_path = (char *)malloc(sizeof(char) * 1000);
    strcpy(entry_path, dir_path);
    strcat(entry_path, "/");
    strcat(entry_path, entry_name);
    return entry_path;
}

paths* return_dir(char *command, int flag)
{
    paths* my_paths=(paths*)malloc(sizeof(paths));
    struct dirent **entry_list;
    int num_entry = scandir(command, &entry_list, NULL, alphasort); // alphasort
    if (num_entry < 0)
    {
        perror("Error");
    }
    my_paths->num_paths=num_entry;
    my_paths->paths=(char**)malloc(sizeof(char*)*num_entry);
    for(int i=0; i<num_entry; i++)
        my_paths->paths[i]=(char*)malloc(sizeof(char)*MAX_PATH_SIZE);

    int count=0;
    for (int i = 0; i < num_entry; i++)
    {
        if (flag == 1 || (flag == 0 && entry_list[i]->d_name[0] != '.'))
        {
            // count++;
            char *entry_path = give_entry_path(command, entry_list[i]->d_name);
            strcpy(my_paths->paths[count++], entry_list[i]->d_name);
            // printf("%s\n", entry_list[i]->d_name);
            free(entry_path);
        }
        free(entry_list[i]);
    }
    free(entry_list);
    my_paths->num_paths=count;
    my_paths->paths=realloc(my_paths->paths, sizeof(char*)*count);
    return my_paths;
}

// int main()
// {
//     char home[50]="a";
//     char pre_dir[50]="b";
//     char com[50]="../..";
//     paths* my_path=return_dir(com, 1);
//     printf("%d\n", my_path->num_paths);
//     for(int i=0; i<my_path->num_paths; i++)
//     {
//         // if(strcmp(my_path->paths[i], "-----")!=0)
//             printf("%s\n", my_path->paths[i]);
//     }
// }