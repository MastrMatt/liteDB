#include "server.h"

// Global structures
HashTable * global_table;
AOF * global_aof;

// !make DEL work for ZSET, HASHTABLE and LIST in the global hash table

// accepts new connection and adds it to the fd2conn array
int accept_new_connection(Conn * fd2conn[], int server_socket) {
    // accept the new connection
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    int confd = accept(server_socket, (struct sockaddr *) &client_address, &client_address_len);
    if (confd < 0) {
        perror("accept failed");
        return -1;
    }

    // set the new connection to non-blocking
    set_fd_nonblocking(confd);

    // find an empty slot in the fd2conn array
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!fd2conn[i]) {
            break;
        }
    }

    if (i == MAX_CLIENTS) {
        fprintf(stderr, "Too many clients\n");
        close(confd);
        return -1;
    }

    // create a new connection object
    Conn * conn = (Conn *) calloc(1, sizeof(Conn));
    conn->fd = confd;
    conn->state = STATE_REQ;

    // add the connection to the fd2conn array
    fd2conn[i] = conn;

    return 0;
}


void connection_io(Conn * conn) {
    if (conn -> state == STATE_REQ) {
        state_req(conn);
    } else if (conn -> state == STATE_RESP) {
        state_resp(conn);
    } else {
        // Should not be in the done state here
        fprintf(stderr, "Invalid state\n");
        exit(1);
    }
}


char * get_response( ValueType type, void * value) {
    SerialType ser_type = SER_NIL;
    int value_len = 0;

    switch (type) {
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
    char * response = calloc(1 + 4 + value_len, sizeof(char));

    if (!response) {
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


char * null_response() {
    SerialType type = SER_NIL;
    int value_len = 0;

    // null terminate the response
    char * response = calloc(1 + 4, sizeof(char));

    if (!response) {
        fprintf(stderr, "Failed to allocate memory for set response\n");
        exit(EXIT_FAILURE);
    }

    // write the type of the response, 1 byte
    memcpy(response, &type, 1);

    // write the length of the value, 4 bytes
    memcpy(response + 1, &value_len, 4);

    return response;
}
    

char * error_response(char * err_msg) {
    SerialType type = SER_ERR;
    int err_msg_len = strlen(err_msg);

    // null terminate the response
    char * response = calloc(1 + 4 + err_msg_len, sizeof(char));

    if (!response) {
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
Command * parse_cmd_string(char *cmd_string, int size) {
    Command *cmd = calloc(1, sizeof(Command));
    if (!cmd) {
        fprintf(stderr, "Failed to allocate memory for Command\n");
        exit(EXIT_FAILURE);
    }

    // Create a new null-terminated string to hold the command string
    char *n_cmd_string = calloc(size + 1, sizeof(char));
    if (!n_cmd_string) {
        fprintf(stderr, "Failed to allocate memory for n_cmd_string\n");
        exit(EXIT_FAILURE);
    }
    strncpy(n_cmd_string, cmd_string, size);

    // Tokenize the command string by splitting at spaces
    char *token = strtok(n_cmd_string, " ");
    int args = 0;

    while (token != NULL) {
        if (args == 0) {
            cmd->name = strdup(token); // Use strdup for convenience
        } else {
            cmd->args[args - 1] = strdup(token); // Assuming args array is preallocated
        }
        token = strtok(NULL, " ");
        args++;
    }

    cmd->num_args = args - 1;

    free(n_cmd_string);

    return cmd;
}


void handle_aof_write(Command * cmd) {

    // check if the AOF was initialized
    if (!global_aof) {
        fprintf(stderr, "AOF not initialized\n");
        exit(EXIT_FAILURE);
    }
    
    // create a string from the command
    char message[MAX_MESSAGE_SIZE] = {'\0'};

    // write the command name
    snprintf(message, MAX_MESSAGE_SIZE, "%s ", cmd->name);

    // write the command arguments
    for (int i = 0; i < cmd->num_args - 1; i++) {
        snprintf(message + strlen(message), MAX_MESSAGE_SIZE - strlen(message) - 1, "%s ", cmd->args[i]);
    }

    // write the final argument
    snprintf(message + strlen(message), MAX_MESSAGE_SIZE - strlen(message) - 1, "%s\n", cmd->args[cmd->num_args - 1]);

    // write the command to the AOF
    aof_write(global_aof, message);

}



// returns a string response representing the number of elements removed
char * del_command(Command * cmd, bool aof_restore) {

    if (cmd->num_args != 1) {
        return error_response("del command requires 1 argument (key)");
    }

    // make sure the key is not for a ZSET
    HashNode * fetched_node = hget(global_table, cmd->args[0]);
    if (!fetched_node) {
        return error_response("Key not in database");
    }

    if (fetched_node->valueType == ZSET) {
        // free the zset contents, don't free the zset itself
        ZSet * zset = (ZSet *) fetched_node->value;
        zset_free_contents(zset);

    } else if (fetched_node->valueType == HASHTABLE) {
        // free the hashtable, don't free the hashtable itself
        HashTable * table = (HashTable *) fetched_node->value;
        hfree_table_contents(table);

    } else if (fetched_node->valueType == LIST) {
        // free the list
        List * list = (List *) fetched_node->value;
        list_free_contents(list);
    } 

    // if the key is not for a ZSET, HASHTABLE, or LIST, no need for extra cleanup, just remove the node from the global table
    HashNode * removed_node = hremove(global_table, cmd->args[0]);
    hfree(removed_node);

    if (!aof_restore) {
        handle_aof_write(cmd);
        return null_response();
    } else {
        return NULL;
    }

}

// returns a protocol string, which contains an array of all the keys in the hash table
char * keys_command() {
    // get all the keys in the hash table
    int num_keys = 0;
    int inc_buffer = 5;

    // buffer for the data
    char * buffer = calloc(1 + 4 + MAX_MESSAGE_SIZE, sizeof(char));

    // iterate through the hash table and write the keys to the buffer
    for (int i = 0; i <= global_table->mask; i++) {
        HashNode * traverseList = global_table->nodes[i];

        while (traverseList != NULL) {
            // write the key to the buffer
            int type = SER_STR;
            int key_len = strlen(traverseList->key);

            // check if the buffer has enough space to write the key
            if (inc_buffer + 5 + key_len > MAX_MESSAGE_SIZE) {
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


// executes and returns a string response for the get command
char * get_command(Command * cmd) {
     // check if the command has the correct number of arguments
    if (cmd->num_args != 1) {
        return error_response("get command requires 1 argument (key)");
    }

    // get the value from the hash table
    HashNode * fetched_node = hget(global_table, cmd->args[0]);
    if (!fetched_node) {
        return error_response("Key not in database");
    }

    // get the value from the hash node
    ValueType type = fetched_node->valueType;

    if (type == ZSET) {
        return error_response("ZSET values not supported for this command");
    }

    char * value = fetched_node->value;

    return get_response(type,value);
}

// returns null response for the set command
char * set_command(Command * cmd, bool aof_restore) {
    
    // obtain type of the value
    if (cmd->num_args != 2) {
        return error_response("set command requires 2 arguments (key, value)");
    }

    // * All data is stored as strings except for the ZSET values
    HashNode * new_node = hinit(strdup(cmd->args[0]), STRING, strdup(cmd->args[1]));
    if (new_node == NULL) {
        fprintf(stderr, "Error creating new node for hashtable\n");
        exit(EXIT_FAILURE);
    }

    HashNode * ret = hinsert(global_table, new_node);
    if (ret == NULL) {
        return error_response("Failed to insert new node into global table");
    }


    if (!aof_restore) {
        handle_aof_write(cmd);
        return null_response();
    } else {
        return NULL;
    }
}

// hashtable only store strings in this db
// returns a string response representing the number of elements added/updated
char * hset_command(Command * cmd, bool aof_restore) {

    ValueType response_type = INTEGER;
    int elem_added = 0;

    if (cmd->num_args < 3) {
        return error_response("hset command requires at least 3 arguments (key, field, value)");
    }

    char * global_table_key = cmd->args[0];
    char * field_key = cmd->args[1];
    char * value = cmd->args[2];

    // fetch the hashtable from the global table
    HashTable * cur_table;

    HashNode * fetched_node = hget(global_table, global_table_key);
    if (!fetched_node) {
        // create a new hashtable
        HashTable * new_hash_table = hcreate(INIT_TABLE_SIZE);

        // insert the new hashtable into the global table
        HashNode * new_node = hinit(strdup(global_table_key), HASHTABLE, new_hash_table);

        HashNode * ret = hinsert(global_table, new_node);
        if (!ret) {
            return error_response("Failed to insert new hash table into global table");
        }

        cur_table = new_hash_table;

    } else {
        // check if the value is a hashtable
        if (fetched_node->valueType != HASHTABLE) {
            return error_response("key is not for a hashtable");
        }

        cur_table = (HashTable *) fetched_node->value;
    }

    // add the value to the hashtable
    HashNode * new_node = hinit(strdup(field_key), STRING, strdup(value));
    if (!new_node) {
        return error_response("Failed to create new node for hashtable");
    }

    // if it already exists, remove the old value
    HashNode * old_node = hremove(cur_table, field_key);
    if (old_node) {
        hfree(old_node);
    }

    HashNode * ret = hinsert(cur_table, new_node);
    if (!ret) {
        return error_response("Failed to insert new node into hashtable");
    }

    elem_added++;

    if (!aof_restore) {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_added);
    } else {
        return NULL;
    }
}

// returns a string response for the hget command
char * hget_command(Command * cmd) {
    if (cmd->num_args < 2) {
        return error_response("hget command requires at least 2 arguments (key, field)");
    }

    char * global_table_key = cmd->args[0];
    char * field_key = cmd->args[1];

    // fetch the hashtable from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);
    if (!fetched_node) {
        return error_response("key not in database");
    }

    // check if the value is a hashtable
    if (fetched_node->valueType != HASHTABLE) {
        return error_response("key is not for a hashtable");
    }

    HashTable * cur_table = (HashTable *) fetched_node->value;

    // fetch the value from the hashtable
    HashNode * ret_node = hget(cur_table, field_key);
    if (!ret_node) {
        return error_response("field not in hashtable");
    }

    return get_response(ret_node->valueType, ret_node->value);
}

// returns an integer response representing the number of elements removed
char * hdel_command(Command * cmd, bool aof_restore) {

    ValueType response_type = INTEGER;
    int elem_removed = 0;

    if (cmd->num_args < 2) {
        return error_response("hdel command requires at least 2 arguments (key, field)");
    }

    char * global_table_key = cmd->args[0];
    char * field_key = cmd->args[1];

    // fetch the hashtable from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);
    if (!fetched_node) {
        return error_response("key not in database");
    }

    // check if the value is a hashtable
    if (fetched_node->valueType != HASHTABLE) {
        return error_response("key is not for a hashtable");
    }

    HashTable * cur_table = (HashTable *) fetched_node->value;

    // remove the value from the hashtable
    HashNode * removed_node = hremove(cur_table, field_key);
    if (!removed_node) {
        return error_response("Failed to remove value from hashtable");
    }

    // free the removed node
    hfree(removed_node);

    elem_removed++;

    if (!aof_restore) {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_removed);
    } else {
        return NULL;
    }
}

// returns a protocol string, which contains an array of all the keys and values in the hash table
char * hgetall_command(Command * cmd) {
    if (cmd->num_args < 1) {
        return error_response("hgetall command requires at least 1 argument (key)");
    }

    char * global_table_key = cmd->args[0];

    // fetch the hashtable from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);
    if (!fetched_node) {
        return error_response("key not in database");
    }

    // check if the value is a hashtable
    if (fetched_node->valueType != HASHTABLE) {
        return error_response("key is not for a hashtable");
    }

    HashTable * cur_table = (HashTable *) fetched_node->value;

    // get all the keys in the hashtable
    int num_elem = 0;
    int inc_buffer = 5;

    // buffer for the data
    char * buffer = calloc(1 + 4 + MAX_MESSAGE_SIZE, sizeof(char));

    // iterate through the hash table and write the keys to the buffer
    for (int i = 0; i <= cur_table->mask; i++) {
        HashNode * traverseList = cur_table->nodes[i];

        while (traverseList != NULL) {
            // write the key to the buffer
            int type = SER_STR;
            int key_len = strlen(traverseList->key);

            // check if the buffer has enough space to write the key
            if (inc_buffer + 5 + key_len > MAX_MESSAGE_SIZE) {
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
            if (inc_buffer + 5 + value_len > MAX_MESSAGE_SIZE) {
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



// returns an integer response representing the number of elements added
char * lpush_command(Command * cmd, bool aof_restore) {
    ValueType response_type = INTEGER;
    int elem_added = 0;

    if (cmd->num_args < 2) {
        return error_response("lpush command requires at least 2 arguments (key, value)");
    }
    
    char * global_table_key = cmd->args[0];
    char * value = cmd->args[1];

    // fetch the list from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);
    if (!fetched_node) {
        // create a new list
        List * new_list = list_init();

        // create a new hash node
        HashNode * new_node = hinit(strdup(global_table_key), LIST, new_list);

        HashNode * ret = hinsert(global_table, new_node);
        if (!ret) {
            return error_response("Failed to insert new list into global table");
        }

        fetched_node = new_node;
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST) {
        return error_response("key is not for a list");
    }

    List * list = (List *) fetched_node->value;

    // add the value to the list
    int ret = list_linsert(list, value, STRING);
    if (ret) {
        return error_response("Failed to add value to list");
    }
    elem_added++;

    if (!aof_restore) {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_added);
    } else {
        return NULL;
    }
}

// returns an integer response representing the number of elements added
char * rpush_command(Command * cmd, bool aof_restore) {
    ValueType response_type = INTEGER;
    int elem_added = 0;

    if (cmd->num_args < 2) {
        return error_response("rpush command requires at least 2 arguments (key, value)");
    }
    
    char * global_table_key = cmd->args[0];
    char * value = cmd->args[1];

    // fetch the list from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);
    if (!fetched_node) {
        // create a new list
        List * new_list = list_init();

        // create a new hash node
        HashNode * new_node = hinit(strdup(global_table_key), LIST, new_list);

        HashNode * ret = hinsert(global_table, new_node);
        if (!ret) {
            return error_response("Failed to insert new list into global table");
        }


        fetched_node = new_node;
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST) {
        return error_response("key is not for a list");
    }

    List * list = (List *) fetched_node->value;

    // add the value to the list
    int ret = list_rinsert(list, value, STRING);
    if (ret) {
        return error_response("Failed to add value to list");
    }
    elem_added++;

    return get_response(response_type, &elem_added);
}

// returns an integer response representing the number of elements removed
char * lpop_command(Command * cmd, bool aof_restore) {

    int elem_removed;

    if (cmd->num_args < 1) {
        return error_response("lpop command requires at least 1 argument (key)");
    }

    char * global_table_key = cmd->args[0];

    // fetch the list from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);
    if (!fetched_node) {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST) {
        return error_response("key is not for a list");
    }

    List * list = (List *) fetched_node->value;

    // remove the value from the list
    int ret = list_lremove(list);
    if (ret) {
        return error_response("Failed to remove value from list");
    }

    elem_removed++;
    

    if (!aof_restore) {
        handle_aof_write(cmd);
        return get_response(INTEGER, &elem_removed);
    } else {
        return NULL;
    }

}

// returns an integer response representing the number of elements removed
char * rpop_cmd(Command * cmd, bool aof_restore) {

    int elem_removed = 0;

    if (cmd->num_args < 1) {
        return error_response("rpop command requires at least 1 argument (key)");
    }

    char * global_table_key = cmd->args[0];

    // fetch the list from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);
    if (!fetched_node) {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST) {
        return error_response("key is not for a list");
    }

    List * list = (List *) fetched_node->value;

    // remove the value from the list
    int ret = list_rremove(list);
    if (ret) {
        return error_response("Failed to remove value from list");
    }

    elem_removed++;

    if (!aof_restore) {
        handle_aof_write(cmd);
        return get_response(INTEGER, &elem_removed);
    } else {
        return NULL;
    }

}

// returns an integer response representing the length of the list
char * llen_cmd(Command * cmd) {
    int len = 0;

    if (cmd->num_args < 1) {
        return error_response("llen command requires at least 1 argument (key)");
    }

    char * global_table_key = cmd->args[0];

    // fetch the list from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);
    if (!fetched_node) {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST) {
        return error_response("key is not for a list");
    }

    List * list = (List *) fetched_node->value;

    // get the length of the list
    len = list->size;

    return get_response(INTEGER, &len);
}

// returns a protocol string, which contains an array of all the elements in the range
char * lrange_cmd(Command * cmd) {
    errno = 0;

    if (cmd->num_args < 3) {
        return error_response("lrange command requires at least 3 arguments (key, start, stop)");
    }

    char * global_table_key = cmd->args[0];
    char * start_str = cmd->args[1];
    char * stop_str = cmd->args[2];
    
    int start;
    int stop;

    // convert the start to an integer
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    start = (int) strtol(start_str, &endptr, 10);

    // check errors
    if (errno) {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == start_str)) {
        return error_response("Failed to convert start to integer");
    }

    // convert the stop to an integer
    stop = (int) strtol(stop_str, &endptr, 10);

    // check errors
    if (errno) {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == stop_str)) {
        return error_response("Failed to convert stop to integer");
    }

    // fetch the list from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);

    if (!fetched_node) {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST) {
        return error_response("key is not for a list");
    }

    List * list = (List *) fetched_node->value;

    // check bounds
    if (start < 0 || stop < 0 || start >= list->size || stop >= list->size) {
        return error_response("start or stop index out of bounds");
    }

    // get the values from the list
    int elems_to_fetch = stop - start + 1;
    int num_elements = 0;
    int inc_buffer = 5;

    // buffer for the data
    char * buffer = calloc(1 + 4 + MAX_MESSAGE_SIZE, sizeof(char));

    // iterate through the list and write the values to the buffer, start and stop are inclusive
    ListNode * current = list_iget(list, start);
    if (!current) {
        return error_response("Failed to get start value from list");
    }

    while (num_elements < elems_to_fetch) {
        // write the value to the buffer
        int type = SER_STR;
        int data_len = strlen(current->data);

        // check if the buffer has enough space to write the value
        if (inc_buffer + 5 + data_len > MAX_MESSAGE_SIZE) {
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


// returns null response
char * ltrim_cmd(Command * cmd, bool aof_restore) {
    errno = 0; 

    if (cmd->num_args < 3) {
        return error_response("ltrim command requires at least 3 arguments (key, start, stop)");
    }

    char * global_table_key = cmd->args[0];
    char * start_str = cmd->args[1];
    char * stop_str = cmd->args[2];
    
    int start;
    int stop;

    // convert the start to an integer
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    start = (int) strtol(start_str, &endptr, 10);

    // check errors
    if (errno) {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == start_str)) {
        return error_response("Failed to convert start to integer");
    }

    // convert the stop to an integer
    stop = (int) strtol(stop_str, &endptr, 10);

    // check errors
    if (errno) {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == stop_str)) {
        return error_response("Failed to convert stop to integer");
    }

    // fetch the list from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);

    if (!fetched_node) {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST) {
        return error_response("key is not for a list");
    }

    List * list = (List *) fetched_node->value;

    // check bounds
    if (start < 0 || stop < 0 || start >= list->size || stop >= list->size) {
        return error_response("start or stop index out of bounds");
    }

    // trim the list
    int ret = list_trim(list, start, stop);
    if (ret) {
        return error_response("Failed to trim list");
    }

    if (!aof_restore) {
        handle_aof_write(cmd);
        return null_response();
    } else {
        return NULL;
    }
}

// returns an integer response representing the number of elements updated
char * lset_cmd(Command * cmd, bool aof_restore) {
    errno = 0; 

    ValueType response_type = INTEGER;
    int elem_updated = 0;

    if (cmd->num_args < 3) {
        return error_response("lset command requires at least 3 arguments (key, index, value)");
    }

    char * global_table_key = cmd->args[0];
    char * index_str = cmd->args[1];
    char * value = cmd->args[2];

    int index;

    // convert the index to an integer
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    index = (int) strtol(index_str, &endptr, 10);

    // check errors
    if (errno) {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == index_str)) {
        return error_response("Failed to convert index to integer");
    }

    // fetch the list from the global table
    HashNode * fetched_node = hget(global_table, global_table_key);

    if (!fetched_node) {
        return error_response("key not in database");
    }

    // check if the value is a list
    if (fetched_node->valueType != LIST) {
        return error_response("key is not for a list");
    }

    List * list = (List *) fetched_node->value;

    // check bounds
    if (index < 0 || index >= list->size) {
        return error_response("index out of bounds");
    }

    // set the value in the list
    int ret = list_imodify(list, index, value, STRING);
    if (ret) {
        return error_response("Failed to set value in list");
    }

    elem_updated++;

    if (!aof_restore) {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_updated);
    } else {
        return NULL;
    }
}


// zadd key score name
char * zadd_command(Command * cmd, bool aof_restore) {
    errno = 0;

    ValueType response_type = INTEGER;
    int elem_added = 0;

    if (cmd->num_args < 3) {
        return error_response("zadd command requires at least 3 arguments (key, score, name)");
    }

    char * zset_key = cmd->args[0];
    char * score_str = cmd->args[1];

    // convert the score to a float
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    float value = strtof(score_str, &endptr);

    // check errors
    if (errno) {
        perror("strtof failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to a float
    if (!(*endptr == '\0') || (endptr == score_str)) {
        return error_response("Failed to convert score to float");
    } 

    // fetch the zset from global table
    HashNode * fetched_node = hget(global_table, zset_key);

    if (!fetched_node) {
        // create a new zset, 
        ZSet * zset = zset_init();
        if (!zset) {
            fprintf(stderr, "Failed to create ZSet\n");
            exit(EXIT_FAILURE);
        }

        // create a new hash node
        HashNode * new_node = hinit(strdup(zset_key), ZSET, zset);
        if (!new_node) {
            fprintf(stderr, "Failed to create new hash node\n");
            exit(EXIT_FAILURE);
        }

        // insert the new node into the global table
        HashNode * ret = hinsert(global_table, new_node);
        if (!ret) {
            fprintf(stderr, "Failed to insert new node into global table\n");
            exit(EXIT_FAILURE);
        }

        fetched_node = new_node;
    }

    // check if the value is a ZSET
    if (fetched_node->valueType != ZSET) {
        return error_response("key is not for a ZSET");
    }

    ZSet * zset = (ZSet *) fetched_node->value;

    // add the value to the ZSET
    int ret = zset_add(zset, cmd->args[2], value);
    if (ret < 0) {
        return error_response("Failed to add value to ZSET");
    }
    elem_added++;

    if (!aof_restore) {
        handle_aof_write(cmd);
        return get_response(response_type, &elem_added);
    } else {
        return NULL;
    }
}

// zrem key name
char * zrem_command(Command * cmd, bool aof_restore) {
    errno = 0;

    ValueType response_type = INTEGER;
    int elem_removed = 0;
    
        if (cmd->num_args < 2) {
            return error_response("zrem command requires at least 2 arguments (key, name)");
        }

        char * zset_key = cmd->args[0];
        char * element_key = cmd->args[1];

        // fetch the zset from the global table
        HashNode * fetched_node = hget(global_table, zset_key);
        if (!fetched_node) {
            return error_response("zset key not in database");
        }

        // check if the value is a ZSET
        if (fetched_node->valueType != ZSET) {
            return error_response("key is not for a zset");
        }

        ZSet * zset = (ZSet *) fetched_node->value;

        // remove the value from the ZSET
        int ret = zset_remove(zset, element_key);
        if (ret < 0) {
            return error_response("Failed to remove value from zset");
        }       

        elem_removed++;


        if (!aof_restore) {
            handle_aof_write(cmd);
            return get_response(response_type, &elem_removed);
        } else {
            return NULL;
        }
}

// zscore key name
// 
char * zscore_cmd(Command * cmd) {
    errno = 0;

    ValueType response_type = FLOAT;
    float score;

    if (cmd->num_args < 2) {
        return error_response("zscore command requires at least 2 arguments (key, name)");
    }

    char * zset_key = cmd->args[0];
    char * element_key = cmd->args[1];

    // fetch the zset from the global table
    HashNode * fetched_node = hget(global_table, zset_key);
    if (!fetched_node) {
        return error_response("zset key not in database");
    }

    // check if the value is a ZSET
    if (fetched_node->valueType != ZSET) {
        return error_response("key is not for a zset");
    }

    ZSet * zset = (ZSet *) fetched_node->value;

    // search for the value in the ZSET
    HashNode * ret_node = zset_search_by_key(zset, element_key);
    if (!ret_node) {
        return error_response("Element not in zset");
    }

    // check if the value is a float
    if (ret_node->valueType != FLOAT) {
        fprintf(stderr, "Value in zset is somehow not a float\n");
        exit(EXIT_FAILURE);
    }

    score = *(float *) ret_node->value;

    return get_response(response_type, &score);
}


char * avl_iterate_response(AVLNode * tree, AVLNode * start, long limit) {
    int num_elements = 0;
    int inc_buffer = 5;

    // buffer for the data
    char * buffer = calloc(1 + 4 + MAX_MESSAGE_SIZE, sizeof(char));

    // iterate through the AVL tree and write the key and score to the buffer
    AVLNode * current = start;
    while (current != NULL && (num_elements/2 < limit) ) {
        // write the key to the buffer
        int type = SER_STR;
        int key_len = strlen((char *)current->scnd_index);

        // check if the buffer has enough space to write the key
        if (inc_buffer + 5 + key_len > MAX_MESSAGE_SIZE) {
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
        if (inc_buffer + 5 + score_len > MAX_MESSAGE_SIZE) {
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

// zquery key score name offset limit
char * zquery_cmd(Command * cmd) {
    errno = 0;

    if (cmd->num_args < 5) {
        return error_response("zquery command requires at least 5 arguments (key, score, name, offset, limit)");
    }

    char * zset_key = cmd->args[0];
    char * score_str = cmd->args[1];
    char * element_key = cmd->args[2];
    char * offset_str = cmd->args[3];
    char * limit_str = cmd->args[4];

    float score;
    int offset;
    int limit;

    // convert the score to a float
    // use strtol and duck typing to determine the type of the value
    char *endptr;
    score = strtof(score_str, &endptr);

    // check errors
    if (errno) {
        perror("strtof failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to a float
    if (!(*endptr == '\0') || (endptr == score_str)) {
        return error_response("Failed to convert score to float");
    }

    // convert the offset to an integer
    offset = (int) strtol(offset_str, &endptr, 10);

    // check errors
    if (errno) {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || (endptr == offset_str)) {
        return error_response("Failed to convert offset to integer");
    }

    // convert the limit to an integer
    limit = (int) strtol(limit_str, &endptr, 10);

    // check errors
    if (errno) {
        perror("strtol failed");
        exit(EXIT_FAILURE);
    }

    // Check if successfully converted to an integer
    if (!(*endptr == '\0') || !(endptr != limit_str)) {
        return error_response("Failed to convert limit to integer");
    }

    // fetch the zset from the global table
    HashNode * fetched_node = hget(global_table, zset_key);
    if (!fetched_node) {
        return error_response("zset key not in database");
    }

    // check if the value is a ZSET
    if (fetched_node->valueType != ZSET) {
        return error_response("key is not for a zset");
    }

    ZSet * zset = (ZSet *) fetched_node->value;

   
   if ((isinf(score) == -1) && (strcmp(element_key, "\"\"") == 0)){
        // "" was passed as the key and -inf was passed as the score, perform a rank query
        printf("Performing rank query\n");

        // find the element with smallest rank
        AVLNode * origin_node = get_min_node(zset->avl_tree);

        if (!origin_node) {
            return error_response("No valid elements in zset");
        }

        // offset the rank of the node in the AVL tree by the value specified by the offset parameter
        AVLNode * offset_node = avl_offset(origin_node, offset);

        return avl_iterate_response(zset->avl_tree, offset_node, limit);

    } else if (strcmp(element_key, "\"\"") == 0) {
        // ! potential problem dealing with equal scores here due to the fact that nodes with equal scores can be stored on the right and left based on AVL rotations
        // "" was passed as the key, perform a range query with score without name
        printf("Performing range query\n");


        // find the element in the ZSET using AVL tree
        AVLNode * origin_node = avl_search_float(zset->avl_tree, score);

        if (!origin_node) {
            return error_response("No valid elements in zset");
        }

        // offset the rank of the node in the AVL tree by the value specified by the offset parameter
        AVLNode * offset_node = avl_offset(origin_node, offset);

        return avl_iterate_response(zset->avl_tree, offset_node, limit);
    } else {
        // perform a query for the specific element

        // find the element in the ZSET using AVL tree
        AVLNode * origin_node = avl_search_pair(zset->avl_tree, element_key, score);

        if (!origin_node) {
            return error_response("Element not in zset");
        }

        // offset the rank of the node in the AVL tree by the value specified by the offset parameter
        AVLNode * offset_node = avl_offset(origin_node, offset);

        return avl_iterate_response(zset->avl_tree, offset_node, limit);
    }
        
}

// execute the command and return the server response string
char * execute_command(Command * cmd, bool aof_restore) {

    char * return_response;

    if (strcmp(cmd->name, "GET") == 0) {

        return_response = get_command(cmd);

    } else if (strcmp(cmd->name, "SET") == 0) {

        return_response = set_command(cmd, aof_restore);

    } else if (strcmp(cmd->name, "DEL") == 0) {

       return_response = del_command(cmd, aof_restore);
       
    } else if (strcmp(cmd->name, "KEYS") == 0) {

        return_response = keys_command();

    } else if (strcmp(cmd->name, "HSET") == 0) {
            
        return_response = hset_command(cmd, aof_restore);

    } else if (strcmp(cmd->name, "HGET") == 0) {

        return_response = hget_command(cmd);

    } else if (strcmp(cmd->name, "HDEL") == 0) {

        return_response = hdel_command(cmd, aof_restore);

    } else if (strcmp(cmd->name, "HGETALL") == 0) {

        return_response = hgetall_command(cmd);

    } else if (strcmp(cmd->name, "LPUSH")== 0) {
                
        return_response = lpush_command(cmd, aof_restore);

    } else if (strcmp(cmd->name, "RPUSH") == 0) {
            
        return_response = rpush_command(cmd, aof_restore);

    } else if (strcmp(cmd->name, "LPOP") == 0) {
    
        return_response = lpop_command(cmd, aof_restore); 
    }
    else if (strcmp(cmd->name, "RPOP") == 0) {

        return_response = rpop_cmd(cmd, aof_restore);

    } else if (strcmp(cmd->name, "LLEN") == 0) {

        return_response = llen_cmd(cmd);

    } else if (strcmp(cmd->name, "LRANGE") == 0) {

        return_response = lrange_cmd(cmd);

    } else if (strcmp(cmd->name, "LTRIM") == 0) {

        return_response = ltrim_cmd(cmd, aof_restore);

    } else if (strcmp(cmd->name, "LSET") == 0) {

        return_response = lset_cmd(cmd, aof_restore);

    }
    else if (strcmp(cmd->name, "ZADD")== 0) {
            
        return_response = zadd_command(cmd , aof_restore);

    } else if (strcmp(cmd->name, "ZREM") == 0) {
            
        return_response = zrem_command(cmd, aof_restore);

    } else if (strcmp(cmd->name, "ZSCORE") == 0) {

        return_response = zscore_cmd(cmd); 
    }
    else if (strcmp(cmd->name, "ZQUERY") == 0) {
        return_response = zquery_cmd(cmd);
    }
    else {
        return_response = error_response("Unknown command");
    }

    // free the command
    free(cmd->name);

    // free all args
    for (int i = 0; i < cmd->num_args; i++) {
        free(cmd->args[i]);
    }

    // free the command
    free(cmd);

    return return_response;
}


void aof_restore_db() {
    bool aof_restore = true;

    // check if the AOF was initialized
    if (!global_aof) {
        fprintf(stderr, "AOF not initialized\n");
        exit(EXIT_FAILURE);
    }

    // read the AOF file line by line
    char * line;
    while ((line = aof_read_line(global_aof)) != NULL) {
        // parse the command
        Command * cmd = parse_cmd_string(line, strlen(line));

        // execute the command
        execute_command(cmd, aof_restore);
    }

}

// response is a byte string that follows the protocol, returns the number of bytes written to the buffer
int buffer_write_response(char * buffer, char * response) {

    // write type and size of message
    memcpy(buffer, response, 1 + 4);

    int type = 0;
    memcpy(&type, buffer, 1);

    int message_size = 0;
    memcpy(&message_size, buffer + 1, 4);

    // for the type and size of the message
    int response_size = 1 + 4;

    if (type == SER_ARR) {
        for (int i = 0; i < message_size; i++) {
            int el_len = 0;
            memcpy(&el_len, response + response_size + 1, 4);

            // make sure the buffer has enough space to write the response
            if (response_size + 5 + el_len > MAX_MESSAGE_SIZE) {
                fprintf(stderr, "Failed to reallocate memory for response\n");
                exit(EXIT_FAILURE);
            }

            // write the response
            memcpy(buffer + response_size, response + response_size, 5 + el_len);

            response_size += 5 + el_len; 
        }

        return response_size;

    } else {
        // response without array
        response_size += message_size;

        // check if the buffer has enough space to write the response
        if (response_size > MAX_MESSAGE_SIZE) {
            fprintf(stderr, "Failed to reallocate memory for response\n");
            exit(EXIT_FAILURE);
        }   

        // write the response
        memcpy(buffer + 5, response + 5, message_size);

        return response_size;
    }

}



bool try_process_single_request(Conn * conn) {
    // check if the read buffer has enough data to process a request
    if (conn->current_read_size < 4) {
        // not enough data to process a request
        return false;
    }

    // copy the first 4 bytes of the read buffer, assume the message size is in little endian (this machine is little endian)
    int message_size = 0;
    memcpy(&message_size, conn->read_buffer, 4);

    if (message_size > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Message size too large\n");
        conn->state = STATE_DONE;
        return false;
    }

    // check if the read buffer has enough data to process a request
    if (conn->current_read_size < 4 + message_size) {
        // not enough data to process a request
        return false;
    }

    // print the message, account for the fact that the message is not null terminated due to pipe-lining
    printf("Client %d says: %.*s\n", conn->fd, message_size, conn->read_buffer + 4);

    // parse the message to extract the command
    Command * cmd = parse_cmd_string(conn->read_buffer + 4, message_size);

    // aof_restore is false, since the command is not being restored from the AOF file
    bool aof_restore = false;

    // execute the command, response is a null terminated byte string following the protocol
    char * response = execute_command(cmd, aof_restore);

    // write response to the write buffer
    int response_size = buffer_write_response(conn->write_buffer, response);

    // free the response
    free(response);

    conn->need_write_size = response_size;

    // remove the request from the read buffer
    int remaining_size = conn->current_read_size - (4 + message_size);

    // using memmove instead of memcpy to handle overlapping memory regions
    if (remaining_size) {
        memmove(conn->read_buffer, conn->read_buffer + 4 + message_size, remaining_size);
    }

    conn->current_read_size = remaining_size;

    // the request has been processed, move to the response state
    conn->state = STATE_RESP;
    state_resp(conn);

    // continue the outer loop to process pipelined requests
    return (conn->state == STATE_REQ);
}


bool try_fill_read_buffer(Conn * conn) {
    // check if the read buffer overflowed
    if (conn->current_read_size > sizeof(conn->read_buffer)) {
        fprintf(stderr, "Read buffer overflow\n");
        return false;
    }

    // read from the socket
    int read_size = 0;

    // attempt to read from the socket until we have read some characters or a signal has not interrupted the read
    do {
        int max_possible_read = sizeof(conn->read_buffer) - conn->current_read_size;

        read_size = read(conn->fd, conn->read_buffer + conn->current_read_size, max_possible_read);
    } while (read_size < 0 && errno == EINTR);

    if ((read_size < 0) && (errno = EAGAIN)) {
        //  read buffer is full, wait for the next poll event
        return false;
    }

    if (read_size < 0) {
        // an error that is not EINTR or EAGAIN occured
        perror("read failed");
        return false;
    }

    if (read_size == 0) {
        // if read_size is 0 then EOF reached 

        // if the current_read_size is greater than 0, then the EOF was reached before reading the full message
        if (conn->current_read_size > 0) {
            fprintf(stderr, "EOF reached before reading full message\n");
        } else {
            // EOF reached
            printf("EOF reached\n");
        }

        conn->state = STATE_DONE;
        return false;
    }

    conn -> current_read_size += read_size;
    if (conn->current_read_size > sizeof(conn->read_buffer)) {
        fprintf(stderr, "Read buffer overflow\n");
        return false;
    }

    // the loop here is to handle that clients can send multiple requests in one go (pipe-lining)
    while (try_process_single_request(conn)) {

    };

    return (conn->state == STATE_REQ);
}

bool try_flush_write_buffer(Conn * conn) {
    
    int write_size = 0;

    // attempt to write to the socket until we have written some characters or a signal has not interrupted the write
    do {
        int max_possible_write = conn->need_write_size - conn->current_write_size;

        write_size = write(conn->fd, conn->write_buffer + conn->current_write_size, max_possible_write); 
    } while (write_size < 0 && errno == EINTR);

    if (write_size < 0) {
        if (errno == EAGAIN) {
            // write buffer is full, wait for the next poll event
            return false;
        }
        // an error that is not EINTR or EAGAIN occured, exit the connection
        perror("write failed");
        conn->state = STATE_DONE;
        return false;
    }

    conn->current_write_size += write_size;

    if (conn->current_write_size > conn->need_write_size) {
        fprintf(stderr, "Write buffer overflow\n");
        conn->state = STATE_DONE;
        return false;
    };

    if (conn->current_write_size == conn->need_write_size) {
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

void state_req(Conn * conn) {
    while(try_fill_read_buffer(conn)) {

    };
}


void state_resp(Conn * conn) {
    while (try_flush_write_buffer(conn)) {

    };
}


// build a TCP server that listens on port 
int main (int argc, char * argv []) {

    // Initialize global structures
    global_table = hcreate(INIT_TABLE_SIZE);
    global_aof = aof_init(AOF_FILE, FLUSH_INTERVAL_SEC, "r");

    // restore state of database from AOF file
    aof_restore_db();

    // change the aof back to append mode, so that new commands are appended to the file
    aof_change_mode(global_aof, "a");

    // create the socket
    int server_socket= socket(AF_INET, SOCK_STREAM, 0);
    check_error(server_socket);
    
    // Set the socket options to allow address reuse, only for debugging
    int optval = 1;
    check_error(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)));

    // define the server address
    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = ntohs(SERVERPORT);

    // allow the server to listen to all network interfaces
    server_address.sin_addr.s_addr = ntohl(INADDR_ANY);


    // bind the socket to the specified IP and port
    check_error(bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)));

    // listen for incoming connections, allow maximum number of connections allowed by the OS
    check_error(listen(server_socket, SOMAXCONN));

    // a map of all client connections, keyed by fd(Use array to avoid hash table for simplicity)
    Conn * fd2conn[MAX_CLIENTS] = {0};

    // set the server socket to non-blocking
    set_fd_nonblocking(server_socket);

    // set up the poll arguments
    struct pollfd poll_args[MAX_CLIENTS + 1];
    memset(poll_args, 0, sizeof(poll_args));

    printf("Server listening on port %d\n", SERVERPORT);
    
    // the event loop, note: there is only on server socket responsible for interating with other client fd's
    while (1) {
        // prepare the arguments of the poll(), the first argument is the server socket, the events specifies that we are interested in reading from the server socket, 
        poll_args[0].fd = server_socket;
        poll_args[0].events = POLLIN;
        poll_args[0].revents = 0;

        // handle the client connections
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!fd2conn[i]) {
                continue;
            }

            // set the poll arguments for the client fd
            poll_args[i+1].fd = fd2conn[i]->fd;

            // set the events to read from the client fd
            poll_args[i+1].events = fd2conn[i]->state == STATE_REQ ? POLLIN : POLLOUT;

            poll_args[i+1].revents = 0;

            // make sure thhe OS listens to errors on the client fd
            poll_args[i].events |= POLLERR;
        }

        // call poll() to wait for events
        int ret = poll(poll_args, MAX_CLIENTS + 1, 1000);

        if (ret < 0) {
            perror("poll failed");
            exit(1);
        }

        // process ready client connections
        for (int i = 1; i < MAX_CLIENTS + 1; i++) {
            if (poll_args[i].revents == 0) {
                // no events on this fd, not interested
                continue;
            }

            Conn * conn = fd2conn[i-1];
            
            connection_io(conn);


            if (conn->state == STATE_DONE) {
                // close the connection
                close(conn->fd);
                free(conn);
                fd2conn[i-1] = 0;
                memset(&poll_args[i], 0, sizeof(poll_args[i]));
            }
        }

        // try to accept new connections if server socket is ready
        if (poll_args[0].revents) {
           accept_new_connection(fd2conn, server_socket);
        }

    }

    return 0;
}


