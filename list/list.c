#include "list.h"


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
        return -1;
    }

    // edit Node
    if (type == NODE_TYPE_STRING) {
        new_node->data = strdup((char *) data);
    } else if (type == NODE_TYPE_FLOAT) {
        new_node->data = calloc(1,sizeof(float));
        if (new_node->data == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            return -1;
        }

        *(float *) new_node->data = *(float *) data;

    } else {
        fprintf(stderr, "Invalid type\n");
        return -1;
    }

    new_node->next = list->head;
    new_node->prev = NULL;

    // edit List
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
        return -1;
    }

    // edit Node
    if (type == NODE_TYPE_STRING) {
        new_node->data = strdup((char *) data);
    } else if (type == NODE_TYPE_FLOAT) {
        new_node->data = calloc(1,sizeof(float));
        if (new_node->data == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            return -1;
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
    list->head = list->head->next;
    list->size--;

    if (list->size == 0) {
        list->tail = NULL;
    }

    // free alloced memory
    free(removed_node->data);
    free(removed_node);

    return 0;
}

// remove from tail
int list_rremove(List * list) {
    if (list->size == 0) {
        fprintf(stderr, "List is empty\n");
        return -1;
    }
    
    ListNode * removed_node = list->tail;

    if (list->tail->prev) {
        list->tail->prev->next = NULL;
    }

    list->tail = list->tail->prev;

    free(removed_node->data);
    free(removed_node);

    list->size--;

}


int list_iremove(List * list, int index) {
    if (index < 0 || index >= list->size) {
        fprintf(stderr, "Index out of bounds\n");
        return -1;
    }

    ListNode * traverse = list->head;
    for (int i = 0; i < index; i++) {
        traverse = traverse->next;
    }

    if (traverse->prev) {
        traverse->prev->next = traverse->next;
    } else {
        list->head = traverse->next;
    }

    if (traverse->next) {
        traverse->next->prev = traverse->prev;
    } else {
        list->tail = traverse->prev;
    }

    free(traverse->data);
    free(traverse);

    list->size--;

    return 0;
}





