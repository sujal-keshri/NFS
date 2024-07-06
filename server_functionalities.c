#include "server_functionalities.h"

// nm interaction
int make_directory(char *directory_name)
{
    // char *dirname = "new_directory_sujal"; // Name of the directory to be created

    // Creating a directory
    if (mkdir(directory_name, 0755) == 0)
    {
        // printf("Directory created successfully.\n");
        return 0;
    }
    else
    {
        // printf("Failed to create directory.\n");
        return 1;
    }
    // return 0;
}
int zip(char *folderPath, char *zipName)
{
    char zipCommand[256];
    snprintf(zipCommand, sizeof(zipCommand), "cd %s && zip -rq ../%s $(ls | grep -v '^R')", folderPath, zipName);

    int status = system(zipCommand);

    // if (status == -1) {
    //     printf("Failed to execute the zip command.\n");
    // } else {
    //     printf("Folder contents zipped successfully!\n");
    // }

    return 0;
}

char *getFinalName(char *path)
{
    char *finalName = path;
    char *ptr = path;

    while (*ptr != '\0')
    {
        if (*ptr == '/' || *ptr == '\\')
        {
            finalName = ptr + 1; // Move to the next character after the separator
        }
        ptr++;
    }

    return finalName;
}
char *zip_curr_dir()
{
    char cwd[256];
    char* zipName=(char*)malloc(sizeof(char)*512);

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        char path[512];
        strcpy(path, cwd);
        char *pathx = getFinalName(path);
        snprintf(zipName, sizeof(zipName), "%s.zip", pathx);

        char zipCommand[512];
        snprintf(zipCommand, sizeof(zipCommand), "zip -r %s $(ls | grep -v '^R') -x */\\.* > /dev/null 2>&1", zipName);

        int status = system(zipCommand);
    }
    else
    {
        perror("getcwd() error");
        // return EXIT_FAILURE;
    }

    return zipName;
}

int unzip(char *zip_file, char *extract_dir)
{
    char command[100];
    snprintf(command, sizeof(command), "unzip -qq %s -d %s > /dev/null 2>&1", zip_file, extract_dir);

    int result = system(command);

    if (result == 0)
    {
        printf("Zip file '%s' contents extracted to '%s'\n", zip_file, extract_dir);
    }
    else
    {
        printf("Failed to extract file contents\n");
    }

    return 0;
}

int make_file(char *file_name)
{
    // char *fileName = "new_file.txt";
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // File permissions (e.g., read/write for user, read for group and others)

    // Open or create a file using open()
    int file_descriptor = open(file_name, O_CREAT | O_RDWR, mode);

    // Checking if the file was created/opened successfully
    if (file_descriptor != -1)
    {
        // Close the file descriptor after creating the file
        close(file_descriptor);
        // printf("File created successfully.\n");
        return 0;
    }
    else
    {
        // printf("File creation failed.\n");
        return 1;
    }

    return 0;
}

int delete_directory(char *directory_name)
{
    // char *dirName = "directory_to_delete";

    // Attempt to remove the directory
    if (rmdir(directory_name) == 0)
    {
        // printf("Directory deleted successfully.\n");
        return 0;
    }
    else
    {
        printf("Failed to delete the directory.\n");
        return 1;
    }

    return 0;
}

int delete_file(char *file_name)
{
    // char *fileName = "file_to_delete.txt";

    // Attempt to delete the file
    if (unlink(file_name) == 0)
    {
        // printf("File deleted successfully.\n");
        return 0;
    }
    else
    {
        // printf("Failed to delete the file.\n");
        return 1;
    }

    return 0;
}

// client interaction
int sc_read(int client_socket, char *file_name)
{
    FILE *filePtr;
    char buffer[BUFFER_SIZE];

    // Open the file for reading
    filePtr = fopen(file_name, "rb");
    if (filePtr == NULL)
    {
        perror("File opening failed");
        return -1;
    }

    // Read file data and send to client
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, filePtr)) > 0)
    {
        send(client_socket, buffer, bytesRead, 0);
    }

    // Close file and socket
    fclose(filePtr);
    close(client_socket);
    // close(server_socket);

    printf("File sent successfully.\n");
    return 0;
}

int sc_write(int client_socket, char *file_name)
{
    FILE *filePtr;
    // Open the file to write received data
    filePtr = fopen(file_name, "wb");
    if (filePtr == NULL)
    {
        perror("File opening failed");
        return -1;
    }

    ssize_t bytesRead;
    char buffer[BUFFER_SIZE];
    bzero(buffer, 1024);
    // Receive file data and write to the file
    while ((bytesRead = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        fwrite(buffer, 1, bytesRead, filePtr);
    }

    // Close the file and socket
    fclose(filePtr);
    close(client_socket);

    printf("File received successfully.\n");
    return 0;
}

int sc_permission(int client_socket, char *file_name)
{
    struct stat fileStat;
    char fileInfo[BUFFER_SIZE];

    // Get file information
    if (stat(file_name, &fileStat) < 0)
    {
        perror("File stat retrieval failed");
        return -1;
    }

    // Prepare file information string
    sprintf(fileInfo, "File size: %ld bytes, Permissions: %o", fileStat.st_size, fileStat.st_mode & 0777);

    // Send file information to client
    send(client_socket, fileInfo, strlen(fileInfo), 0);

    // Close file and socket
    close(client_socket);

    printf("File sent successfully.\n");
    return 0;
}

// int main()
// {
//     // if(make_directory("n_dir"))
//     //     printf("Directory created successfully.\n");
//     // else
//     //     printf("Failed to create directory.\n");

//     // make_file("b.txt");
//     // sleep(5);
//     // delete_file("b.txt");
//     // make_directory("a");
//     // sleep(5);
//     // delete_directory("a");

//     // char* content_str=read_file("a.txt");
//     // printf("%d\n", strlen(content_str));
//     // return 0;

//     sc_try_permission(231, "a.txt");
// }

// int main()
// {
//     char *ip = "127.0.0.1";
//     int port = 5566;

//     int server_sock, client_sock;
//     struct sockaddr_in server_addr, client_addr;
//     char buffer[1024];
//     int n;

//     server_sock = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_sock < 0)
//     {
//         perror("[-]Socket error");
//         exit(1);
//     }
//     printf("[+]TCP server socket created.\n");

//     memset(&server_addr, '\0', sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = port;
//     server_addr.sin_addr.s_addr = inet_addr(ip);

//     n = bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
//     if (n < 0)
//     {
//         perror("[-]Bind error");
//         exit(1);
//     }
//     printf("[+]Bind to the port number: %d\n", port);

//     listen(server_sock, 5);
//     printf("Listening...\n");

//     server_client_arg sc_arg;
//     sc_arg.server_client_sock = server_sock;
//     sc_arg.server_client_addr = server_addr;
//     strcpy(sc_arg.file_name, "a.txt");
//     pthread_t client_send;
//     if (pthread_create(&client_send, NULL, client_send_read, (void *)&sc_arg) != 0)
//     {
//         perror("thread create");
//         exit(EXIT_FAILURE);
//     }

//     if (pthread_join(client_send, NULL) != 0)
//     {
//         perror("pthread join");
//         exit(EXIT_FAILURE);
//     }
//     return 0;
// }