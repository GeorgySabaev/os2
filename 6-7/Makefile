all: client server viewer
client: client.c DieWithError.c protocol.h protocol.c
	gcc client.c DieWithError.c protocol.c -o client
viewer: viewer.c DieWithError.c protocol.h protocol.c
	gcc viewer.c DieWithError.c protocol.c -o viewer
server: server.c DieWithError.c protocol.h protocol.c
	gcc server.c DieWithError.c protocol.c -o server