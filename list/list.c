#include "list.h"

// This will be a doubly linked list, allow bidirectional traversal

// initialize the list
List * list_init() {
    List * new_list = (List *) calloc(1,sizeof(List));

    new_list->head = NULL;
    new_list->tail = NULL;
    new_list->size = 0;

    return new_list;
}


//insert at head
int list_linsert(List * list, void * data, NodeType type) {
    ListNode * new_node = (ListNode *) calloc(1,sizeof(ListNode));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // edit Node
    if (type == NODE_TYPE_STRING) {
        new_node->data = strdup((char *) data);
    } else if (type == NODE_TYPE_FLOAT) {
        new_node->data = calloc(1,sizeof(float));
        if (new_node->data == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        *(float *) new_node->data = *(float *) data;

    } else {
        fprintf(stderr, "Invalid type\n");
        return -1;
    }

    new_node->next = list->head;
    new_node->prev = NULL;

    // edit List
    if (list->head) {
        list->head->prev = new_node;
    }
    list->head = new_node;

    if (!list->tail) {
        list->tail = new_node;
    }

    list->size++;

    return 0;
}


// insert at tail
int list_rinsert(List * list, void * data, NodeType type) {
    ListNode * new_node = (ListNode *) calloc(1,sizeof(ListNode));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // edit Node
    if (type == NODE_TYPE_STRING) {
        new_node->data = strdup((char *) data);
    } else if (type == NODE_TYPE_FLOAT) {
        new_node->data = calloc(1,sizeof(float));
        if (new_node->data == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        *(float *) new_node->data = *(float *) data;

    } else {
        fprintf(stderr, "Invalid type\n");
        return -1;
    }
    
    new_node->next = NULL;
    new_node->prev = list->tail;

    // edit List
    if (list->tail) {
        list->tail->next = new_node;
    }
    list->tail = new_node;

    if (!list->head) {
        list->head = new_node;
    }

    list->size++;

    return 0;
}


// remove from head
int list_lremove(List * list) {
    if (list->size == 0) {
        fprintf(stderr, "List is empty\n");
        return -1;
    }

    ListNode * removed_node = list->head;

    // Check if the head node exists and update the prev pointer of the new head
    if (list->head && list->head->next) {
        list->head->next->prev = NULL;
        list->head = list->head->next;
    } else {
        // If the list becomes empty after removal, set head and tail to NULL

        list->head = NULL;
        list->tail = NULL;
    }

    // free alloced memory
    if (removed_node) {
        free(removed_node->data);
        free(removed_node);
    }

    list->size--;

    return 0;
}

// remove from tail
int list_rremove(List *list) {
    if (list->size == 0) {
        fprintf(stderr, "List is empty\n");
        return -1;
    }
    
    ListNode *removed_node = list->tail;

    // Check if the tail node exists and update the prev pointer of the new tail
    if (list->tail && list->tail->prev) {
        list->tail->prev->next = NULL;
        list->tail = list->tail->prev;
    } else {
        // If the list becomes empty after removal, set head and tail to NULL
        list->head = NULL;
        list->tail = NULL;
    }

    // Free the data and the node itself
    if (removed_node) {
        free(removed_node->data);
        free(removed_node);
    }

    list->size--;

    return 0;
}

int list_imodify(List * list, int index, void * data, NodeType type) {
    if (index < 0 || index >= list->size) {
        fprintf(stderr, "Index out of bounds\n");
        return -1;
    }
    
    ListNode * traverse = list->head;
    for (int i = 0; i < index; i++) {
        traverse = traverse->next;
    }

    if (type == NODE_TYPE_STRING) {
        free(traverse->data);
        traverse->data = strdup((char *) data);
    } else if (type == NODE_TYPE_FLOAT) {
        free(traverse->data);
        traverse->data = calloc(1,sizeof(float));
        if (traverse->data == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        *(float *) traverse->data = *(float *) data;

    } else {
        fprintf(stderr, "Invalid type\n");
        return -1;
    }

    return 0;
}


ListNode * list_iget(List * list, int index) {
    if (index < 0 || index >= list->size) {
        fprintf(stderr, "Index out of bounds\n");
        return NULL;
    }

    ListNode * traverse = list->head;
    for (int i = 0; i < index; i++) {
        traverse = traverse->next;
    }

    return traverse;
}

List * list_trim(List * list, int start, int end) {
    if (start < 0 || start >= list->size || end < 0 || end >= list->size) {
        fprintf(stderr, "Index out of bounds\n");
        return NULL;
    }

    // free the nodes that are trimmed from the start
    for (int i = 0; i < start; i++) {
        list_lremove(list);
    }

    // free the nodes that are trimmed from the end
    for (int i = list->size - 1; i > end; i--) {
        list_rremove(list);
    }

    return list;
}






    


