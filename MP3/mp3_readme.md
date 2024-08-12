This is the readme file for ECEN602 - MP3.


Roles:
Rohan Dalvi - write the code for server.c.
Donguk Kim - run the test cases, debug server.c and modify the code.


Architecture of the code:
The program only contains the tftp server code, not the client side. We used the Hera server and the tftp client that is installed in the environment. The server is able to process RRQ requests from multiple clients. Size of each block is 512Bytes, and if the file that we want to send is larger than 512 Bytes, the file gets separated into multiple blocks and gets sent to the client. Also, for the bonus feature, the server is able to process WRQ requests from clients. The file gets saved on the server with the name "WRQ_data.txt".


Usage:
1. Goto the MP3 folder, Build using make command.
> make
2. In one terminal, start the server by
> ./server 127.0.0.1 2222 (arguments: IP address, port)
3. Open another terminal, start a client by
> tftp 127.0.0.1 2222 (arguments: IP address, port)
4. Open another terminal, start the second client by
> tftp 127.0.0.1 2222 (arguments: IP address, port)
5. Open another terminal, start the third client by
> tftp 127.0.0.1 2222 (arguments: IP address, port)
6. To read data from the server, type the following in the client terminal
> get data_in_server.txt
7. To write data to the server, type the following in the client terminal
> put data_in_client.txt
8. To change modes,
use the following command
> mode netascii
or
> mode binary
