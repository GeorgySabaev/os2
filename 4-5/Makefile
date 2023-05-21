all: os2_client os2_server
os2_client: os2_client.c DieWithError.c letterstruct.h
	gcc os2_client.c DieWithError.c -o os2_client
os2_server: os2_server.c DieWithError.c letterstruct.h
	gcc os2_server.c DieWithError.c -o os2_server