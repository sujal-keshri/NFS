#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_AX_PATHS 100
#define PATH_LENGTH 1024
#define MAX_SS_SERVERS 100
#define SIZEOFNAME 128
#define ALPHABET_SIZE (30)
#define NMSSACCEPTPORT 5566
#define NMCLACCEPTPORT 5577
#define INITSSSERVERS 3
#define MAX_CACHE_SIZE 10

FILE *LogFilePtr;
typedef struct FileEntry
{
    char path[SIZEOFNAME];
    int ServerIndex;
    pthread_rwlock_t *lock;
    struct FileEntry *next;
} FileEntry;

typedef struct LRUCache
{
    FileEntry *head;
    int size;
    int capacity;
} LRUCache;

LRUCache *initCache()
{
    LRUCache *cache = (LRUCache *)malloc(sizeof(LRUCache));
    cache->head = NULL;
    cache->size = 0;
    cache->capacity = MAX_CACHE_SIZE;
    return cache;
}

// Function to add a new entry to the cache
void addFileEntry(LRUCache *cache, const char *path, int server, pthread_rwlock_t *lock)
{
    // Check if the cache is full
    if (cache->size >= cache->capacity)
    {
        // Remove the least recently used entry
        FileEntry *current = cache->head;
        FileEntry *prev = NULL;
        while (current->next != NULL)
        {
            prev = current;
            current = current->next;
        }
        if (prev != NULL)
        {
            prev->next = NULL;
            free(current);
            cache->size--;
        }
    }
    // Add the new entry to the front of the list
    FileEntry *newEntry = (FileEntry *)malloc(sizeof(FileEntry));
    strncpy(newEntry->path, path, sizeof(newEntry->path) - 1);
    newEntry->ServerIndex = server;
    newEntry->lock = lock;
    newEntry->next = cache->head;
    cache->head = newEntry;
    cache->size++;
}

// Function to access a file in the cache
FileEntry *accessFile(LRUCache *cache, const char *path)
{
    FileEntry *prev = NULL;
    FileEntry *current = cache->head;

    // Traverse the cache to find the requested file
    while (current != NULL && strcmp(current->path, path) != 0)
    {
        prev = current;
        current = current->next;
    }

    // If the file is found, move it to the front of the cache (mark it as most recently used)
    if (current != NULL)
    {
        if (prev != NULL)
        {
            prev->next = current->next;
            current->next = cache->head;
            cache->head = current;
        }
        return current;
    }

    return NULL; // File not found in the cache
}
void printCache(const LRUCache *cache)
{
    FileEntry *current = cache->head;
    while (current != NULL)
    {
        printf("Path: %s, Server: %d\n", current->path, current->ServerIndex);
        current = current->next;
    }
    printf("\n");
}
LRUCache *CacheMemory;
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

typedef struct StorageServerArg
{
    int ServerSock;
    struct sockaddr_in ServerAddress;
} StorageServerArg;

typedef struct ClientWorksArg
{
    int ClientSock;
    char ip[15];
    int port;
} ClientWorksArg;

typedef struct storage_server_info
{
    char ip[15];
    int NMSSport;
    int SSCLport;
    int sizeoffile_paths;
    char file_paths[MAX_AX_PATHS][PATH_LENGTH];
} storage_server_info;

storage_server_info ServerInfos[MAX_SS_SERVERS];
struct TrieNode *ServerAccessiblePaths;

int ServerIndex = 0;

/* Tries Code Start */
struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
    int NoofChildren;
    char Word[SIZEOFNAME];
    bool isEndOfWord;
    int SSIndex;
    pthread_rwlock_t ReadersWritersLock;
};

struct TrieNode *getNode(void)
{
    struct TrieNode *pNode = NULL;

    pNode = (struct TrieNode *)malloc(sizeof(struct TrieNode));

    if (pNode)
    {
        int i;

        pNode->isEndOfWord = false;
        pNode->NoofChildren = 0;
        for (i = 0; i < ALPHABET_SIZE; i++)
            pNode->children[i] = NULL;
    }
    pNode->isEndOfWord = false;
    pNode->SSIndex = -1;
    pthread_rwlock_init(&pNode->ReadersWritersLock, NULL);
    return pNode;
}

int zip(char *folderPath, char *zipName)
{
    char zipCommand[256];
    snprintf(zipCommand, sizeof(zipCommand), "cd %s && zip -rq ../%s $(ls | grep -v '^SS')", folderPath, zipName);

    int status = system(zipCommand);

    // if (status == -1) {
    //     printf("Failed to execute the zip command.\n");
    // } else {
    //     printf("Folder contents zipped successfully!\n");
    // }

    return 0;
}

int CheckIfChildExists(struct TrieNode *root, const char *path)
{
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (root->children[i] == NULL)
        {
            return -1;
        }
        else
        {
            if (!strcmp(root->children[i]->Word, path))
            {
                return i;
            }
        }
    }
    return -1;
}

void insert(struct TrieNode *root, const char *path, int SSIndex)
{
    struct TrieNode *pCrawl = root;

    char *PETH = strdup(path);

    char *Token = strtok(PETH, "/");
    while (Token != NULL)
    {
        int value = CheckIfChildExists(pCrawl, Token);
        if (value == -1)
        {
            int index = pCrawl->NoofChildren;
            pCrawl->children[index] = getNode();
            strcpy(pCrawl->children[index]->Word, Token);
            pCrawl->NoofChildren++;
            pCrawl = pCrawl->children[index];
        }
        else
        {
            pCrawl = pCrawl->children[value];
        }
        Token = strtok(NULL, "/");
    }
    pCrawl->isEndOfWord = true;
    pCrawl->SSIndex = SSIndex;
    free(PETH);
    return;
}

void delete(struct TrieNode *root, const char *path)
{
    struct TrieNode *pCrawl = root;
    char *PETH = strdup(path);
    char *Token = strtok(PETH, "/");
    while (Token != NULL)
    {
        int value = CheckIfChildExists(pCrawl, Token);
        if (value == -1)
        {
            free(PETH);
            return;
        }
        else
        {
            pCrawl = pCrawl->children[value];
        }
        Token = strtok(NULL, "/");
    }
    pCrawl->isEndOfWord = false;
    pCrawl->SSIndex = -1;
    free(PETH);
    return;
}

int search(struct TrieNode *root, const char *path, pthread_rwlock_t **ReadersWritersLock)
{
    FileEntry *Entrie = accessFile(CacheMemory, path);
    if (Entrie != NULL)
    {
        *ReadersWritersLock = Entrie->lock;
        return Entrie->ServerIndex;
    }
    struct TrieNode *pCrawl = root;
    char *PETH = strdup(path);

    char *Token = strtok(PETH, "/");
    while (Token != NULL)
    {
        int value = CheckIfChildExists(pCrawl, Token);
        if (value == -1)
        {
            return -1;
        }
        else
        {
            pCrawl = pCrawl->children[value];
        }
        Token = strtok(NULL, "/");
    }
    if (pCrawl->isEndOfWord)
    {
        addFileEntry(CacheMemory, path, pCrawl->SSIndex, &pCrawl->ReadersWritersLock);
        *ReadersWritersLock = &pCrawl->ReadersWritersLock;
        return pCrawl->SSIndex;
    }
    else
    {
        addFileEntry(CacheMemory, path, -1, NULL);
        return -1;
    }
}
int search2(struct TrieNode *root, const char *path)
{
    struct TrieNode *pCrawl = root;

    char *PETH = strdup(path);

    char *Token = strtok(PETH, "/");
    while (Token != NULL)
    {
        int value = CheckIfChildExists(pCrawl, Token);
        if (value == -1)
        {
            return -1;
        }
        else
        {
            pCrawl = pCrawl->children[value];
        }
        Token = strtok(NULL, "/");
    }
    if (pCrawl->isEndOfWord)
    {
        return pCrawl->SSIndex;
    }
    return -1;
}
/* Tries Code End */

void *StorageServerAcceptFunc()
{
    char *ip = "127.0.0.1";
    int port = NMSSACCEPTPORT;

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];
    int n;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[-]Bind error");
        exit(1);
    }
    listen(server_sock, 5);
    printf("Listening...\n");
    ServerAccessiblePaths = getNode();
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0)
        {
            perror("Accept Failed");
            exit(1);
        }
        if (recv(client_sock, &ServerInfos[ServerIndex], sizeof(ServerInfos[ServerIndex]), 0) == -1)
        {
            perror("recv failed");
            exit(1);
        }
        for (int i = 0; i < ServerInfos[ServerIndex].sizeoffile_paths; i++)
        {
            printf("%s\n", ServerInfos[ServerIndex].file_paths[i]);
        }
        for (int i = 0; i < ServerInfos[ServerIndex].sizeoffile_paths; i++)
        {
            insert(ServerAccessiblePaths, ServerInfos[ServerIndex].file_paths[i], ServerIndex);
        }
        fprintf(LogFilePtr, "Storage Server(%s) has successfully sent its Accessible Paths through port : %d\n", ServerInfos[ServerIndex].ip, NMSSACCEPTPORT);
        fflush(LogFilePtr);
        ServerIndex++;
        if (ServerIndex == 3)
        {
            int rss_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (rss_socket == -1)
            {
                perror("Error creating socket");
                exit(EXIT_FAILURE);
            }
            struct sockaddr_in rss_address;
            rss_address.sin_family = AF_INET;
            rss_address.sin_port = htons(ServerInfos[0].NMSSport);
            if (inet_pton(AF_INET, ServerInfos[0].ip, &rss_address.sin_addr) <= 0)
            {
                perror("Invalid address/ Address not supported");
                exit(EXIT_FAILURE);
            }
            if (connect(rss_socket, (struct sockaddr *)&rss_address, sizeof(rss_address)) == -1)
            {
                perror("Connection failed");
                exit(EXIT_FAILURE);
            }
            request Request;
            strcpy(Request.oper, "replicate");
            Request.FileorDirec = ServerInfos[1].SSCLport;
            strcpy(Request.path1, ServerInfos[1].ip);
            if (send(rss_socket, &Request, sizeof(Request), 0) == -1)
            {
                perror("Sending Failed");
                exit(1);
            }
            close(rss_socket);

            rss_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (rss_socket == -1)
            {
                perror("Error creating socket");
                exit(EXIT_FAILURE);
            }

            struct sockaddr_in rss2_address;
            rss2_address.sin_family = AF_INET;
            rss2_address.sin_port = htons(ServerInfos[1].NMSSport);
            if (inet_pton(AF_INET, ServerInfos[1].ip, &rss2_address.sin_addr) <= 0)
            {
                perror("Invalid address/ Address not supported");
                exit(EXIT_FAILURE);
            }
            if (connect(rss_socket, (struct sockaddr *)&rss2_address, sizeof(rss2_address)) == -1)
            {
                perror("Connection failed");
                exit(EXIT_FAILURE);
            }
            request Request2;
            strcpy(Request2.oper, "replicate");
            Request2.FileorDirec = ServerInfos[2].SSCLport;
            strcpy(Request2.path1, ServerInfos[2].ip);
            if (send(rss_socket, &Request2, sizeof(Request2), 0) == -1)
            {
                perror("Sending Failed");
                exit(1);
            }
            close(rss_socket);

            // if (recv(rss_socket, &Resp, sizeof(Resp), 0) == -1)
            // {
            //     perror("Recieving Response Failed");
            //     exit(1);
            // }
        }
        else if (ServerIndex > 3)
        {
            int rss_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (rss_socket == -1)
            {
                perror("Error creating socket");
                exit(EXIT_FAILURE);
            }
            struct sockaddr_in rss_address;
            rss_address.sin_family = AF_INET;
            rss_address.sin_port = htons(ServerInfos[ServerIndex - 2].NMSSport);
            if (inet_pton(AF_INET, ServerInfos[ServerIndex - 2].ip, &rss_address.sin_addr) <= 0)
            {
                perror("Invalid address/ Address not supported");
                exit(EXIT_FAILURE);
            }
            if (connect(rss_socket, (struct sockaddr *)&rss_address, sizeof(rss_address)) == -1)
            {
                perror("Connection failed");
                exit(EXIT_FAILURE);
            }
            request Request;
            strcpy(Request.oper, "replicate");
            Request.FileorDirec = ServerInfos[ServerIndex - 1].SSCLport;
            strcpy(Request.path1, ServerInfos[ServerIndex - 1].ip);
            if (send(rss_socket, &Request, sizeof(Request), 0) == -1)
            {
                perror("Sending Failed");
                exit(1);
            }
            close(rss_socket);
        }
        close(client_sock);
    }
    return NULL;
}

int checkSocket(const char *ipAddress, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress);

    int connectionStatus = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    if (connectionStatus == 0)
    {
        printf("Socket is alive!\n");
        close(sockfd);
        return 1;
    }
    else
    {
        perror("Connection failed");
        close(sockfd);
        return 0;
    }
}

void *ClientWorksFunc(void *arg)
{
    ClientWorksArg *ClientArg = (ClientWorksArg *)arg;
    int ClientSock = ClientArg->ClientSock;
    char ip[15];
    strcpy(ip, ClientArg->ip);
    int port = ClientArg->port;
    struct input Request;
    while (1)
    {
        int bytesRead = 0;
        bytesRead = recv(ClientSock, &Request, sizeof(Request), 0);
        if (bytesRead > 0)
        {
            fprintf(LogFilePtr, "Recieved %s Request from client(%s) through the port %d\n", Request.oper, ip, port);
            fflush(LogFilePtr);
            if (!strcmp(Request.oper, "read") || !strcmp(Request.oper, "write") || !strcmp(Request.oper, "get"))
            {
                pthread_rwlock_t *ReadersWritersLock;
                int value = search(ServerAccessiblePaths, Request.path1, &ReadersWritersLock);
                if (value != -1)
                {
                    if (!strcmp(Request.oper, "read"))
                    {
                        if (pthread_rwlock_tryrdlock(ReadersWritersLock) == 0)
                        {
                            Response Resp;
                            Resp.responsevalue = 3;
                            if (send(ClientSock, &Resp, sizeof(Resp), 0) == -1)
                            {
                                perror("Sending Response to Client Failed");
                                exit(1);
                            }
                        }
                        else
                        {
                            Response Resp;
                            Resp.responsevalue = 4;
                            if (send(ClientSock, &Resp, sizeof(Resp), 0) == -1)
                            {
                                perror("Sending Response to Client Failed");
                                exit(1);
                            }
                            continue;
                        }
                        ss_connection_info SSConnectInfo;
                        strcpy(SSConnectInfo.ip, "127.0.0.1");
                        SSConnectInfo.ss_port = ServerInfos[value].SSCLport;
                        SSConnectInfo.r = 0;

                        int isAlive = checkSocket(SSConnectInfo.ip, SSConnectInfo.ss_port);

                        if (!isAlive)
                        {
                            if (value == 0)
                            {
                                SSConnectInfo.ss_port = ServerInfos[value + 1].SSCLport;
                                SSConnectInfo.r = 1;
                            }
                            else
                            {
                                isAlive = checkSocket(SSConnectInfo.ip, ServerInfos[value - 1].SSCLport);
                                if (isAlive == 1)
                                {
                                    SSConnectInfo.ss_port = ServerInfos[value - 1].SSCLport;
                                    SSConnectInfo.r = 2;
                                }
                                else
                                {
                                    SSConnectInfo.ss_port = ServerInfos[value + 1].SSCLport;
                                    SSConnectInfo.r = 1;
                                }
                            }
                        }

                        if (send(ClientSock, &SSConnectInfo, sizeof(SSConnectInfo), 0) == -1)
                        {
                            perror("Sending Failed");
                            exit(1);
                        }
                        Response Resp2;
                        if (recv(ClientSock, &Resp2, sizeof(Resp2), 0) == -1)
                        {
                            perror("Recieveing Response Failed");
                            exit(1);
                        }
                        // Add to Log File
                        fprintf(LogFilePtr, "Recieved Read Response from the client(%s) through the port %d\n", ip, port);
                        fflush(LogFilePtr);
                        if (Resp2.responsevalue == 1)
                        {
                            printf("Read Operation Successful\n");
                            fprintf(LogFilePtr, "Read operation from %s successful\n", Request.path1);
                            fflush(LogFilePtr);
                        }
                        else
                        {
                            printf("Read Operation Unsuccessful\n");
                            fprintf(LogFilePtr, "Read operation from %s unsuccessful\n", Request.path1);
                            fflush(LogFilePtr);
                        }
                        pthread_rwlock_unlock(ReadersWritersLock);
                        // Recieve Response From Storage Server
                        // Decrement ReadersCount
                    }
                    else if (!strcmp(Request.oper, "write"))
                    {
                        if (pthread_rwlock_trywrlock(ReadersWritersLock) == 0)
                        {
                            Response Resp;
                            Resp.responsevalue = 3;
                            if (send(ClientSock, &Resp, sizeof(Resp), 0) == -1)
                            {
                                perror("Sending Response to Client Failed");
                                exit(1);
                            }
                        }
                        else
                        {
                            Response Resp;
                            Resp.responsevalue = 4;
                            if (send(ClientSock, &Resp, sizeof(Resp), 0) == -1)
                            {
                                perror("Sending Response to Client Failed");
                                exit(1);
                            }
                            continue;
                        }
                        ss_connection_info SSConnectInfo;
                        strcpy(SSConnectInfo.ip, "127.0.0.1");
                        SSConnectInfo.ss_port = ServerInfos[value].SSCLport;
                        if (send(ClientSock, &SSConnectInfo, sizeof(SSConnectInfo), 0) == -1)
                        {
                            perror("Sending Failed");
                            exit(1);
                        }
                        // Recieve Response From Storage Server
                        // Release Lock
                        Response Resp2;
                        if (recv(ClientSock, &Resp2, sizeof(Resp2), 0) == -1)
                        {
                            perror("Recieveing Response Failed");
                            exit(1);
                        }
                        fprintf(LogFilePtr, "Recieved Write Response from the client(%s) through the port %d\n", ip, port);
                        fflush(LogFilePtr);
                        // Add to Log File
                        if (Resp2.responsevalue == 1)
                        {
                            printf("Write Operation Successful\n");
                            fprintf(LogFilePtr, "Write operation to %s successful\n", Request.path1);
                            fflush(LogFilePtr);
                        }
                        else
                        {
                            printf("Write Operation Unsuccessful\n");
                            fprintf(LogFilePtr, "Write operation to %s unsuccessful\n", Request.path1);
                            fflush(LogFilePtr);
                        }
                        pthread_rwlock_unlock(ReadersWritersLock);
                    }
                    else
                    {
                        Response Resp;
                        Resp.responsevalue = 3;
                        if (send(ClientSock, &Resp, sizeof(Resp), 0) == -1)
                        {
                            perror("Sending Response to Client Failed");
                            exit(1);
                        }
                        ss_connection_info SSConnectInfo;
                        strcpy(SSConnectInfo.ip, "127.0.0.1");
                        SSConnectInfo.ss_port = ServerInfos[value].SSCLport;
                        if (send(ClientSock, &SSConnectInfo, sizeof(SSConnectInfo), 0) == -1)
                        {
                            perror("Sending Failed");
                            exit(1);
                        }
                        Response Resp2;
                        if (recv(ClientSock, &Resp2, sizeof(Resp2), 0) == -1)
                        {
                            perror("Recieveing Response Failed");
                            exit(1);
                        }
                        fprintf(LogFilePtr, "Recieved  Get Response from the client(%s) through the port %d\n", ip, port);
                        fflush(LogFilePtr);
                        // Add to Log File
                        if (Resp2.responsevalue == 1)
                        {
                            printf("Get Operation Successful\n");
                            fprintf(LogFilePtr, "Get operation from %s successful\n", Request.path1);
                            fflush(LogFilePtr);
                        }
                        else
                        {
                            printf("Get Operation Unsuccessful\n");
                            fprintf(LogFilePtr, "Get operation from %s unsuccessful\n", Request.path1);
                            fflush(LogFilePtr);
                        }
                    }
                }
                else
                {
                    Response Resp;
                    Resp.responsevalue = 2;
                    if (send(ClientSock, &Resp, sizeof(Resp), 0) == -1)
                    {
                        perror("Sending Response to Client Failed");
                        exit(1);
                    }
                    fprintf(LogFilePtr, "Operation %s from client(%s) from port(%d) unsuccessful because No valid %s found\n", Request.oper, ip, port, Request.path1);
                    fflush(LogFilePtr);
                    // Add to Log File
                }
                // close(ClientSock);
                // return NULL;
            }
            else if (!strcmp(Request.oper, "create") || !strcmp(Request.oper, "delete"))
            {
                int value = search2(ServerAccessiblePaths, Request.path1); // See search
                if (value != -1)
                {
                    Response Resp;
                    Resp.responsevalue = 3;
                    if (send(ClientSock, &Resp, sizeof(Resp), 0) == -1)
                    {
                        perror("Sending Response to Client Failed");
                        exit(1);
                    }
                    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    if (client_socket == -1)
                    {
                        perror("Error creating socket");
                        exit(EXIT_FAILURE);
                    }
                    struct sockaddr_in ss_address;
                    ss_address.sin_family = AF_INET;
                    ss_address.sin_port = htons(ServerInfos[value].NMSSport);
                    if (inet_pton(AF_INET, ServerInfos[value].ip, &ss_address.sin_addr) <= 0)
                    {
                        perror("Invalid address/ Address not supported");
                        exit(EXIT_FAILURE);
                    }
                    if (connect(client_socket, (struct sockaddr *)&ss_address, sizeof(ss_address)) == -1)
                    {
                        perror("Connection failed");
                        exit(EXIT_FAILURE);
                    }
                    if (send(client_socket, &Request, sizeof(Request), 0) == -1)
                    {
                        perror("Sending Failed");
                        exit(1);
                    }
                    if (recv(client_socket, &Resp, sizeof(Resp), 0) == -1)
                    {
                        perror("Recieving Response Failed");
                        exit(1);
                    }
                    fprintf(LogFilePtr, "Recieved %s Response from the Storage Server(%s) through the port %d\n", Request.oper, ServerInfos[value].ip, ServerInfos[value].NMSSport);
                    fflush(LogFilePtr);
                    // Add to Log File
                    if (Resp.responsevalue == 0)
                    {
                        if (strcmp(Request.oper, "create") == 0)
                        {
                            fprintf(LogFilePtr, "Create operation of %s in %s successful\n", Request.path2, Request.path1);
                            fflush(LogFilePtr);
                            printf("Create operation successful\n");
                            char *Path = (char *)malloc(sizeof(char) * SIZEOFNAME);
                            strcpy(Path, Request.path1);
                            strcat(Path, "/");
                            strcat(Path, Request.path2);
                            insert(ServerAccessiblePaths, Path, value);
                            free(Path);
                        }
                        else
                        {
                            fprintf(LogFilePtr, "Delete operation of %s successful\n", Request.path1);
                            fflush(LogFilePtr);
                            printf("Delete operation successful\n");
                            delete (ServerAccessiblePaths, Request.path1);
                        }
                    }
                    else
                    {
                        if (strcmp(Request.oper, "create") == 0)
                        {
                            fprintf(LogFilePtr, "Create operation from %s in %s unsuccessful\n", Request.path2, Request.path1);
                            fflush(LogFilePtr);
                            printf("Create operation unsuccessful\n");
                        }
                        else
                        {
                            fprintf(LogFilePtr, "Delete operation of %s unsuccessful\n", Request.path1);
                            fflush(LogFilePtr);
                            printf("Delete operation unsuccessful\n");
                        }
                    }
                    if (send(ClientSock, &Resp, sizeof(Resp), 0) == -1)
                    {
                        perror("Sending Response to Client Failed");
                        exit(1);
                    }
                    close(client_socket);
                }
                else
                {
                    Response Resp;
                    Resp.responsevalue = 2;
                    if (send(ClientSock, &Resp, sizeof(Resp), 0) == -1)
                    {
                        perror("Sending Response to Client Failed");
                        exit(1);
                    }
                    fprintf(LogFilePtr, "Operation %s from client(%s) from port(%d) unsuccessful because No valid %s found\n", Request.oper, ip, port, Request.path1);
                    fflush(LogFilePtr);
                }
            }
        }
        // close(ClientSock);
        // return NULL;
    }
    close(ClientSock);
    return NULL;
}

void *ClientAcceptFunc()
{
    char *ip = "127.0.0.1";
    int port = NMCLACCEPTPORT;

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];
    int n;

    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[-]Bind error");
        exit(1);
    }
    listen(server_sock, 5);
    printf("Listening To Clients...\n");
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        // Add to Log File
        if (client_sock < 0)
        {
            perror("Accept Failed");
            exit(1);
        }
        ClientWorksArg ClientArg;
        ClientArg.ClientSock = client_sock;
        inet_ntop(AF_INET, &client_addr.sin_addr, ClientArg.ip, INET_ADDRSTRLEN);
        ClientArg.port = ntohs(client_addr.sin_port);
        pthread_t ClientWorkThreads;
        if (pthread_create(&ClientWorkThreads, NULL, (void *)&ClientWorksFunc, (void *)&ClientArg) != 0)
        {
            perror("Clients Works Func Error");
            exit(1);
        }
        pthread_detach(ClientWorkThreads);
    }
    return NULL;
}

int main()
{
    LogFilePtr = fopen("LogFile.txt", "w+");
    CacheMemory = initCache();
    pthread_t SSAcceptThread;
    if (pthread_create(&SSAcceptThread, NULL, (void *)&StorageServerAcceptFunc, NULL) != 0)
    {
        perror("thread create");
        exit(EXIT_FAILURE);
    }
    pthread_t ClientAcceptThread;
    if (pthread_create(&ClientAcceptThread, NULL, (void *)&ClientAcceptFunc, NULL) != 0)
    {
        perror("thread create");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(ClientAcceptThread, NULL) != 0)
    {
        perror("pthread join");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(SSAcceptThread, NULL) != 0)
    {
        perror("pthread join");
        exit(EXIT_FAILURE);
    }

    return 0;
}