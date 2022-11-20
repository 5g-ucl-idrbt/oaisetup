all:
	make server
	make client
server:
	gcc sctptest.c -o server -lsctp -Wall
client:
	gcc sctptest.c -o client -lsctp -Wall
