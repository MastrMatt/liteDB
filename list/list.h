#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum NodeType {
    NODE_TYPE_FLOAT,
    NODE_TYPE_STRING
} NodeType;

typedef struct ListNode {
    void * data;
    NodeType type;
    
    struct ListNode * prev;
    struct ListNode * next;
} ListNode;

typedef struct List{
    ListNode *head;
    ListNode *tail;
    int size;
} List;


List * list_init();

int list_linsert(List * list, void * data, NodeType type);
int list_rinsert(List * list, void * data, NodeType type);
int list_lremove(List * list);
int list_rremove(List * list);
int list_imodify(List * list, int index, void * data, NodeType type);
ListNode * list_iget(List * list, int index);
List * list_trim(List * list, int start, int end);
List * list_print(List * list);





