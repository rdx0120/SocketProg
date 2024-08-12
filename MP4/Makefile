# Makefile for MP4

output: client.o cache_file.o server.o 
	gcc client.o -o client
	gcc cache_file.o server.o -o server

client.o: client.c
	gcc -c client.c

server.o: server.c cache_file.h
	gcc -c server.c

cache.o: cache_file.c cache_file.h
	gcc -c cache_file.c

clean:
	rm -f client server *.o core
