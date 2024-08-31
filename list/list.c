// * This file contains the implementation of the doubly linked list. The list is a collection of nodes, each node contains a data field and two pointers to the next and previous nodes. The list can be used to store data of different types, the type of the data is specified by the NodeType enum. Will use a doubly linked list to allow for bidirectional traversal: O(1) insertion and deletion at both ends of the list and at the cost of memory. The list supports insertion, removal, modification, retrieval, and trimming of nodes.

//! When a node is removed using list_lremove or list_rremove, still need to free the node using l_free_node.

#include "list.h"

#define EPSILON 1e-9f

/**
 * @brief Initializes a new doubly list
 *
 * @return List* The initialized list
 */
List *list_init()
{
    List *new_list = (List *)calloc(1, sizeof(List));

    new_list->head = NULL;
    new_list->tail = NULL;
    new_list->size = 0;

    return new_list;
}

/**
 * @brief Compare two floating point numbers using epsilon method
 *
 * @param a The first number
 * @param b The second number
 */
bool compare_float(float a, float b)
{
    return fabs(a - b) < EPSILON;
}

/**
 * @brief Compare two list nodes
 *
 * @param node1 The first node
 * @param node2 The second node
 *
 * @return bool true if the nodes are equal, false otherwise
 */
bool compare_list_node(ListNode *node1, ListNode *node2)
{
    if (node1->listType != node2->listType)
    {
        return false;
    }

    if (node1->listType == LIST_TYPE_INT)
    {
        return *(int *)node1->data == *(int *)node2->data;
    }
    else if (node1->listType == LIST_TYPE_FLOAT)
    {
        return compare_float(*(float *)node1->data, *(float *)node2->data);
    }
    else if (node1->listType == LIST_TYPE_STRING)
    {
        return strcmp((char *)node1->data, (char *)node2->data) == 0;
    }
    else
    {
        // should never reach here, list only supports int, float, and string
        fprintf(stderr, "Invalid type, list should only contain int,float,string\n");
        exit(EXIT_FAILURE);
    }
}

/**
 *  @brief Checks if a list contains a value
 *
 *  @param list The list to check
 *  @param data The data to check for
 *  @param listType The type of the data
 */
bool list_contains(List *list, void *data, ListType listType)
{
    ListNode input_node = {data, listType, NULL, NULL};
    ListNode *current = list->head;

    while (current)
    {
        if (compare_list_node(current, &input_node))
        {
            return true;
        }

        current = current->next;
    }

    return false;
}

/**
 * @brief Insert a new node at the head of the list
 *
 * @param list The list to insert into
 * @param data The data to insert
 * @param listType The type of the data
 *
 * @return int 0 if successful, -1 if failed
 */
int list_linsert(List *list, void *data, ListType listType)
{
    ListNode *new_node = (ListNode *)calloc(1, sizeof(ListNode));
    if (new_node == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // edit Node
    if (listType == LIST_TYPE_STRING)
    {
        new_node->data = strdup((char *)data);
        new_node->listType = LIST_TYPE_STRING;
    }
    else if (listType == LIST_TYPE_FLOAT)
    {
        new_node->data = calloc(1, sizeof(float));
        if (new_node->data == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        *(float *)new_node->data = *(float *)data;
        new_node->listType = LIST_TYPE_FLOAT;
    }
    else if (listType == LIST_TYPE_INT)
    {
        new_node->data = calloc(1, sizeof(int));
        if (new_node->data == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        *(int *)new_node->data = *(int *)data;
        new_node->listType = LIST_TYPE_INT;
    }
    else
    {
        fprintf(stderr, "Invalid type\n");
        return -1;
    }

    new_node->next = list->head;
    new_node->prev = NULL;

    // edit List
    if (list->head)
    {
        list->head->prev = new_node;
    }
    list->head = new_node;

    if (!list->tail)
    {
        list->tail = new_node;
    }

    list->size++;

    return 0;
}

/**
 * @brief Insert a new node at the tail of the list
 *
 * @param list The list to insert into
 * @param data The data to insert
 * @param listType The type of the data
 *
 * @return int 0 if successful, -1 if failed
 */
int list_rinsert(List *list, void *data, ListType listType)
{
    ListNode *new_node = (ListNode *)calloc(1, sizeof(ListNode));
    if (new_node == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // edit Node
    if (listType == LIST_TYPE_STRING)
    {
        new_node->data = strdup((char *)data);
        new_node->listType = LIST_TYPE_STRING;
    }
    else if (listType == LIST_TYPE_FLOAT)
    {
        new_node->data = calloc(1, sizeof(float));
        if (new_node->data == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        *(float *)new_node->data = *(float *)data;
        new_node->listType = LIST_TYPE_FLOAT;
    }
    else if (listType == LIST_TYPE_INT)
    {
        new_node->data = calloc(1, sizeof(int));
        if (new_node->data == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        *(int *)new_node->data = *(int *)data;
        new_node->listType = LIST_TYPE_INT;
    }
    else
    {
        fprintf(stderr, "Invalid type\n");
        return -1;
    }

    new_node->next = NULL;
    new_node->prev = list->tail;

    // edit List
    if (list->tail)
    {
        list->tail->next = new_node;
    }
    list->tail = new_node;

    if (!list->head)
    {
        list->head = new_node;
    }

    list->size++;

    return 0;
}

/**
 * @brief Free a list node
 *
 * @param node List node to free
 *
 */
void list_free_node(ListNode *node)
{
    free(node->data);
    free(node);
}

/**
 * @brief Remove from head
 *
 * @param list The list to remove from
 *
 * @return ListNode* The removed node
 */
ListNode *list_lremove(List *list)
{
    if (list->size == 0)
    {
        fprintf(stderr, "List is empty\n");
        return NULL;
    }

    ListNode *removed_node = list->head;

    // Check if the head node exists and update the prev pointer of the new head
    if (list->head && list->head->next)
    {
        list->head->next->prev = NULL;
        list->head = list->head->next;
    }
    else
    {
        // If the list becomes empty after removal, set head and tail to NULL

        list->head = NULL;
        list->tail = NULL;
    }

    list->size--;

    return removed_node;
}

/**
 * @brief Remove from tail
 *
 * @param list The list to remove from
 *
 * @return int 0 if successful, -1 if failed
 */
ListNode *list_rremove(List *list)
{
    if (list->size == 0)
    {
        fprintf(stderr, "List is empty\n");
        return NULL;
    }

    ListNode *removed_node = list->tail;

    // Check if the tail node exists and update the prev pointer of the new tail
    if (list->tail && list->tail->prev)
    {
        list->tail->prev->next = NULL;
        list->tail = list->tail->prev;
    }
    else
    {
        // If the list becomes empty after removal, set head and tail to NULL
        list->head = NULL;
        list->tail = NULL;
    }

    list->size--;

    return removed_node;
}

/**
 * @brief Remove up to a given "count" of nodes with the given data starting from the head of the list. If count = 0, all nodes with the given data will be removed.

* @param list The list to remove from
* @param data The data to remove
* @param listType The type of the data
* @param count The number of nodes to remove
*
* @return int The number of nodes removed
*/
int list_removeFromHead(List *list, void *data, ListType listType, int count)
{
    int removed_count = 0;
    int amountToRemove = count == 0 ? list->size : count;

    if (amountToRemove < 0)
    {
        fprintf(stderr, "Cannot remove negative count from list\n");
        exit(EXIT_FAILURE);
    }

    ListNode input_node = {data, listType, NULL, NULL};
    ListNode *traverse = list->head;

    while (traverse && removed_count < amountToRemove)
    {
        ListNode *temp = traverse->next;

        if (compare_list_node(traverse, &input_node))
        {
            // update the links
            if (traverse->prev)
            {
                traverse->prev->next = traverse->next;
            }
            else
            {
                list->head = traverse->next;
            }

            if (traverse->next)
            {
                traverse->next->prev = traverse->prev;
            }
            else
            {
                list->tail = traverse->prev;
            }

            // free the node
            list_free_node(traverse);
            removed_count++;
            list->size--;
        }

        // move to the next node
        traverse = temp;
    }

    return removed_count;
}

/**
 * @brief Remove up to a given "count" of nodes with the given data starting from the tail of the list. If count = 0, all nodes with the given data will be removed.
 *
 * @param list The list to remove from
 * @param data The data to remove
 * @param listType The type of the data
 * @param count The number of nodes to remove
 *
 * @return int The number of nodes removed
 */
int list_removeFromTail(List *list, void *data, ListType listType, int count)
{
    int removed_count = 0;
    int amountToRemove = count == 0 ? list->size : count;

    if (amountToRemove < 0)
    {
        fprintf(stderr, "Cannot remove negative count from list\n");
        exit(EXIT_FAILURE);
    }

    ListNode input_node = {data, listType, NULL, NULL};
    ListNode *traverse = list->tail;

    while (traverse && removed_count < amountToRemove)
    {
        ListNode *temp = traverse->prev;

        if (compare_list_node(traverse, &input_node))
        {
            // update the links
            if (traverse->prev)
            {
                traverse->prev->next = traverse->next;
            }
            else
            {
                list->head = traverse->next;
            }

            if (traverse->next)
            {
                traverse->next->prev = traverse->prev;
            }
            else
            {
                list->tail = traverse->prev;
            }

            // free the node
            list_free_node(traverse);
            removed_count++;
            list->size--;
        }

        // move to the next node
        traverse = temp;
    }

    return removed_count;
}

/**
 * @brief Modify the data of a node at a given index
 *
 * @param list The list to modify
 * @param index The index of the node to modify
 * @param listType The type of the data
 * @param type The type of the data
 *
 * @return int 0 if successful, -1 if failed
 */
int list_imodify(List *list, int index, void *data, ListType listType)
{
    if (index < 0 || index >= list->size)
    {
        fprintf(stderr, "Index out of bounds\n");
        return -1;
    }

    ListNode *traverse = list->head;
    for (int i = 0; i < index; i++)
    {
        traverse = traverse->next;
    }

    if (listType == LIST_TYPE_STRING)
    {
        free(traverse->data);
        traverse->data = strdup((char *)data);
    }
    else if (listType == LIST_TYPE_STRING)
    {
        free(traverse->data);
        traverse->data = calloc(1, sizeof(float));
        if (traverse->data == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        *(float *)traverse->data = *(float *)data;
    }
    else
    {
        fprintf(stderr, "Invalid type\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Retrieve the node at a given index
 *
 * @param list The list to retrieve from
 * @param index The index of the node to retrieve
 *
 * @return ListNode* The node at the given index
 */
ListNode *list_iget(List *list, int index)
{
    if (index < 0 || index >= list->size)
    {
        fprintf(stderr, "Index out of bounds\n");
        return NULL;
    }

    ListNode *traverse = list->head;
    for (int i = 0; i < index; i++)
    {
        traverse = traverse->next;
    }

    return traverse;
}

/**
 * @brief Trim the list from start to end
 *
 * @param list The list to trim
 * @param start The start index
 * @param end The end index
 *
 * @return int 0 if successful, -1 if failed
 */
int list_trim(List *list, int start, int end)
{
    if (start < 0 || start >= list->size || end < 0 || end >= list->size)
    {
        fprintf(stderr, "Index out of bounds\n");
        return -1;
    }

    int initial_size = list->size;

    // free the nodes that are trimmed from the start
    for (int i = 0; i < start; i++)
    {
        ListNode *removedNode = list_lremove(list);
        if (!removedNode)
        {
            fprintf(stderr, "Failed to remove some node from trim\n");
            return -1;
        }

        list_free_node(removedNode);
    }

    // free the nodes that are trimmed from the end
    for (int i = initial_size - 1; i > end; i--)
    {
        ListNode *removedNode = list_rremove(list);
        if (!removedNode)
        {
            fprintf(stderr, "Failed to remove some node from trim\n");
            return -1;
        }

        list_free_node(removedNode);
    }

    return 0;
}

/**
 * @brief Free the contents of the list, excluding the list itself
 *
 * @param list The list to free
 */
void list_free_contents(List *list)
{
    ListNode *traverse = list->head;
    while (traverse)
    {
        ListNode *temp = traverse->next;
        free(traverse->data);
        free(traverse);
        traverse = temp;
    }
}

// print the list
void list_print(List *list)
{
    ListNode *traverse = list->head;
    while (traverse)
    {
        if (traverse->listType == LIST_TYPE_STRING)
        {
            printf("%s\n", (char *)traverse->data);
        }
        else if (traverse->listType == LIST_TYPE_FLOAT)
        {
            printf("%f\n", *(float *)traverse->data);
        }
        traverse = traverse->next;
    }
}
