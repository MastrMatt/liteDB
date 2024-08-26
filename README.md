# liteDB: A Lightweight In-Memory Database Inspired by Redis

liteDB is a lightweight, in-memory database inspired by Redis. It combines caching with the capabilities of a full-fledged database, featuring Append Only File (AOF) persistence for data durability.

## Getting Started

If you have docker installed on your machine, it is easy to use liteDB:

To spin up a server:

```bash

docker run -p 9255:9255 -it mastrmatt/litedb:latest

```

and a cli client:

```bash

docker run --init --network=host -it mastrmatt/litedb-cli:latest

```

Else, To begin using liteDB, follow these simple steps:

1. Ensure you have a unix environment with gcc installed

2. Clone the repository.

First, you need to obtain the source code of liteDB. You can do this by cloning the GitHub repository. Open your terminal or command prompt and execute the following commands:

```
   git clone https://github.com/MastrMatt/LiteDB.git
   cd LiteDB
```

3. Compile and run the server in a terminal window

Before running the server, ensure you have all the necessary tools installed, such as gcc for compiling C programs. Then, compile and run the server by executing the following commands in a new terminal window:

```
   cd server
   make all
   ./runserver
```

4. Compile and run the client in another terminal window

Similarly, to interact with the liteDB server, you need to compile and run the client. Make sure you're in the root directory of the liteDB project (liteDB). Then, in another terminal window, execute:

```
   cd client
   make all
   ./runclient
```

## Usage

Basic example of the client

```bash
./runclient
Connecting to server at: (127.0.0.1:9255)
liteDB> set g 4
(nil)
liteDB> get g
(str) 4
liteDB> hset hash field value
(int) 1
liteDB> keys
(arr) len = 2
(str) g
(str) hash
(arr) end
liteDB> del g
(int) 1
liteDB> waiting for command ...

```

## Key Features

-   **In-Memory Storage**: Offers rapid access to data with the option for persistence through AOF.
-   **Custom Data Structures**: Implements its own versions of hash tables and AVL trees for flexibility
-   **Single-threaded Event Loop**: Similar to Redis, LiteDB operates a single-threaded event loop with IO multiplexing for handling requests, minimizing thread creation overhead and improving performance
-   **Multithreading for Persistence**: Utilizes multithreading to flush the AOF buffer to disk, guaranteeing data durability without impacting main thread performance.
-   **Command Pipelining**: Supports pipelined commands from clients for batch processing and efficiency.
-   **TCP Server Architecture**: Operates as a TCP server

## Database Structure

-   All data in liteDB are stored as strings, except for the ZSET values which are stored as floats

## Communication Protocol

### Client Request Format

Clients communicate with the liteDB server using a straightforward protocol:

```
len(4 bytes): little endian integer representing the length of the msg
msg: the cmd string to be executed, args separated by spaces
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------
```

### Server Response Format

The server responds with messages formatted as follows:

```
type(1 byte): null,err,string,int,float,arr
len(4 bytes): little endian integer representing the length of the msg
msg: response msg

+-----+------+---+
type | len | msg |
+-----+------+---+
```

Array responses have a slightly different format to accommodate multiple elements:

```
type(1 byte): arr
len_array: length of the array

+-----+------+-----+------+------+------+------+------+-----+---------+-----
type | len_array | type_1 | len_1| element_1| type-2| len_2 | element_2|...
+-----+------+-----+------+------+------+------+------+-----+---------+-----
```

## Supported Commands

liteDB supports a variety of commands across different data structures:

### Meta Commands

-   EXISTS: (key) - Checks if the specified key exists in the database. Returns 1 if it does else, 0.
-   DEL: (key) - Deletes the value specified by key. Returns the amount of keys deleted
-   KEYS - Returns all the key:value pairs in the database
-   FLUSHALL - Removes all the key:value pairs in the database. Returns nil

### Strings

-   GET: (key) - Get the value of a key, it the key does not exist return an error. Returns the value
-   SET: (key, value) - Sets a new key:value pair in the hashtable, it the key already exists returns an error. Returns nil

### Hashtable

-   HEXISTS: (key, field) - checks if a field exists in a hash specified by key. Returns an integer response indicating the number of fields found.
-   HSET: (key, field, value) - Sets a field:value pair in the hash specified by key. If the key does not exist, it will create it. It the field already exists, it overrides the previous value. Returns nil
-   HGET: (key, field) - Gets the value of field from the hash specified by key. Returns the value
-   HDEL: (key, field) - Deletes a field from the hash specified by key. Returns an integer for how many elements were removed
-   HGETALL: (key) - Returns all fields and values of the hash specified by key.

### Lists

-   LPUSH, RPUSH: (key, value) - Adds value to the list specified by key. If key does not exist, a new list is created. Returns an integer for how many elements were added
-   LPOP, RPOP: (key, value) - Removes and returns the corresponding element of the list specified by key. Returns an integer for how many elements were removed
-   LLEN: (key) - Returns the length of the list specified by key
-   LRANGE: (key, start, stop) - Returns values from index start up to and including index stop from the list. The list is specified by key.
-   LTRIM: (key, start, stop) - Trims a list from index start up to and including index stop. The list is specified by key. Returns nil
-   LSET: (key, index, value) - Sets the index of the list to contain value. The list is specified by the key. Returns nil

### Sorted Sets

-   ZADD: (key, score, name) - Adds (score, name) to the set specified by key. If the key does not exist, it is created. If (score, name) already exists , it is updated. Returns the number of elements inserted or updated.

-   ZREM: (key, name) - Removes the element from the sorted set with the specified name. The sorted set is specified by key. Returns the number of elements removed.

-   ZSCORE: (key, name) - Returns the score of the element with the specified name from the sorted set specified by key.

-   ZQUERY: (key score name offset limit) -
    General query command meant to combine various typical Redis sorted cmds into one.
    ZrangeByScore: ZQUERY with (key score "" offset limit),
    Zrange by rank: ZQUERY with (key -inf "" offset limit)

## Planned Features

-   Client connection timers for idle detection and disconnection.
-   Time-to-live (TTL) for data in the global hashtable for caching purposes.
-   Automatic rewriting of the AOF file when it exceeds a certain size threshold.

## Author

Matthew Neba / [@MastrMatt](https://github.com/MastrMatt)

## License

This project is licensed under the [MIT License](LICENSE).

## Main Sources

-   [Redis](https://redis.io/)
-   [Build Your Own Redis](https://build-your-own.org/redis/#table-of-contents)
-   [Redis Persistence Demystified](http://oldblog.antirez.com/post/redis-persistence-demystified.html)

## Contributing

Contributions to this project are welcome Please submit pull requests or open issues to discuss potential improvements or report bugs.
