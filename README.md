# LiteDB: A Lightweight In-Memory Database Inspired by Redis

LiteDB is a lightweight, in-memory database inspired by Redis. It combines high-speed caching with the capabilities of a full-fledged database, featuring Append Only File (AOF) persistence for data durability.It is built from the ground up, including custom implementations of data structures such as hash tables and AVL trees, ensuring flexibility.

## Key Features

- **In-Memory Storage**: Offers rapid access to data with the option for persistence through AOF.
- **Custom Data Structures**: Implements its own versions of hash tables and AVL trees for optimized operations.
- **Single-threaded Event Loop**: Similar to Redis, LiteDB operates a single-threaded event loop for handling requests, ensuring simplicity and predictability.
- **Multithreading for Persistence**: Utilizes multithreading to flush the AOF buffer to disk, guaranteeing data durability without impacting main thread performance.
- **Command Pipelining**: Supports pipelined commands from clients for batch processing and efficiency.
- **Flexible Data Types**: Stores most input values as strings, with support for floats in sorted sets (ZSETs).
- **TCP Server Architecture**: Operates as a TCP server
- **Little Endian Compatibility**: Designed to run on little endian machines for broad hardware compatibility.

## Communication Protocol

### Client Request Format

Clients communicate with the LiteDB server using a straightforward protocol:

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

LiteDB supports a variety of commands across different data structures:

### Meta Commands
- DEL
- KEYS
- FLUSHALL

### Strings
- GET
- SET

### Hashtable
- HSET
- HGET
- HDEL
- HGETALL  

### Lists
- LPUSH, RPUSH
- LPOP, RPOP
- LLEN
- LRANGE
- LTRIM
- LSET

### Sorted Sets
- ZADD
- ZREM
- ZSCORE
- ZQUERY:  ZQUERY key score name offset limit. 
This command is not present in Redis, it is a general query command meant to combine various Redis Zset cmds into one.
ZrangeByScore: ZQUERY with (score , ""), 
Zrange by rank: ZQUERY with (-inf, "")

## Getting Started

To begin using LiteDB, follow these simple steps:

1. Clone the repository.
```
   git clone https://github.com/MastrMatt/AITradingBot.git
   cd lumibot-sentiment-strategy
```

2. Run `make all` to compile the source code.
3. Start the server with `./server`.
4. Connect to the server using the client executable `./client`.

## Planned Features

- Client connection timers for idle detection and disconnection.
- Time-to-live (TTL) for data in the global hashtable for caching purposes.
- Automatic rewriting of the AOF file when it exceeds a certain size threshold.


## Author
Matthew Neba / [@MastrMatt](https://github.com/MastrMatt)

## License
This project is licensed under the [MIT License](LICENSE).


## Main Sources

- [Redis](https://redis.io/)
- [Build Your Own Redis](https://build-your-own.org/redis/#table-of-contents)
- [Redis Persistence Demystified](http://oldblog.antirez.com/post/redis-persistence-demystified.html)

## Contributing

Contributions to this project are welcome Please submit pull requests or open issues to discuss potential improvements or report bugs.
