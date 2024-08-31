#include "list.h"

int main()
{
    List *list = list_init();

    char *test_strings[] = {"hello", "world", "cookies", "are yummy"};

    // test list_linsert
    list_linsert(list, test_strings[1], LIST_TYPE_STRING);
    list_linsert(list, test_strings[0], LIST_TYPE_STRING);

    ListNode *test1 = list_iget(list, 0);
    ListNode *test2 = list_iget(list, 1);

    if ((strcmp((char *)test1->data, test_strings[0]) != 0) || (strcmp((char *)test2->data, test_strings[1]) != 0))
    {
        printf("Test 1 failed\n");
    }

    // test list_rinsert
    list_rinsert(list, test_strings[2], LIST_TYPE_STRING);
    list_rinsert(list, test_strings[3], LIST_TYPE_STRING);

    ListNode *test3 = list_iget(list, 2);
    ListNode *test4 = list_iget(list, 3);

    if ((strcmp((char *)test3->data, test_strings[2]) != 0) || (strcmp((char *)test4->data, test_strings[3]) != 0))
    {
        printf("Test 2 failed\n");
    }

    // test list_lremove
    ListNode *removedNode1 = list_lremove(list);
    if (!removedNode1 || strcmp((char *)removedNode1->data, test_strings[0]) != 0)
    {
        printf("Test 2 failed\n");
    }

    if (removedNode1)
    {
        list_free_node(removedNode1);
    }

    ListNode *test5 = list_iget(list, 0);
    if (strcmp((char *)test5->data, test_strings[1]) != 0)
    {
        printf("Test 3 failed\n");
    }

    // test list_rremove
    ListNode *removedNode2 = list_rremove(list);
    if (!removedNode2 || strcmp((char *)removedNode2->data, test_strings[3]) != 0)
    {
        printf("Test 3 failed\n");
    }
    if (removedNode2)
    {
        list_free_node(removedNode2);
    }

    ListNode *test6 = list_iget(list, list->size - 1);
    if (strcmp((char *)test6->data, test_strings[2]) != 0)
    {
        printf("Test 4 failed\n");
    }

    // test list_imodify
    list_imodify(list, 0, test_strings[3], LIST_TYPE_STRING);
    ListNode *test7 = list_iget(list, 0);
    if (strcmp((char *)test7->data, test_strings[3]) != 0)
    {
        printf("Test 5 failed\n");
    }

    // test list_trim
    list_imodify(list, 0, test_strings[0], LIST_TYPE_STRING);
    list_imodify(list, 1, test_strings[1], LIST_TYPE_STRING);
    list_rinsert(list, test_strings[2], LIST_TYPE_STRING);
    list_rinsert(list, test_strings[3], LIST_TYPE_STRING);

    // trim from 1 to 2
    list_trim(list, 1, 2);

    ListNode *test8 = list_iget(list, 0);
    ListNode *test9 = list_iget(list, 1);

    if ((strcmp((char *)test8->data, test_strings[1]) != 0) || (strcmp((char *)test9->data, test_strings[2]) != 0) || list->size != 2)
    {
        printf("Test 6 failed\n");
    }

    // test list_contains
    List *list2 = list_init();
    int intData = 123;
    float floatData = 123.456;

    list_linsert(list2, test_strings[0], LIST_TYPE_STRING);
    list_linsert(list2, &intData, LIST_TYPE_INT);
    list_linsert(list2, &floatData, LIST_TYPE_FLOAT);

    if (!list_contains(list2, test_strings[0], LIST_TYPE_STRING) || !list_contains(list2, &intData, LIST_TYPE_INT) || !list_contains(list2, &floatData, LIST_TYPE_FLOAT))
    {
        printf("Test 7 failed\n");
        exit(EXIT_FAILURE);
    }

    // create new list to test list_removeFromHead
    List *list3 = list_init();
    list_linsert(list3, test_strings[0], LIST_TYPE_STRING);
    list_linsert(list3, test_strings[2], LIST_TYPE_STRING);
    list_linsert(list3, test_strings[2], LIST_TYPE_STRING);
    list_linsert(list3, test_strings[3], LIST_TYPE_STRING);
    list_linsert(list3, test_strings[3], LIST_TYPE_STRING);
    list_linsert(list3, test_strings[3], LIST_TYPE_STRING);

    // remove 2 instances of test_strings[0]
    list_removeFromHead(list3, test_strings[0], LIST_TYPE_STRING, 0);

    if (list_contains(list3, test_strings[0], LIST_TYPE_STRING))
    {
        printf("Test 8 failed\n");
        exit(EXIT_FAILURE);
    }

    // remove 3 instances of test_strings[3]
    list_removeFromTail(list3, test_strings[3], LIST_TYPE_STRING, 3);

    if (list_contains(list3, test_strings[3], LIST_TYPE_STRING))
    {
        printf("Test 9 failed\n");
        exit(EXIT_FAILURE);
    }

    // remove 1 instance of test_strings[0]
    list_removeFromTail(list3, test_strings[0], LIST_TYPE_STRING, 1);

    if (list_contains(list3, test_strings[0], LIST_TYPE_STRING))
    {
        printf("Test 10 failed\n");
        exit(EXIT_FAILURE);
    }

    // free the list contents
    list_free_contents(list);
    list_free_contents(list2);
    list_free_contents(list3);

    // free the list
    free(list);
    free(list2);
    free(list3);

    printf("All tests passed\n");
}