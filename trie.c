#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SIZEOFNAME 128
#define ALPHABET_SIZE (30)

struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
    int NoofChildren;
    char Word[SIZEOFNAME];
    bool isEndOfWord;
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

    return pNode;
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

void insert(struct TrieNode *root, const char *path)
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
}

bool search(struct TrieNode *root, const char *path)
{
    struct TrieNode *pCrawl = root;

    char *PETH = strdup(path);

    char *Token = strtok(PETH, "/");
    while (Token != NULL)
    {
        int value = CheckIfChildExists(pCrawl, Token);
        if (value == -1)
        {
            return false;
        }
        else
        {
            pCrawl = pCrawl->children[value];
        }
        Token = strtok(NULL, "/");
    }
    return pCrawl->isEndOfWord;
}
