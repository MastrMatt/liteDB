#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

typedef enum ListType
{
    LIST_TYPE_INT,
    LIST_TYPE_FLOAT,
    LIST_TYPE_STRING
} ListType;

typedef struct ListNode
{
    void *data;
    ListType listType;

    struct ListNode *prev;
    struct ListNode *next;
} ListNode;

typedef struct List
{
    ListNode *head;
    ListNode *tail;
    int size;
} List;

List *list_init();

bool list_contains(List *list, void *data, ListType listType);
int list_linsert(List *list, void *data, ListType listType);
int list_rinsert(List *list, void *data, ListType listType);

ListNode *list_lremove(List *list);
ListNode *list_rremove(List *list);
int list_removeFromHead(List *list, void *data, ListType listType, int count);
int list_removeFromTail(List *list, void *data, ListType listType, int count);

int list_imodify(List *list, int index, void *data, ListType listType);
int list_trim(List *list, int start, int end);
ListNode *list_iget(List *list, int index);

void list_free_node(ListNode *node);
void list_free_contents(List *list);
void list_print(List *list);
