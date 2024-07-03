# LiteDB

-This is an in memory database that is run on a server, it can be used as a cache and even a complete database after persistence has been implemented. The inspiration for this project was redis, attempting to build from scratch, even data structure like hash tables and AVL trees

-Server is ran on a little endian machine


-Client will send a command of the regular communication protocol:
len(4 bytes): little endian integer representing the length of the msg
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------


-Server will respond with a message of format:
type(1 byte): null,err,int,string,arr
len(4 bytes): little endian integer representing the length of the msg
data: reponse data

+-----+------+-----+
type | len | data
+-----+------+-----+

-database stores long values




