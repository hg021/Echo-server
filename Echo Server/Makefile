default: server client

server.o: echo_server.c
	gcc -c echo_server.c -o server.o
server: server.o
	gcc server.o -o server

client.o: client_20.c
	gcc -c client_20.c -o client.o

client: client.o
	gcc client.o -o client

clean:
	-rm -f server.o
	-rm -f server
	-rm -f client.o
	-rm -f client
