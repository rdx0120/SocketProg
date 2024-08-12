This is the readme file for ECEN602 - MP1.

Roles:
Rohan Dalvi - write the code for server.c.
Donguk Kim - write the code for client.c and run test cases.

Architecture of the code:
The program consists of two main parts, the client and the server. The server and the client each have its own main function. There are several assumptions between the client and the server. First, they should use TCP protocol. Second, the length of the line which is being sent must be defined in the same length. Here, we defined the length as 10.

Usage:
1. Goto the MP1 folder, Build using make command.
> make
2. In one terminal, start the server
> ./server 3490
3. Open another terminal,  start the client
> ./client 127.0.0.1 3490
4. Input string and press enter on the client terminal.
> Howdy, World!!!
5. (Optional) open another terminal, start another client.
> ./client 127.0.0.1 3490
