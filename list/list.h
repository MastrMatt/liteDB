#include <stdio.h>
#include <stdlib.h>

typedef enum NodeType {
    NODE_TYPE_FLOAT,
    NODE_TYPE_STRING
} NodeType;

typedef struct ListNode{
    void * data;
    NodeType type;
    
    struct Node *prev;
    struct Node *next;
} ListNode;

typedef struct List{
    Node *head;
    Node *tail;
    int size;
} List;
