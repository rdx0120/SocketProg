This is the readme file for ECEN602 - MP2.

Roles:
Rohan Dalvi - write the code for server.c and run test cases.
Donguk Kim - write the code for client.c, debugging.

Architecture of the code:
The program consists of two main parts, the client and the server. The server and the client each have its own main function. There are several assumptions between the client and the server. The Server hosts a chat service, and multiple clients can join the chat with JOIN request, SEND messages, and receive the messages that other clients sent which is broadcasted by the server with FWD message.

Usage:
1. Goto the MP2 folder, Build using make command.
> make
2. In one terminal, start the server by
> ./server 127.0.0.1 2222 4 (arguments: IP address, port, max number of clients)
3. Open another terminal,  start the client by
> ./client user1 127.0.0.1 2222 (arguments: username, IP address, port)
3. Open another terminal,  start the client using different username.
> ./client user2 127.0.0.1 2222 (arguments: username, IP address, port)
4. Input string and press enter on the client terminal.
> Howdy, World!!!
5. To leave the chat, go to the client terminal and press ctrl+c.
> ./client 127.0.0.1 2222
