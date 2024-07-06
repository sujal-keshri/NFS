#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5577
#define BUFFER_SIZE 1024
#define SIZEOFNAME 128
#define MAX_TEXT_LENGTH 10240
#define MAX_PATH_LENGTH 500
#define TIMEOUT_SECS 10
typedef struct response
{
    int responsevalue; // 1 : Success , 0 : Failure , 2 : FileNotFound , 3 : FileFound , 4 : Being Written Onto
} Response;
typedef struct input
{
    char oper[50];
    char path1[SIZEOFNAME];
    char path2[SIZEOFNAME];
    int FileorDirec;
} request;

typedef struct ss_connection_info
{
    char ip[50];
    int ss_port;
    int r;
} ss_connection_info;

char *getFileName(char *path) 
{
    char *fileName = path + strlen(path);

    while (fileName > path && *fileName != '/') {
        fileName--;
    }

    if (*fileName == '/') {
        fileName++;
    }

    return fileName;
}

int read_from_ss(request command, int client_socket)
{
    // const char *message = "Hello, server!";
    send(client_socket, &command, sizeof(command), 0);

    FILE *filePtr;
    filePtr = fopen("received_file.txt", "wb");
    if (filePtr == NULL)
    {
        perror("File opening failed");
        return -1;
    }

    ssize_t bytesRead;
    char buffer[BUFFER_SIZE];
    while ((bytesRead = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        fwrite(buffer, 1, bytesRead, filePtr);
    }
    fclose(filePtr);
    // close(client_socket);

    printf("File reading successful.\n");
    return 0;

    // close(client_socket);
}

int write_from_file(request command, int client_socket, char *file_path)
{
    send(client_socket, &command, BUFFER_SIZE, 0);

    FILE *filePtr;
    char buffer[BUFFER_SIZE];

    filePtr = fopen(file_path, "rb");
    if (filePtr == NULL)
    {
        perror("File opening failed");
        return -1;
    }
    bzero(buffer, 1024);
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, filePtr)) > 0)
    {
        // printf("%s", buffer);
        send(client_socket, buffer, bytesRead, 0);
    }
    fclose(filePtr);
    // close(clientSocket);

    printf("data from file sent successfully.\n");
    return 0;
}

int write_text(request command, int client_socket, char *text)
{
    send(client_socket, &command, BUFFER_SIZE, 0);

    send(client_socket, text, strlen(text), 0);

    printf("text data sent successfully.\n");
    return 0;
}

int get_from_ss(request command, int client_socket)
{
    // const char *message = "Hello, server!";
    send(client_socket, &command, BUFFER_SIZE, 0);
    ssize_t bytesRead;
    char buffer[BUFFER_SIZE];
    bytesRead = recv(client_socket, buffer, BUFFER_SIZE, 0);
    // fwrite(buffer, 1, bytesRead, filePtr);
    printf("%s\n", buffer);

    // close(client_socket);

    printf("File reading successful.\n");
    return 0;

    // close(client_socket);
}

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    struct input input;
    char acknowledgment[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket Assigned : %d\n", sock);
    while (1)
    {
        printf("enter the operation: ");
        scanf("%s", input.oper);

        if (strcmp(input.oper, "copy") == 0)
        {
            printf("enter the source path: ");
            scanf("%s", input.path1);
            printf("enter the destination path: ");
            scanf("%s", input.path2);
        }
        else if (strcmp(input.oper, "create") == 0)
        {
            printf("Enter File[1] or Directory[2] : ");
            scanf("%d", &input.FileorDirec);
            printf("enter the path : ");
            scanf("%s", input.path1);
            printf("enter the name of the file : ");
            scanf("%s", input.path2);
        }
        else if (strcmp(input.oper, "delete") == 0)
        {
            printf("Enter File[1] or Directory[2] : ");
            scanf("%d", &input.FileorDirec);
            printf("enter the path : ");
            scanf("%s", input.path1);
        }
        else
        {
            printf("enter the path : ");
            scanf("%s", input.path1);
        }

        if (send(sock, &input, sizeof(input), 0) == -1)
        {
            perror("Error in sending struct");
            exit(1);
        }
        printf("Struct sent to server.\n");
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SECS;
        timeout.tv_usec = 0;
        int result = select(sock + 1, &readSet, NULL, NULL, &timeout);
        if (result == -1)
        {
            perror("Select error");
            exit(EXIT_FAILURE);
        }
        else if (result == 0)
        {
            printf("No response received within the %d seconds\n\n", TIMEOUT_SECS);
            continue;
        }
        Response Resp;
        if (recv(sock, &Resp, sizeof(Resp), 0) == -1)
        {
            perror("Recieveing Response Failed");
            exit(1);
        }
        if (Resp.responsevalue == 3)
        {
            if (strcmp(input.oper, "read") == 0 || strcmp(input.oper, "write") == 0 || strcmp(input.oper, "get") == 0)
            {
                ss_connection_info info;
                if (recv(sock, &info, sizeof(info), 0) == -1)
                {
                    perror("recv error");
                    exit(1);
                }
                printf("ss ip: %s\n", info.ip);
                printf("ss port: %d\n", info.ss_port);
                if(info.r==1)
                {
                    char temp[MAX_PATH_LENGTH];
                    // snprintf(temp, )
                    strcpy(temp, "R1/");
                    strcat(temp, input.path1);
                    strcpy(input.path1, temp);
                }
                else if(info.r==2)
                {
                    char temp[MAX_PATH_LENGTH];
                    // snprintf(temp, )
                    strcpy(temp, "R2/");
                    strcat(temp, input.path1);
                    strcpy(input.path1, temp);
                }
                
                int client_socket = socket(AF_INET, SOCK_STREAM, 0);
                if (client_socket == -1)
                {
                    perror("Error creating socket");
                    exit(EXIT_FAILURE);
                }

                struct sockaddr_in ss_address;
                ss_address.sin_family = AF_INET;
                ss_address.sin_port = htons(info.ss_port);
                printf("%s\n", info.ip);
                if (inet_pton(AF_INET, info.ip, &ss_address.sin_addr) <= 0)
                {
                    perror("Invalid address/ Address not supported");
                    exit(EXIT_FAILURE);
                }

                if (connect(client_socket, (struct sockaddr *)&ss_address, sizeof(ss_address)) == -1)
                {
                    perror("Connection failed");
                    exit(EXIT_FAILURE);
                }

                printf("Connected to the storage server.\n");
                int value;
                if (strcmp(input.oper, "read") == 0)
                {
                    value = read_from_ss(input, client_socket);
                }
                else if (strcmp(input.oper, "write") == 0)
                {
                    int option;
                    printf("enter 1 to give text or enter 2 to write from a file: ");
                    scanf("%d", &option);
                    if (option == 1)
                    {
                        char text[MAX_TEXT_LENGTH];
                        printf("enter the text: ");
                        getchar();
                        scanf("%[^\n]s", text);
                        printf("%s\n", text);
                        value = write_text(input, client_socket, text);
                    }
                    else
                    {
                        printf("enter the file path from which data need to be written: ");
                        char file_path[MAX_PATH_LENGTH];
                        getchar();
                        scanf("%[^\n]s", file_path);
                        value = write_from_file(input, client_socket, file_path);
                    }
                }
                else
                {
                    value = get_from_ss(input, client_socket);
                }
                Response Resp2;
                if (value == 0)
                {
                    Resp2.responsevalue = 1;
                }
                else
                {
                    Resp2.responsevalue = 0;
                }
                if (send(sock, &Resp2, sizeof(Resp2), 0) == -1)
                {
                    perror("Sending Response Failed To NM Failed");
                    exit(1);
                }
                // const char *message = "Hello, server!";
                // send(client_socket, message, strlen(message), 0);

                // char buffer[1024];
                // memset(buffer, 0, sizeof(buffer));
                // recv(client_socket, buffer, sizeof(buffer), 0);
                // printf("Server response: %s\n", buffer);

                close(client_socket);
            }
            else if (strcmp(input.oper, "delete") == 0 || strcmp(input.oper, "create") == 0)
            {
                if (recv(sock, &Resp, sizeof(Resp), 0) == -1)
                {
                    perror("Recieving Response from Naming Server Failed");
                    exit(1);
                }
                printf("Response Recieved : %d\n", Resp.responsevalue);
                if (strcmp(input.oper, "delete") == 0)
                {
                    printf("Deletion of file");
                }
                else
                {
                    printf("Creation of file");
                }
                if (Resp.responsevalue == 0)
                {
                    printf(" Successful\n");
                }
                else if (Resp.responsevalue == 1)
                {
                    printf(" Unsuccessful\n");
                }
            }
        }
        else if (Resp.responsevalue == 2)
        {
            printf("Requested File Not Found\n");
        }
        else if (Resp.responsevalue == 4)
        {
            printf("Requested File is Already being Written Onto\n");
        }
        printf("\n");
    }
    close(sock);

    return 0;
}