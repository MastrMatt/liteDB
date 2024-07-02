# LiteDB

-This is an in memory database that is run on a server, it can be used as a cache and even a complete database after persistence has been implemented. The inspiration for this project was redis, attempting to build from scratch, even data structure like AVL trees and hash tables

-Server is ran on a little endian machine

-Communication protocol:
the len of the message is a 4 byte little endian ordered integer, followed by the message
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------

-Note: the client sends strings that are not null terminated due to pipelining


