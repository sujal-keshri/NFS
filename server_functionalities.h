#ifndef __SERVER_FUNCTIONALITIES_H_
#define __SERVER_FUNCTIONALITIES_H_

#define CONTENT_SIZE 1024
#define BUFFER_SIZE 1024
#define PORT_READ 8888

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
// #include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef struct server_client_arg
{
    int server_client_sock;
    struct sockaddr_in server_client_addr;
    char file_name[100];
} server_client_arg;

int make_directory(char* directory_name);
int make_file(char* file_name);
int delete_directory(char* directory_name);
int delete_file(char* file_name);
int sc_read(int client_socket, char* file_name);
int sc_write(int client_socket, char *file_name);
int sc_permission(int client_socket, char *file_name);
int zip_file_dir(char* folder_to_zip, char* zip_file);
int zip_dir_content(char *folder_to_zip, char *zip_file);
int zip(char *folderPath, char *zipName);
char *getFinalName(char *path);
char* zip_curr_dir();
int unzip(char* zip_file, char* extract_dir);

#endif