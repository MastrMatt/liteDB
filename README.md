# LiteDB

-This is an in memory database that is run on a server, it can be used as a cache and even a complete database after persistence has been implemented.The inspiration for this project was redis

-Server is ran on a little endian machine

-Communication protocol:
the len of the message is a 4 byte little endian ordered integer, followed by the message
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------



