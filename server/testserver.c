// unit test most of the server functions
#include "server.h"
#include <assert.h>

// test the server functions
bool test_get_response()
{
    // test string
    ValueType type = STRING;
    void *value = "hello";

    ValueType response_type;
    int len;

    char *response;

    response = get_response(type, value);

    // check first byte
    memcpy(&response_type, response, 1);
    if (type != STRING)
    {
        fprintf(stderr, "response type should be string\n");
        return false;
    }

    memcpy(&len, response + 1, 4);
    if (len != strlen(value))
    {
        fprintf(stderr, "string response length should be %ld\n", strlen(value));
        return false;
    }

    if (strcmp(value, response + 5) != 0)
    {
        fprintf(stderr, "string response value should be %s\n", (char *)value);
        return false;
    }

    // test int
    type = INTEGER;
    int i = 123;
    value = &i;

    response = get_response(type, value);

    // check first byte
    memcpy(&response_type, response, 1);
    if (type != INTEGER)
    {
        fprintf(stderr, "response type should be integer\n");
        return false;
    }

    memcpy(&len, response + 1, 4);
    if (len != 4)
    {
        fprintf(stderr, "integer response length should be 4\n");
        return false;
    }

    if (i != *(int *)(response + 5))
    {
        fprintf(stderr, "integer response value should be %d\n", i);
        return false;
    }

    // test float
    type = FLOAT;
    float f = 123.456;
    value = &f;

    response = get_response(type, value);

    // check first byte
    memcpy(&response_type, response, 1);
    if (type != FLOAT)
    {
        fprintf(stderr, "response type should be float\n");
        return false;
    }

    memcpy(&len, response + 1, 4);
    if (len != 4)
    {
        fprintf(stderr, "float response length should be 4\n");
        return false;
    }

    if (f != *(float *)(response + 5))
    {
        fprintf(stderr, "float response value should be %f\n", f);
        return false;
    }

    // all tests passed
    return true;
}

// ! Don't have to unit test all fucntions, some are covered by integration tests

int main()
{
    assert(test_get_response());
    printf("All tests passed\n");
    return 0;
}