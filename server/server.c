#include "server.h"

// protocol header
#include "../protocol.h"

// global variables
HashTable *global_table;
AOF *global_aof;
pthread_t aof_thread;
int server_socket;
Conn *fd2conn[MAX_CLIENTS] = {0};

/**
 * @brief Set a file descriptor to nonblocking mode
 *
 * @param fd file descriptor to set to nonblocking mode
 */
void set_fd_nonblocking(int fd)
{
    // get the current flags for the fd from the OS
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);

    if (errno)
    {
        perror("fcntl failed");
        exit(1);
    }

    // set the nonblocking flag
    flags |= O_NONBLOCK;

    // set the new flags for the fd
    errno = 0;
    int ret = fcntl(fd, F_SETFL, flags) < 0;

    if (ret < 0)
    {
        perror("fcntl failed");
        exit(1);
    }

    return;
}

/**
 * @brief Accept a new pending client connection and add it to the list of client connections
 *
 * @param fd2conn array of client connections
 * @param server_socket file descriptor of the server socket
 *
 * @return int 0 on success, -1 on failure
 */
int accept_new_connection(Conn *fd2conn[], int server_socket)
{
    // accept the new connection
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    int confd = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (confd < 0)
    {
        perror("accept failed");
        return -1;
    }

    // set the new connection to non-blocking
    set_fd_nonblocking(confd);

    // find an empty slot in the fd2conn array
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (!fd2conn[i])
        {
            break;
        }
    }

    if (i == MAX_CLIENTS)
    {
        fprintf(stderr, "Too many clients\n");
        close(confd);
        return -1;
    }

    // create a new connection object
    Conn *conn = (Conn *)calloc(1, sizeof(Conn));
    if (!conn)
    {
        fprintf(stderr, "Failed to allocate memory for connection\n");
        exit(EXIT_FAILURE);
    }
    conn->fd = confd;
    conn->state = STATE_REQ;

    // add the connection to the fd2conn array
    fd2conn[i] = conn;

    return 0;
}

/**
 * @brief Handles the IO for a connection
 *
 * @param conn connection object
 */
void connection_io(Conn *conn)
{
    if (conn->state == STATE_REQ)
    {
        state_req(conn);
    }
    else if (conn->state == STATE_RESP)
    {
        state_resp(conn);
    }
    else
    {
        // Should not be in the done state here
        fprintf(stderr, "Invalid state\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Generates a value response according to the liteDB protocol
 *
 *  See README for more information on protocol
 *
 * @param type type of the value
 * @param value value to send to the client
 *
 * @return char* response
 */
char *get_response(ValueType type, void *value)
{
    SerialType ser_type;
    int value_len = 0;

    switch (type)
    {
    case STRING:
        ser_type = SER_STR;
        value_len = strlen(value);
        break;
    case INTEGER:
        ser_type = SER_INT;
        value_len = sizeof(int);
        break;
    case FLOAT:
        ser_type = SER_FLOAT;
        value_len = sizeof(float);
        break;
    default:
        fprintf(stderr, "Invalid value type\n");
        exit(EXIT_FAILURE);
    }

    // null terminate the response
    char *response = calloc(1 + 4 + value_len, sizeof(char));

    if (!response)
    {
        fprintf(stderr, "Failed to allocate memory for get response\n");
        exit(EXIT_FAILURE);
    }

    // write the type of the response, 1 byte
    memcpy(response, &ser_type, 1);

    // write the length of the value, 4 bytes
    memcpy(response + 1, &value_len, 4);

    // write the value
    memcpy(response + 1 + 4, value, value_len);

    return response;
}

/**
 * @brief Generates a null response according to the liteDB protocol
 *
 *  See README for more information on protocol
 *
 * @return char* response
 */
char *null_response()
{
    SerialType type = SER_NIL;
    int value_len = 0;

    // null terminate the response
    char *response = calloc(1 + 4, sizeof(char));

    if (!response)
    {
        fprintf(stderr, "Failed to allocate memory for set response\n");
        exit(EXIT_FAILURE);
    }

    // write the type of the response, 1 byte
    memcpy(response, &type, 1);

    // write the length of the value, 4 bytes
    memcpy(response + 1, &value_len, 4);

    return response;
}

/**
 * @brief Generates an error response according to the liteDB protocol
 *
 *  See README for more information on protocol
 *
 * @param err_msg error message
 *
 * @return char* response
 */
char *error_response(char *err_msg)
{
    SerialType type = SER_ERR;
    int err_msg_len = strlen(err_msg);

    // null terminate the response
    char *response = calloc(1 + 4 + err_msg_len, sizeof(char));

    if (!response)
    {
        fprintf(stderr, "Failed to allocate memory for error response\n");
        exit(EXIT_FAILURE);
    }

    // write the type of the response, 1 byte
    memcpy(response, &type, 1);

    // write the length of the error message, 4 bytes
    memcpy(response + 1, &err_msg_len, 4);

    // write the error message
    memcpy(response + 1 + 4, err_msg, err_msg_len);

    return response;
}

// Parse a request from the client(not null terminated) , need to free the returned command and it's args
/**
 * @brief Parse an input string from the client into a Command struct
 *
 * Spaces are used as a delimiter to toknize the characters in the input string,
 * The first token is expected to be the command name, and the rest are expected to be the arguments.
 *
 * @param cmd_string input string from the client
 * @param size size of the input string
 *
 * @return Command* - parsed command
 */
Command *parse_cmd_string(char *cmd_string, int size)
{
    Command *cmd = calloc(1, sizeof(Command));
    if (!cmd)
    {
        fprintf(stderr, "Failed to allocate memory for Command\n");
        exit(EXIT_FAILURE);
    }

    // Create a new null-terminated string to hold the command string
    char *n_cmd_string = calloc(size + 1, sizeof(char));
    if (!n_cmd_string)
    {
        fprintf(stderr, "Failed to allocate memory for n_cmd_string\n");
        exit(EXIT_FAILURE);
    }
    strncpy(n_cmd_string, cmd_string, size);

    // Tokenize the command string by splitting at spaces
    char *token = strtok(n_cmd_string, " ");
    int args = 0;

    while (token != NULL)
    {
        if (args == 0)
        {
            cmd->name = strdup(token); // Use strdup for convenience
        }
        else
        {
            cmd->args[args - 1] = strdup(token); // Assuming args array is preallocated
        }
        token = strtok(NULL, " ");
        args++;
    }

    cmd->num_args = args - 1;

    free(n_cmd_string);

    return cmd;
}

/**
 * @brief Write a command to the AOF file
 *
 * @param cmd Command to write to the AOF file
 *
 * @return void
 */
void handle_aof_write(Command *cmd)
{

    // check if the AOF was initialized
    if (!global_aof)
    {
        fprintf(stderr, "AOF not initialized\n");
        exit(EXIT_FAILURE);
    }

    // create a string from the command
    char message[MAX_MESSAGE_SIZE + 1] = {'\0'};

    // write the command name
    snprintf(message, MAX_MESSAGE_SIZE, "%s", cmd->name);

    // write the command arguments
    for (int i = 0; i < cmd->num_args; i++)
    {
        snprintf(message + strlen(message), MAX_MESSAGE_SIZE - strlen(message) - 1, " %s", cmd->args[i]);
    }

    // write the final newline character`
    snprintf(message + strlen(message), MAX_MESSAGE_SIZE - strlen(message) - 1, "\n");

    // write the command to the AOF
    aof_write(global_aof, message);
}

/**
 * @brief Deletes a key-value pair from the global table and handles cleanup of the value if necessary.
 *
 * @param global_table Global hash table
 * @param key Key to delete from the global table.
 * @param value Value to delete from the global table.
 * @param type Type of the value to delete.
 *
 * @return void
 */
void global_table_del(char *key, char *value, ValueType type)
{
    if (type == ZSET)
    {
        // free the zset contents, don't free the zset itself
        ZSet *zset = (ZSet *)value;
        zset_free_contents(zset);
    }
    else if (type == HASHTABLE)
    {
        // free the hashtable, don't free the hashtable itself
        HashTable *table = (HashTable *)value;
        hfree_table_contents(table);
    }
    else if (type == LIST)
    {
        // free the list
        List *list = (List *)value;
        list_free_contents(list);
    }

    // if the key is not for a ZSET, HASHTABLE, or LIST, no need for extra cleanup, just remove the node from the global table
    HashNode *removed_node = hremove(global_table, key);
    hfree(removed_node);
}

/**
 *  EXISTS (key) -  Checks if the specified key exists in the database. Returns an integer response, 1 if the key exists, 0 otherwise.
 *
 * @param cmd Command structure containing the (key)
 * @return char* response
 */
char *exists_command(Command *cmd)
{

    int elem_exists = 0;

    if (cmd->num_args != 1)
    {
        return error_response("exists command requires 1 argument (key)");
    }

    HashNode *fetched_node = hget(global_table, cmd->args[0]);
    if (!fetched_node)
    {
        elem_exists = 0;
    }
    else
    {
        elem_exists = 1;
    }

    return get_response(INTEGER, &elem_exists);
}

/**
 * @brief Deletes a key-value pair from the global table ,where value is a string and handles AOF logging if necessary.
 *
 * @param cmd Command structure containing the (key)
 * @param aof_restore Flag indicating whether to log the deletion to the AOF file.
 *
 * @return char* A string response indicating the number of elements removed, or NULL if AOF restore is enabled.
 */
char *del_command(Command *cmd, bool aof_restore)
{

    int elem_removed = 0;

    if (cmd->num_args != 1)
    {
        return error_response("del command requires 1 argument (key)");
    }

    HashNode *fetched_node = hget(global_table, cmd->args[0]);
    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // execute delete
    global_table_del(fetched_node->key, fetched_node->value, fetched_node->valueType);
    elem_removed++;

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return get_response(INTEGER, &elem_removed);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Returns a protocol string containing all keys in the global hash table.
 *
 * @return char* Protocol string with keys.
 */
char *keys_command()
{
    // get all the keys in the hash table
    int num_keys = 0;
    int inc_buffer = 5;

    // buffer for the data
    char *buffer = calloc(1 + 4 + MAX_MESSAGE_SIZE, sizeof(char));

    // iterate through the hash table and write the keys to the buffer
    for (int i = 0; i <= global_table->mask; i++)
    {
        HashNode *traverseList = global_table->nodes[i];

        while (traverseList != NULL)
        {
            // write the key to the buffer
            int type = SER_STR;
            int key_len = strlen(traverseList->key);

            // check if the buffer has enough space to write the key
            if (inc_buffer + 5 + key_len > MAX_MESSAGE_SIZE)
            {
                fprintf(stderr, "Failed to reallocate memory for keys response\n");
                exit(EXIT_FAILURE);
            }

            // write the type and length of the response, 1 byte
            memcpy(buffer + inc_buffer, &type, 1);
            memcpy(buffer + inc_buffer + 1, &key_len, 4);

            // write the key
            memcpy(buffer + inc_buffer + 5, traverseList->key, key_len);

            inc_buffer += 5 + key_len;
            num_keys++;
            traverseList = traverseList->next;
        }
    }

    // write the type and length of array to buffer
    int type = SER_ARR;
    memcpy(buffer, &type, 1);
    memcpy(buffer + 1, &num_keys, 4);

    return buffer;
}

/**
 * @brief Flushes the entire database and optionally logs the action to the AOF file.
 *
 * @param cmd Command structure triggering the flush operation.
 * @param aof_restore Flag indicating whether to log the flush operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 */
char *flushall_cmd(Command *cmd, bool aof_restore)
{
    // iterate through the hash table and free all the nodes
    for (int i = 0; i <= global_table->mask; i++)
    {
        HashNode *traverseList = global_table->nodes[i];

        while (traverseList != NULL)
        {
            HashNode *next = traverseList->next;

            // execute delete
            global_table_del(traverseList->key, traverseList->value, traverseList->valueType);

            traverseList = next;
        }
    }

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return null_response();
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Get the value of a key, it the key does not exist return nil. Returns the value
 *
 * @param cmd Command structure specifying the (key)
 *
 * @return char* response
 */
char *get_command(Command *cmd)
{

    // check if the command has the correct number of arguments
    if (cmd->num_args != 1)
    {
        return error_response("get command requires 1 argument (key)");
    }

    // get the value from the hash table
    HashNode *fetched_node = hget(global_table, cmd->args[0]);
    if (!fetched_node)
    {
        return null_response();
    }

    // get the value from the hash node
    ValueType type = fetched_node->valueType;

    if (type != STRING)
    {
        return error_response("Value for this key is not a string");
    }

    char *value = fetched_node->value;

    return get_response(type, value);
}

// returns null response for the set command

/**
 * @brief Executes a SET command and optionally logs the action to the AOF file. All values are stored as strings in the global hashtable.
 *
 * @param cmd Command structure specifying the (key, value)
 * @param aof_restore Flag indicating whether to log the SET operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 *
 */
char *set_command(Command *cmd, bool aof_restore)
{

    // obtain type of the value
    if (cmd->num_args != 2)
    {
        return error_response("set command requires 2 arguments (key, value)");
    }

    // * All data is stored as strings except for the ZSET values
    HashNode *new_node = hinit(strdup(cmd->args[0]), STRING, strdup(cmd->args[1]));
    if (new_node == NULL)
    {
        fprintf(stderr, "Error creating new node for hashtable\n");
        exit(EXIT_FAILURE);
    }

    HashNode *ret = hinsert(global_table, new_node);
    if (ret == NULL)
    {
        return error_response("Failed to insert new node into global table");
    }

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return null_response();
    }
    else
    {
        return NULL;
    }
}

/**
 * The HEXISTS (key, field) command checks if a field exists in a hash . Returns an integer response indicating the number of fields found.
 *
 * @param cmd Command structure specifying the (key, field)
 * @return char* response
 */
char *hexists_command(Command *cmd)
{
    int elem_exists = 0;

    if (cmd->num_args < 2)
    {
        return error_response("hexists command requires at least 2 arguments (key, field)");
    }

    char *global_table_key = cmd->args[0];
    char *field_key = cmd->args[1];

    // fetch the hashtable from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);

    if (!fetched_node)
    {
        fprintf(stderr, "key not in database\n");
        return error_response("key not in database");
    }

    // check if the value is a hashtable
    if (fetched_node->valueType != HASHTABLE)
    {
        fprintf(stderr, "key is not for a hashtable\n");
        return error_response("key is not for a hashtable");
    }

    HashTable *cur_table = (HashTable *)fetched_node->value;

    // check if the field exists in the hashtable
    HashNode *ret_node = hget(cur_table, field_key);

    if (ret_node)
    {
        elem_exists = 1;
    }
    else
    {
        elem_exists = 0;
    }

    return get_response(INTEGER, &elem_exists);
}

/**
 * @brief Executes an HSET command and optionally logs the action to the AOF file.
 *
 * The HSET (key, field, value) command sets the value of a key in a hash table. If the key does not exist, a new hash table is created. Returns a response which containts the number of elements added/updated.
 *
 * @param cmd Command structure specifying the (key, field, value)
 * @param aof_restore Flag indicating whether to log the HSET operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 */
char *hset_command(Command *cmd, bool aof_restore)
{

    ValueType response_type = INTEGER;
    int elem_added = 0;

    if (cmd->num_args < 3)
    {
        return error_response("hset command requires at least 3 arguments (key, field, value)");
    }

    char *global_table_key = cmd->args[0];
    char *field_key = cmd->args[1];
    char *value = cmd->args[2];

    // fetch the hashtable from the global table
    HashTable *cur_table;

    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        // create a new hashtable
        HashTable *new_hash_table = hcreate(INIT_TABLE_SIZE);

        // insert the new hashtable into the global table
        HashNode *new_node = hinit(strdup(global_table_key), HASHTABLE, new_hash_table);

        HashNode *ret = hinsert(global_table, new_node);
        if (!ret)
        {
            return error_response("Failed to insert new hash table into global table");
        }

        cur_table = new_hash_table;
    }
    else
    {
        // check if the value is a hashtable
        if (fetched_node->valueType != HASHTABLE)
        {
            return error_response("key is not for a hashtable");
        }

        cur_table = (HashTable *)fetched_node->value;
    }

    // add the value to the hashtable
    HashNode *new_node = hinit(strdup(field_key), STRING, strdup(value));
    if (!new_node)
    {
        return error_response("Failed to create new node for hashtable");
    }

    // if it already exists, remove the old value
    HashNode *old_node = hremove(cur_table, field_key);
    if (old_node)
    {
        hfree(old_node);
    }

    HashNode *ret = hinsert(cur_table, new_node);
    if (!ret)
    {
        return error_response("Failed to insert new node into hashtable");
    }

    elem_added++;

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_added);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Executes an HGET command and returns the corresponding response string according to the liteDB protocol.
 *
 * Gets the value of field from the hash specified by key. Returns the value. If the key, or field don't exist in database, return nil
 *
 * @param cmd Command structure specifying the (key, field)
 *
 * @return char* response
 */
char *hget_command(Command *cmd)
{
    if (cmd->num_args < 2)
    {
        return error_response("hget command requires at least 2 arguments (key, field)");
    }

    char *global_table_key = cmd->args[0];
    char *field_key = cmd->args[1];

    // fetch the hashtable from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        return null_response();
    }

    // check if the value is a hashtable
    if (fetched_node->valueType != HASHTABLE)
    {
        return error_response("key is not for a hashtable");
    }

    HashTable *cur_table = (HashTable *)fetched_node->value;

    // fetch the value from the hashtable
    HashNode *ret_node = hget(cur_table, field_key);
    if (!ret_node)
    {
        return null_response();
    }

    return get_response(ret_node->valueType, ret_node->value);
}

/**
 * @brief Executes an HDEL command and optionally logs the action to the AOF file.
 *
 * The HDEL command removes a field from a hash table. Returns an integer response indicating the number of elements removed.
 *
 * @param cmd Command structure specifying the (key, field)
 * @param aof_restore Flag indicating whether to log the HDEL operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 */
char *hdel_command(Command *cmd, bool aof_restore)
{

    ValueType response_type = INTEGER;
    int elem_removed = 0;

    if (cmd->num_args < 2)
    {
        return error_response("hdel command requires at least 2 arguments (key, field)");
    }

    char *global_table_key = cmd->args[0];
    char *field_key = cmd->args[1];

    // fetch the hashtable from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // check if the value is a hashtable
    if (fetched_node->valueType != HASHTABLE)
    {
        return error_response("key is not for a hashtable");
    }

    HashTable *cur_table = (HashTable *)fetched_node->value;

    // remove the value from the hashtable
    HashNode *removed_node = hremove(cur_table, field_key);
    if (!removed_node)
    {
        return error_response("Failed to remove value from hashtable");
    }

    // free the removed node
    hfree(removed_node);

    elem_removed++;

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_removed);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Executes an HGETALL command and returns the corresponding response string according to the liteDB protocol.
 *
 * The HGETALL command retrieves all fields and values in a hash table. Returns an error response if the key does not exist.
 *
 * @param cmd Command structure specifying (key)
 *
 * @return char* response
 */
char *hgetall_command(Command *cmd)
{
    if (cmd->num_args < 1)
    {
        return error_response("hgetall command requires at least 1 argument (key)");
    }

    char *global_table_key = cmd->args[0];

    // fetch the hashtable from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // check if the value is a hashtable
    if (fetched_node->valueType != HASHTABLE)
    {
        return error_response("key is not for a hashtable");
    }

    HashTable *cur_table = (HashTable *)fetched_node->value;

    // get all the keys in the hashtable
    int num_elem = 0;
    int inc_buffer = 5;

    // buffer for the data
    char *buffer = calloc(1 + 4 + MAX_MESSAGE_SIZE, sizeof(char));

    // iterate through the hash table and write the keys to the buffer
    for (int i = 0; i <= cur_table->mask; i++)
    {
        HashNode *traverseList = cur_table->nodes[i];

        while (traverseList != NULL)
        {
            // write the key to the buffer
            int type = SER_STR;
            int key_len = strlen(traverseList->key);

            // check if the buffer has enough space to write the key
            if (inc_buffer + 5 + key_len > MAX_MESSAGE_SIZE)
            {
                fprintf(stderr, "Failed to reallocate memory for hgetall response\n");
                exit(EXIT_FAILURE);
            }

            // write the type and length of the response, 1 byte
            memcpy(buffer + inc_buffer, &type, 1);
            memcpy(buffer + inc_buffer + 1, &key_len, 4);

            // write the key
            memcpy(buffer + inc_buffer + 5, traverseList->key, key_len);

            inc_buffer += 5 + key_len;
            num_elem++;

            // write the value to the buffer
            type = SER_STR;
            int value_len = strlen(traverseList->value);

            // check if the buffer has enough space to write the value
            if (inc_buffer + 5 + value_len > MAX_MESSAGE_SIZE)
            {
                fprintf(stderr, "Failed to reallocate memory for hgetall response\n");
                exit(EXIT_FAILURE);
            }

            // write the type and length of the response, 1 byte
            memcpy(buffer + inc_buffer, &type, 1);
            memcpy(buffer + inc_buffer + 1, &value_len, 4);

            // write the value
            memcpy(buffer + inc_buffer + 5, traverseList->value, value_len);

            inc_buffer += 5 + value_len;
            num_elem++;

            traverseList = traverseList->next;
        }
    }

    // write the type and length of array to buffer
    int type = SER_ARR;
    memcpy(buffer, &type, 1);
    memcpy(buffer + 1, &num_elem, 4);

    return buffer;
}

/**
 *  LEXISTS (key, value) - Checks if a value exists in a list. Returns an integer response indicating the number of values found.
 *
 * @param cmd Command structure specifying the (key, value)
 * @return char* response
 *
 */
char *lexists_command(Command *cmd)
{
    int elem_exists = 0;

    if (cmd->num_args < 2)
    {
        return error_response("lexists command requires at least 2 arguments (key, value)");
    }

    char *global_table_key = cmd->args[0];
    char *value = cmd->args[1];

    // fetch the list from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST)
    {
        return error_response("key is not for a list");
    }

    List *list = (List *)fetched_node->value;

    // check if the value exists in the list
    int ret = list_contains(list, value, LIST_TYPE_STRING);
    if (ret)
    {
        elem_exists = 1;
    }
    else
    {
        elem_exists = 0;
    }

    return get_response(INTEGER, &elem_exists);
}

/**
 * @brief Executes an LPUSH command and optionally logs the action to the AOF file.
 *
 * The LPUSH command adds a value to the head of a list. If the key does not exist, a new list is created. Returns an integer response indicating the number of elements added.
 *
 * @param cmd Command structure specifying (key, value)
 * @param aof_restore Flag indicating whether to log the LPUSH operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 */
char *lpush_command(Command *cmd, bool aof_restore)
{
    ValueType response_type = INTEGER;
    int elem_added = 0;

    if (cmd->num_args < 2)
    {
        return error_response("lpush command requires at least 2 arguments (key, value)");
    }

    char *global_table_key = cmd->args[0];
    char *value = cmd->args[1];

    // fetch the list from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        // create a new list
        List *new_list = list_init();

        // create a new hash node
        HashNode *new_node = hinit(strdup(global_table_key), LIST, new_list);

        HashNode *ret = hinsert(global_table, new_node);
        if (!ret)
        {
            return error_response("Failed to insert new list into global table");
        }

        fetched_node = new_node;
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST)
    {
        return error_response("key is not for a list");
    }

    List *list = (List *)fetched_node->value;

    // add the value to the list
    int ret = list_linsert(list, value, LIST_TYPE_STRING);
    if (ret)
    {
        return error_response("Failed to add value to list");
    }
    elem_added++;

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_added);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Executes an RPUSH command and optionally logs the action to the AOF file.
 *
 * The RPUSH command adds a value to the tail of a list. If the key does not exist, a new list is created. Returns an integer response indicating the number of elements added.
 *
 * @param cmd Command structure specifying the (key,value)
 * @param aof_restore Flag indicating whether to log the RPUSH operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 */
char *rpush_command(Command *cmd, bool aof_restore)
{
    ValueType response_type = INTEGER;
    int elem_added = 0;

    if (cmd->num_args < 2)
    {
        return error_response("rpush command requires at least 2 arguments (key, value)");
    }

    char *global_table_key = cmd->args[0];
    char *value = cmd->args[1];

    // fetch the list from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        // create a new list
        List *new_list = list_init();

        // create a new hash node
        HashNode *new_node = hinit(strdup(global_table_key), LIST, new_list);

        HashNode *ret = hinsert(global_table, new_node);
        if (!ret)
        {
            return error_response("Failed to insert new list into global table");
        }

        fetched_node = new_node;
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST)
    {
        return error_response("key is not for a list");
    }

    List *list = (List *)fetched_node->value;

    // add the value to the list
    int ret = list_rinsert(list, value, LIST_TYPE_STRING);
    if (ret)
    {
        return error_response("Failed to add value to list");
    }
    elem_added++;

    return get_response(response_type, &elem_added);
}

/**
 * @brief Executes an LPOP command and optionally logs the action to the AOF file.
 *
 * The LPOP command removes a value from the head of a list. Returns an integer response indicating the number of elements removed.
 *
 * @param cmd Command structure specifying the (key)
 * @param aof_restore Flag indicating whether to log the LPOP operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 */
char *lpop_command(Command *cmd, bool aof_restore)
{

    int elem_removed;

    if (cmd->num_args < 1)
    {
        return error_response("lpop command requires at least 1 argument (key)");
    }

    char *global_table_key = cmd->args[0];

    // fetch the list from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST)
    {
        return error_response("key is not for a list");
    }

    List *list = (List *)fetched_node->value;

    // remove the value from the list
    int ret = list_lremove(list);
    if (ret)
    {
        return error_response("Failed to remove value from list");
    }

    elem_removed++;

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return get_response(INTEGER, &elem_removed);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Executes an RPOP command and optionally logs the action to the AOF file.
 *
 * The RPOP command removes a value from the tail of a list. Returns an integer response indicating the number of elements removed.
 *
 * @param cmd Command structure specifying the (key)
 * @param aof_restore Flag indicating whether to log the RPOP operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 */
char *rpop_command(Command *cmd, bool aof_restore)
{

    int elem_removed = 0;

    if (cmd->num_args < 1)
    {
        return error_response("rpop command requires at least 1 argument (key)");
    }

    char *global_table_key = cmd->args[0];

    // fetch the list from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST)
    {
        return error_response("key is not for a list");
    }

    List *list = (List *)fetched_node->value;

    // remove the value from the list
    int ret = list_rremove(list);
    if (ret)
    {
        return error_response("Failed to remove value from list");
    }

    elem_removed++;

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return get_response(INTEGER, &elem_removed);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Executes an LLEN command and returns the corresponding response string according to the liteDB protocol.
 *
 * The LLEN command returns the length of a list. Returns an integer response indicating the number of elements in the list.
 *
 * @param cmd Command structure specifying the (key)
 *
 * @return char* response
 */
char *llen_cmd(Command *cmd)
{
    int len = 0;

    if (cmd->num_args < 1)
    {
        return error_response("llen command requires at least 1 argument (key)");
    }

    char *global_table_key = cmd->args[0];

    // fetch the list from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);
    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST)
    {
        return error_response("key is not for a list");
    }

    List *list = (List *)fetched_node->value;

    // get the length of the list
    len = list->size;

    return get_response(INTEGER, &len);
}

/**
 * @brief Executes an LRANGE command and returns the corresponding response string according to the liteDB protocol.
 *
 * The LRANGE command retrieves a range of elements from a list. Returns an array response containing the elements in the specified range.
 *
 * @param cmd Command structure specifying the (key, start, stop)
 *
 * @return char* response
 */
char *lrange_cmd(Command *cmd)
{
    errno = 0;

    if (cmd->num_args < 3)
    {
        return error_response("lrange command requires at least 3 arguments (key, start, stop)");
    }

    char *global_table_key = cmd->args[0];
    char *start_str = cmd->args[1];
    char *stop_str = cmd->args[2];

    int start;
    int stop;

    // convert the start to an integer
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    start = (int)strtol(start_str, &endptr, 10);

    // check errors
    if (errno)
    {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == start_str))
    {
        return error_response("Failed to convert start to integer");
    }

    // convert the stop to an integer
    stop = (int)strtol(stop_str, &endptr, 10);

    // check errors
    if (errno)
    {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == stop_str))
    {
        return error_response("Failed to convert stop to integer");
    }

    // fetch the list from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);

    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST)
    {
        return error_response("key is not for a list");
    }

    List *list = (List *)fetched_node->value;

    // check bounds
    if (start < 0 || stop < 0 || start >= list->size || stop >= list->size)
    {
        return error_response("start or stop index out of bounds");
    }

    // get the values from the list
    int elems_to_fetch = stop - start + 1;
    int num_elements = 0;
    int inc_buffer = 5;

    // buffer for the data
    char *buffer = calloc(1 + 4 + MAX_MESSAGE_SIZE, sizeof(char));

    // iterate through the list and write the values to the buffer, start and stop are inclusive
    ListNode *current = list_iget(list, start);
    if (!current)
    {
        return error_response("Failed to get start value from list");
    }

    while (num_elements < elems_to_fetch)
    {
        // write the value to the buffer
        int type = SER_STR;
        int data_len = strlen(current->data);

        // check if the buffer has enough space to write the value
        if (inc_buffer + 5 + data_len > MAX_MESSAGE_SIZE)
        {
            fprintf(stderr, "Failed to reallocate memory for lrange response\n");
            exit(EXIT_FAILURE);
        }

        // write the type and length of the response, 1 byte
        memcpy(buffer + inc_buffer, &type, 1);
        memcpy(buffer + inc_buffer + 1, &data_len, 4);

        // write the value
        memcpy(buffer + inc_buffer + 5, current->data, data_len);

        inc_buffer += 5 + data_len;
        num_elements++;
        current = current->next;
    }

    // write the type and length of array to buffer
    int type = SER_ARR;
    memcpy(buffer, &type, 1);
    memcpy(buffer + 1, &num_elements, 4);

    return buffer;
}

/**
 * @brief Executes an LTRIM command and optionally logs the action to the AOF file.
 *
 * The LTRIM command trims a list to the specified range. Returns a null response if the operation is successful.
 *
 * @param cmd Command structure specifying the (key, start, stop)
 * @param aof_restore Flag indicating whether to log the LTRIM operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 */
char *ltrim_cmd(Command *cmd, bool aof_restore)
{
    errno = 0;

    if (cmd->num_args < 3)
    {
        return error_response("ltrim command requires at least 3 arguments (key, start, stop)");
    }

    char *global_table_key = cmd->args[0];
    char *start_str = cmd->args[1];
    char *stop_str = cmd->args[2];

    int start;
    int stop;

    // convert the start to an integer
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    start = (int)strtol(start_str, &endptr, 10);

    // check errors
    if (errno)
    {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == start_str))
    {
        return error_response("Failed to convert start to integer");
    }

    // convert the stop to an integer
    stop = (int)strtol(stop_str, &endptr, 10);

    // check errors
    if (errno)
    {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == stop_str))
    {
        return error_response("Failed to convert stop to integer");
    }

    // fetch the list from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);

    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST)
    {
        return error_response("key is not for a list");
    }

    List *list = (List *)fetched_node->value;

    // check bounds
    if (start < 0 || stop < 0 || start >= list->size || stop >= list->size)
    {
        return error_response("start or stop index out of bounds");
    }

    // trim the list
    int ret = list_trim(list, start, stop);
    if (ret)
    {
        return error_response("Failed to trim list");
    }

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return null_response();
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Executes an LSET command and optionally logs the action to the AOF file.
 *
 * The LSET command sets the value of an element in a list. Returns an integer response indicating the number of elements updated.
 *
 * @param cmd Command structure specifying the (key, index, value)
 * @param aof_restore Flag indicating whether to log the LSET operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.
 */
char *lset_cmd(Command *cmd, bool aof_restore)
{
    errno = 0;

    ValueType response_type = INTEGER;
    int elem_updated = 0;

    if (cmd->num_args < 3)
    {
        return error_response("lset command requires at least 3 arguments (key, index, value)");
    }

    char *global_table_key = cmd->args[0];
    char *index_str = cmd->args[1];
    char *value = cmd->args[2];

    int index;

    // convert the index to an integer
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    index = (int)strtol(index_str, &endptr, 10);

    // check errors
    if (errno)
    {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == index_str))
    {
        return error_response("Failed to convert index to integer");
    }

    // fetch the list from the global table
    HashNode *fetched_node = hget(global_table, global_table_key);

    if (!fetched_node)
    {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST)
    {
        return error_response("key is not for a list");
    }

    List *list = (List *)fetched_node->value;

    // check bounds
    if (index < 0 || index >= list->size)
    {
        return error_response("index out of bounds");
    }

    // set the value in the list
    int ret = list_imodify(list, index, value, LIST_TYPE_STRING);
    if (ret)
    {
        return error_response("Failed to set value in list");
    }

    elem_updated++;

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_updated);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Executes a ZADD command and optionally logs the action to the AOF file.
 *
 * The ZADD (key, score, name) command adds a value to a sorted set. If the key does not exist, a new sorted set is created. If the field already exists, it is updated instead. Returns an integer response indicating the number of elements added/updated.
 *
 * @param cmd Command structure specifying the (key, score, name)
 * @param aof_restore Flag indicating whether to log the ZADD operation to the AOF file.
 *
 * @return char* response if AOF restore is disabled, or NULL otherwise.

 */
char *zadd_command(Command *cmd, bool aof_restore)
{
    errno = 0;

    ValueType response_type = INTEGER;
    int elem_added = 0;

    if (cmd->num_args < 3)
    {
        return error_response("zadd command requires at least 3 arguments (key, score, name)");
    }

    char *zset_key = cmd->args[0];
    char *score_str = cmd->args[1];

    // convert the score to a float
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    float value = strtof(score_str, &endptr);

    // check errors
    if (errno)
    {
        perror("strtof failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to a float
    if (!(*endptr == '\0') || (endptr == score_str))
    {
        return error_response("Failed to convert score to float");
    }

    // fetch the zset from global table
    HashNode *fetched_node = hget(global_table, zset_key);

    if (!fetched_node)
    {
        // create a new zset,
        ZSet *zset = zset_init();
        if (!zset)
        {
            fprintf(stderr, "Failed to create ZSet\n");
            exit(EXIT_FAILURE);
        }

        // create a new hash node
        HashNode *new_node = hinit(strdup(zset_key), ZSET, zset);
        if (!new_node)
        {
            fprintf(stderr, "Failed to create new hash node\n");
            exit(EXIT_FAILURE);
        }

        // insert the new node into the global table
        HashNode *ret = hinsert(global_table, new_node);
        if (!ret)
        {
            fprintf(stderr, "Failed to insert new node into global table\n");
            exit(EXIT_FAILURE);
        }

        fetched_node = new_node;
    }

    // check if the value is a ZSET
    if (fetched_node->valueType != ZSET)
    {
        return error_response("key is not for a ZSET");
    }

    ZSet *zset = (ZSet *)fetched_node->value;

    // add the value to the ZSET
    int ret = zset_add(zset, cmd->args[2], value);
    if (ret < 0)
    {
        return error_response("Failed to add value to ZSET");
    }
    elem_added++;

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_added);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Executes a ZREM command and optionally logs the action to the AOF file.
 *
 * The ZREM command removes a value from a sorted set. Returns an integer response indicating the number of elements removed.
 *
 * @param cmd Command structure specifying the (key, name)
 * @param aof_restore Flag indicating whether to log the ZREM operation to the AOF file.
 */
char *zrem_command(Command *cmd, bool aof_restore)
{
    errno = 0;

    ValueType response_type = INTEGER;
    int elem_removed = 0;

    if (cmd->num_args < 2)
    {
        return error_response("zrem command requires at least 2 arguments (key, name)");
    }

    char *zset_key = cmd->args[0];
    char *element_key = cmd->args[1];

    // fetch the zset from the global table
    HashNode *fetched_node = hget(global_table, zset_key);
    if (!fetched_node)
    {
        return error_response("zset key not in database");
    }

    // check if the value is a ZSET
    if (fetched_node->valueType != ZSET)
    {
        return error_response("key is not for a zset");
    }

    ZSet *zset = (ZSet *)fetched_node->value;

    // remove the value from the ZSET
    int ret = zset_remove(zset, element_key);
    if (ret < 0)
    {
        return error_response("Failed to remove value from zset");
    }

    elem_removed++;

    if (!aof_restore)
    {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_removed);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Executes a ZSCORE command and returns the corresponding response string according to the liteDB protocol.
 *
 * The ZSCORE command retrieves the score of a field in a sorted set. Returns a float response indicating the score of the field.
 *
 * @param cmd Command structure specifying the  (key, name)
 *
 * @return char* response
 */
char *zscore_cmd(Command *cmd)
{
    errno = 0;

    ValueType response_type = FLOAT;
    float score;

    if (cmd->num_args < 2)
    {
        return error_response("zscore command requires at least 2 arguments (key, name)");
    }

    char *zset_key = cmd->args[0];
    char *element_key = cmd->args[1];

    // fetch the zset from the global table
    HashNode *fetched_node = hget(global_table, zset_key);
    if (!fetched_node)
    {
        return error_response("zset key not in database");
    }

    // check if the value is a ZSET
    if (fetched_node->valueType != ZSET)
    {
        return error_response("key is not for a zset");
    }

    ZSet *zset = (ZSet *)fetched_node->value;

    // search for the value in the ZSET
    HashNode *ret_node = zset_search_by_key(zset, element_key);
    if (!ret_node)
    {
        return error_response("Element not in zset");
    }

    // check if the value is a float
    if (ret_node->valueType != FLOAT)
    {
        fprintf(stderr, "Value in zset is somehow not a float\n");
        exit(EXIT_FAILURE);
    }

    score = *(float *)ret_node->value;

    return get_response(response_type, &score);
}

/**
 * @brief Generates an array response for a range of avl nodes in a sorted set.
 *
 * The function generates an array response containing a range keys and scores in the AVL tree. The range is determined by the start node and the limit. Elements are ordered by thier rank in the AVL tree.
 *
 * @param tree AVL tree to iterate through
 * @param start Starting node in the AVL tree
 * @param limit Maximum number of elements to return
 *
 * @return char* response
 */
char *avl_iterate_response(AVLNode *tree, AVLNode *start, long limit)
{
    int num_elements = 0;
    int inc_buffer = 5;

    // buffer for the data
    char *buffer = calloc(1 + 4 + MAX_MESSAGE_SIZE, sizeof(char));

    // iterate through the AVL tree and write the key and score to the buffer
    AVLNode *current = start;
    while (current != NULL && (num_elements / 2 < limit))
    {
        // write the key to the buffer
        int type = SER_STR;
        int key_len = strlen((char *)current->scnd_index);

        // check if the buffer has enough space to write the key
        if (inc_buffer + 5 + key_len > MAX_MESSAGE_SIZE)
        {
            fprintf(stderr, "Failed to reallocate memory for zquery response\n");
            exit(EXIT_FAILURE);
        }

        // write the type and length of the response, 1 byte
        memcpy(buffer + inc_buffer, &type, 1);
        memcpy(buffer + inc_buffer + 1, &key_len, 4);

        // write the key
        memcpy(buffer + inc_buffer + 5, current->scnd_index, key_len);

        inc_buffer += 5 + key_len;
        num_elements++;

        // now write the score
        type = SER_FLOAT;
        int score_len = sizeof(float);

        // check if the buffer has enough space to write the score
        if (inc_buffer + 5 + score_len > MAX_MESSAGE_SIZE)
        {
            fprintf(stderr, "Failed to reallocate memory for zquery response\n");
            exit(EXIT_FAILURE);
        }

        // write the type and length of the response, 1 byte
        memcpy(buffer + inc_buffer, &type, 1);
        memcpy(buffer + inc_buffer + 1, &score_len, 4);

        // write the score
        memcpy(buffer + inc_buffer + 5, &current->value, score_len);

        inc_buffer += 5 + score_len;
        num_elements++;

        // go to next ranked node
        current = avl_offset(current, 1);
    }

    // write the type and length of array to buffer
    int type = SER_ARR;
    memcpy(buffer, &type, 1);
    memcpy(buffer + 1, &num_elements, 4);

    return buffer;
}

/**
 * @brief Executes a ZQUERY command and returns the corresponding response string according to the liteDB protocol.
 *
 * The ZQUERY command retrieves a range of elements from a sorted set. Returns an array response containing the elements in the specified range.
 *
 * @param cmd Command structure specifying the (key, score, name, offset, limit)
 *
 * @return char* response
 */
char *zquery_cmd(Command *cmd)
{
    errno = 0;

    if (cmd->num_args < 5)
    {
        return error_response("zquery command requires at least 5 arguments (key, score, name, offset, limit)");
    }

    char *zset_key = cmd->args[0];
    char *score_str = cmd->args[1];
    char *element_key = cmd->args[2];
    char *offset_str = cmd->args[3];
    char *limit_str = cmd->args[4];

    float score;
    int offset;
    int limit;

    // convert the score to a float
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    score = strtof(score_str, &endptr);

    // check errors
    if (errno)
    {
        perror("strtof failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to a float
    if (!(*endptr == '\0') || (endptr == score_str))
    {
        return error_response("Failed to convert score to float");
    }

    // convert the offset to an integer
    offset = (int)strtol(offset_str, &endptr, 10);

    // check errors
    if (errno)
    {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == offset_str))
    {
        return error_response("Failed to convert offset to integer");
    }

    // convert the limit to an integer
    limit = (int)strtol(limit_str, &endptr, 10);

    // check errors
    if (errno)
    {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || !(endptr != limit_str))
    {
        return error_response("Failed to convert limit to integer");
    }

    // fetch the zset from the global table
    HashNode *fetched_node = hget(global_table, zset_key);
    if (!fetched_node)
    {
        return error_response("zset key not in database");
    }

    // check if the value is a ZSET
    if (fetched_node->valueType != ZSET)
    {
        return error_response("key is not for a zset");
    }

    ZSet *zset = (ZSet *)fetched_node->value;

    if ((isinf(score) == -1) && (strcmp(element_key, "\"\"") == 0))
    {
        // "" was passed as the key and -inf was passed as the score, perform a rank query
        printf("Performing rank query\n");

        // find the element with smallest rank
        AVLNode *origin_node = get_min_node(zset->avl_tree);

        if (!origin_node)
        {
            return error_response("No valid elements in zset");
        }

        // offset the rank of the node in the AVL tree by the value specified by the offset parameter
        AVLNode *offset_node = avl_offset(origin_node, offset);

        return avl_iterate_response(zset->avl_tree, offset_node, limit);
    }
    else if (strcmp(element_key, "\"\"") == 0)
    {
        // ! potential problem dealing with equal scores here due to the fact that nodes with equal scores can be stored on the right and left based on AVL rotations
        // "" was passed as the key, perform a range query with score without name
        printf("Performing range query\n");

        // find the element in the ZSET using AVL tree
        AVLNode *origin_node = avl_search_float(zset->avl_tree, score);

        if (!origin_node)
        {
            return error_response("No valid elements in zset");
        }

        // offset the rank of the node in the AVL tree by the value specified by the offset parameter
        AVLNode *offset_node = avl_offset(origin_node, offset);

        return avl_iterate_response(zset->avl_tree, offset_node, limit);
    }
    else
    {
        // perform a query for the specific element

        // find the element in the ZSET using AVL tree
        AVLNode *origin_node = avl_search_pair(zset->avl_tree, element_key, score);

        if (!origin_node)
        {
            return error_response("Element not in zset");
        }

        // offset the rank of the node in the AVL tree by the value specified by the offset parameter
        AVLNode *offset_node = avl_offset(origin_node, offset);

        return avl_iterate_response(zset->avl_tree, offset_node, limit);
    }
}

/**
 * @brief Executes a command and returns the corresponding response string according to the liteDB protocol.
 *
 * The function executes a command and returns the corresponding response string according to the liteDB protocol. The function also handles logging the command to the AOF file if the aof_restore flag is set.
 *
 * @param cmd Command structure specifying the command to execute
 * @param aof_restore Flag indicating whether to log the command to the AOF file
 *
 * @return char* response
 */
char *execute_command(Command *cmd, bool aof_restore)
{

    char *return_response;

    if (!cmd->name)
    {
        return_response = error_response("Command name was not specified");
    }
    else if (strcmp(cmd->name, "PING") == 0)
    {
        return_response = get_response(STRING, "PONG");
    }
    else if (strcmp(cmd->name, "EXISTS") == 0)
    {
        return_response = exists_command(cmd);
    }
    else if (strcmp(cmd->name, "DEL") == 0)
    {

        return_response = del_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "KEYS") == 0)
    {

        return_response = keys_command();
    }
    else if (strcmp(cmd->name, "FLUSHALL") == 0)
    {

        return_response = flushall_cmd(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "GET") == 0)
    {

        return_response = get_command(cmd);
    }
    else if (strcmp(cmd->name, "SET") == 0)
    {

        return_response = set_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "HEXISTS") == 0)
    {
        return_response = hexists_command(cmd);
    }
    else if (strcmp(cmd->name, "HSET") == 0)
    {

        return_response = hset_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "HGET") == 0)
    {

        return_response = hget_command(cmd);
    }
    else if (strcmp(cmd->name, "HDEL") == 0)
    {

        return_response = hdel_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "HGETALL") == 0)
    {

        return_response = hgetall_command(cmd);
    }
    else if (strcmp(cmd->name, "LEXISTS") == 0)
    {
        return_response = lexists_command(cmd);
    }
    else if (strcmp(cmd->name, "LPUSH") == 0)
    {

        return_response = lpush_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "RPUSH") == 0)
    {

        return_response = rpush_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "LPOP") == 0)
    {

        return_response = lpop_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "RPOP") == 0)
    {

        return_response = rpop_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "LLEN") == 0)
    {

        return_response = llen_cmd(cmd);
    }
    else if (strcmp(cmd->name, "LRANGE") == 0)
    {

        return_response = lrange_cmd(cmd);
    }
    else if (strcmp(cmd->name, "LTRIM") == 0)
    {

        return_response = ltrim_cmd(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "LSET") == 0)
    {

        return_response = lset_cmd(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "ZADD") == 0)
    {

        return_response = zadd_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "ZREM") == 0)
    {

        return_response = zrem_command(cmd, aof_restore);
    }
    else if (strcmp(cmd->name, "ZSCORE") == 0)
    {

        return_response = zscore_cmd(cmd);
    }
    else if (strcmp(cmd->name, "ZQUERY") == 0)
    {
        return_response = zquery_cmd(cmd);
    }
    else
    {
        return_response = error_response("Unknown command");
    }

    // free the command
    free(cmd->name);

    // free all args
    for (int i = 0; i < cmd->num_args; i++)
    {
        free(cmd->args[i]);
    }

    // free the command
    free(cmd);

    return return_response;
}

/**
 * @brief Restores the database state from the AOF file.
 *
 * The function reads the AOF file line by line and executes the commands to restore the database state. The function is called when the server starts up.
 */
void aof_restore_db()
{
    bool aof_restore = true;

    // check if the AOF was initialized
    if (!global_aof)
    {
        fprintf(stderr, "AOF not initialized\n");
        exit(EXIT_FAILURE);
    }

    // read the AOF file line by line
    char *line;
    while ((line = aof_read_line(global_aof)) != NULL)
    {
        // parse the command
        Command *cmd = parse_cmd_string(line, strlen(line));

        // execute the command
        execute_command(cmd, aof_restore);

        // free the line from aof_read_line()
        free(line);
    }
}

/**
 * @brief Writes a response to a buffer following the liteDB protocol.
 *
 * The function writes a response to a buffer following the liteDB protocol. The response is a byte string that follows the protocol. The function returns the number of bytes written to the buffer.
 *
 * @param buffer Buffer to write the response to
 * @param response Response to write to the buffer
 *
 * @return int number of bytes written to the buffer
 */
int buffer_write_response(char *buffer, char *response)
{

    // write type and size of message
    memcpy(buffer, response, 1 + 4);

    int type = 0;
    memcpy(&type, buffer, 1);

    int message_size = 0;
    memcpy(&message_size, buffer + 1, 4);

    // for the type and size of the message
    int response_size = 1 + 4;

    if (type == SER_ARR)
    {
        for (int i = 0; i < message_size; i++)
        {
            int el_len = 0;
            memcpy(&el_len, response + response_size + 1, 4);

            // make sure the buffer has enough space to write the response
            if (response_size + 5 + el_len > MAX_MESSAGE_SIZE)
            {
                fprintf(stderr, "Failed to reallocate memory for response\n");
                exit(EXIT_FAILURE);
            }

            // write the response
            memcpy(buffer + response_size, response + response_size, 5 + el_len);

            response_size += 5 + el_len;
        }

        return response_size;
    }
    else
    {
        // response without array
        response_size += message_size;

        // check if the buffer has enough space to write the response
        if (response_size > MAX_MESSAGE_SIZE)
        {
            fprintf(stderr, "Failed to reallocate memory for response\n");
            exit(EXIT_FAILURE);
        }

        // write the response
        memcpy(buffer + 5, response + 5, message_size);

        return response_size;
    }
}

/**
 * @brief Attempts to process a single request from a connection.
 *
 * The function attempts to process a single request from a connection. The function checks if the read buffer of the connection has enough data to process a request. If the read buffer has enough data, the function processes the request and writes the response to the write buffer. The function returns true if the request was fully processed and sent to the connection, and false otherwise.
 *
 * @param conn Connection structure to handle
 */
bool try_process_single_request(Conn *conn)
{
    // check if the read buffer has enough data to process a request
    if (conn->current_read_size < 4)
    {
        // not enough data to process a request
        return false;
    }

    // copy the first 4 bytes of the read buffer, assume the message size is in little endian (this machine is little endian)
    int message_size = 0;
    memcpy(&message_size, conn->read_buffer, 4);

    if (message_size > MAX_MESSAGE_SIZE)
    {
        fprintf(stderr, "Message size too large\n");
        conn->state = STATE_DONE;
        return false;
    }

    // check if the read buffer has enough data to process a request
    if (conn->current_read_size < 4 + message_size)
    {
        // not enough data to process a request
        return false;
    }

    // print the message, account for the fact that the message is not null terminated due to pipe-lining
    printf("Client %d says: %.*s\n", conn->fd, message_size, conn->read_buffer + 4);

    // parse the message to extract the command
    Command *cmd = parse_cmd_string(conn->read_buffer + 4, message_size);

    // aof_restore is false, since the command is not being restored from the AOF file
    bool aof_restore = false;

    // execute the command, response is a null terminated byte string following the protocol
    char *response = execute_command(cmd, aof_restore);

    // write response to the write buffer
    int response_size = buffer_write_response(conn->write_buffer, response);

    // free the response
    free(response);

    conn->need_write_size = response_size;

    // remove the request from the read buffer
    int remaining_size = conn->current_read_size - (4 + message_size);

    // using memmove instead of memcpy to handle overlapping memory regions
    if (remaining_size)
    {
        memmove(conn->read_buffer, conn->read_buffer + 4 + message_size, remaining_size);
    }

    conn->current_read_size = remaining_size;

    // the request has been processed, move to the response state
    conn->state = STATE_RESP;
    state_resp(conn);

    // continue the outer loop to process pipelined requests
    return (conn->state == STATE_REQ);
}

/**
 * @brief Attempts to fill the read buffer of a connection.
 *
 * The function attempts to fill the read buffer of a connection. The function reads from the socket and fills the read buffer of the connection. The function returns false to indicate to that the attempt to read any characters into the buffer was unsuccessful or that the connection is waiting for a response, and true otherwise.
 *
 * @param conn Connection structure to handle
 *
 * @return bool indicating if this function should be called again in the while loop of state_req()
 */
bool try_fill_read_buffer(Conn *conn)
{
    // check if the read buffer overflowed
    if (conn->current_read_size > sizeof(conn->read_buffer))
    {
        fprintf(stderr, "Read buffer overflow\n");
        return false;
    }

    // read from the socket
    int read_size = 0;

    // attempt to read from the socket until we have read some characters or a signal has not interrupted the read
    do
    {
        int max_possible_read = sizeof(conn->read_buffer) - conn->current_read_size;

        read_size = read(conn->fd, conn->read_buffer + conn->current_read_size, max_possible_read);
    } while (read_size < 0 && errno == EINTR);

    if ((read_size < 0) && (errno = EAGAIN))
    {
        //  read buffer is full, wait for the next poll event
        return false;
    }

    if (read_size < 0)
    {
        // an error that is not EINTR or EAGAIN occured
        perror("read failed");
        return false;
    }

    if (read_size == 0)
    {
        // if read_size is 0 then EOF reached

        // if the current_read_size is greater than 0, then the EOF was reached before reading the full message
        if (conn->current_read_size > 0)
        {
            fprintf(stderr, "EOF reached before reading full message\n");
        }
        else
        {
            // EOF reached
            printf("EOF reached\n");
        }

        conn->state = STATE_DONE;
        return false;
    }

    conn->current_read_size += read_size;
    if (conn->current_read_size > sizeof(conn->read_buffer))
    {
        fprintf(stderr, "Read buffer overflow\n");
        return false;
    }

    // the loop here is to handle that clients can send multiple requests in one go (pipe-lining)
    while (try_process_single_request(conn))
    {
    };

    return (conn->state == STATE_REQ);
}

/**
 * @brief Attempts to flush the write buffer of a connection.
 *
 * The function attempts to flush the write buffer of a connection. The function writes to the socket and flushes the write buffer of the connection. The function returns false to indicate that the attempt to write any characters into the buffer was unsuccessful or that the connection is waiting for a request, and true otherwise.
 *
 * @param conn Connection structure to handle
 *
 * @return bool indicating if this function should be called again in the while loop of state_resp()
 */
bool try_flush_write_buffer(Conn *conn)
{

    int write_size = 0;

    // attempt to write to the socket until we have written some characters or a signal has not interrupted the write
    do
    {
        int max_possible_write = conn->need_write_size - conn->current_write_size;

        write_size = write(conn->fd, conn->write_buffer + conn->current_write_size, max_possible_write);
    } while (write_size < 0 && errno == EINTR);

    if (write_size < 0)
    {
        if (errno == EAGAIN)
        {
            // write buffer is full, wait for the next poll event
            return false;
        }
        // an error that is not EINTR or EAGAIN occured, exit the connection
        perror("write failed");
        conn->state = STATE_DONE;
        return false;
    }

    conn->current_write_size += write_size;

    if (conn->current_write_size > conn->need_write_size)
    {
        fprintf(stderr, "Write buffer overflow\n");
        conn->state = STATE_DONE;
        return false;
    };

    if (conn->current_write_size == conn->need_write_size)
    {
        // the response has been fully written, move to the request state
        conn->state = STATE_REQ;
        conn->current_write_size = 0;
        conn->need_write_size = 0;

        // exit the outer write loop
        return false;
    }

    // data still in the write buffer, try to write again in the outer loop
    return true;
}

/**
 * @brief Handles the request state of a connection.
 *
 * The function handles the request state of a connection. Calls try_fill_read_buffer() repeatedly.
 *
 * @param conn Connection structure to handle
 */
void state_req(Conn *conn)
{
    while (try_fill_read_buffer(conn))
    {
    };
}

/**
 * @brief Handles the response state of a connection.
 *
 * The function handles the response state of a connection. Calls try_flush_write_buffer() repeatedly.
 *
 * @param conn Connection structure to handle
 */
void state_resp(Conn *conn)
{
    while (try_flush_write_buffer(conn))
    {
    };
}
