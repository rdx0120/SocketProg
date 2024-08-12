# Makefile for MP3

output: server.o
	gcc server.o -o server
#	gcc client.o -o client

#client.o: client.c
#	gcc -c client.c

server.o: server.c
	gcc -c server.c

clean:
	rm -f server *.o core
#	rm -f client server *.o core
