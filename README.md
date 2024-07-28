# LiteDB

-This is an in memory database that is run on a server, it can be used as a cache and even a complete database after persistence has been implemented. The inspiration for this project was redis, attempting to build everything from scratch, even data structure like hash tables and AVL trees

-Server is ran on a little endian machine

-Client will send a command of the regular communication protocol:
len(4 bytes): little endian integer representing the length of the msg
msg: the cmd string to be executed, args seperated by spaces
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------


-Server will respond with a message of format:
type(1 byte): null,err,string,arr
len(4 bytes): little endian integer representing the length of the msg
msg: reponse msg

+-----+------+-----+
type | len | msg
+-----+------+-----+

for arrays: 
+-----+------+-----+------+------+------+------+------+-----+---------+
type | len_array | type_1 | len_1| element_1| type-2| len_2| element_2
+-----+------+-----+------+------+------+------+------+-----+---------+

-LiteDB will store all values as strings except in the ZSET will be storing floats

-*** Whoever allocates memory should be responsible for freeing it ***


Commands currently suported:

Dealing with elements in the main hashtable:

Meta:
-DEL
-KEYS

Strings:
-GET
-SET

Hashtable -> only stores strings:
-HSET
-HGET
-HDEL
-HGETALL  

Lists:
-LPUSH, RPUSH
-LPOP, RPOP
-LLEN
-LRANGE
-LTRIM
-LSET

Sorted Sets -> only stores floats, has a key for each element:
-ZADD
-ZREM
-ZSCORE
-ZQUERY: includes zrangebyscore and zrank


Features to add:
-Timers for the client connections to determine if idle and kickout
-Timer for the data stored in global hashtable (TTL for caching)
-AOF rewriting when the aof file becomes too large