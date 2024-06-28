# LiteDB

-Server is ran on a little endian machine

-Communication protocol:
the len of the message is a 4 byte little endian ordered integer, followed by the message
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------



