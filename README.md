# LiteDB

-This is an in memory database that is run on a server, it can be used as a cache and even a complete database after persistence has been implemented. The inspiration for this project was redis, attempting to build from scratch, even data structure like hash tables and AVL trees

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

-ZSET will be storing integers

-*** Whoever allocates memory should be responsible for freeing it ***




