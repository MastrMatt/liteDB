#include "list.h"

int main () {
    List * list = list_init();

    char * test_strings [] = {"hello", "world", "cookies", "are yummy"};

    // test list_linsert
    list_linsert(list, test_strings[1], NODE_TYPE_STRING);
    list_linsert(list, test_strings[0], NODE_TYPE_STRING);

    ListNode * test1 = list_iget(list, 0);
    ListNode * test2 = list_iget(list, 1);

    if ((strcmp((char *) test1->data, test_strings[0]) != 0) || (strcmp((char *) test2->data, test_strings[1]) != 0)) {
        printf("Test 1 failed\n");
    } 


    // test list_rinsert
    list_rinsert(list, test_strings[2], NODE_TYPE_STRING);
    list_rinsert(list, test_strings[3], NODE_TYPE_STRING);

    ListNode * test3 = list_iget(list, 2);
    ListNode * test4 = list_iget(list, 3);

    if ((strcmp((char *) test3->data, test_strings[2]) != 0) || (strcmp((char *) test4->data, test_strings[3]) != 0)) {
        printf("Test 2 failed\n");
    }

    // test list_lremove
    list_lremove(list);
    ListNode * test5 = list_iget(list, 0);
    if (strcmp((char *) test5->data, test_strings[1]) != 0) {
        printf("Test 3 failed\n");
    }

    // test list_rremove
    list_rremove(list);
    ListNode * test6 = list_iget(list, list->size -1);
    if (strcmp((char *) test6->data, test_strings[2]) != 0) {
        printf("Test 4 failed\n");
    }


    // ! Finish rest of tests

    printf("----------------\n");
    list_print(list);
    printf("----------------\n");


    printf("Size of list: %d\n", list->size);
    printf("All tests passed\n");

}