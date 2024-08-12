This is the readme file for ECEN602 - MP4.


Roles:
Donguk Kim - write the code for server.c.
Rohan Dalvi - write the code for client.c.
            - run the test cases, debug and modify the codes.


Architecture of the code:
The program contains server.c, client.c, cache.h, and cache.c. The server.c file is the proxy server that caches upto 10 file requests from clients. The client code contains code that parses the commands and send http requests. The cache.c and cache.h files contain data structure for the LRU cache which the proxy server uses to keep track of the http requested files. 


Usage:
1. Goto the MP4 folder, Build using make command.
> make
2. In one terminal, start the server by
> ./server 127.0.0.1 8000 (arguments: IP address, port)
3. Open another terminal, start a client by
> ./client 127.0.0.1 8000 www.evanjones.ca/crc32c.html (arguments: IP address, port, url)
4. Open another terminal, start the second client by
> ./client 127.0.0.1 8000 <url2> (arguments: IP address, port, url)
> ./client 127.0.0.1 8000 <url3> (arguments: IP address, port, url)
> ./client 127.0.0.1 8000 <url4> (arguments: IP address, port, url)
5. check the folder to see if the files are saved.
> cat crc32c.html

