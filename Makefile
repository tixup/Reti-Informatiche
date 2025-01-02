all: client server

server: server.c
	gcc -Wall -o server server.c utility.c questions.c
    
client: client.c
	gcc -Wall -o client client.c utility.c