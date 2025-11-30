Come visit the project at https://www.udemy.com/course/networks-in-c-from-sockets-to-a-multiplexed-chat-server/

This project implements a **TCP server in C**, built entirely using low-level APIs from the operating system, including:

- POSIX/BSD sockets (`AF_INET`, `SOCK_STREAM`)
- TCP protocol obtained via `getprotobyname()`
- Manual client session management
- I/O multiplexing with `poll()`
- Per-client message buffers
- Broadcast of messages between connected clients

The server is part of a teaching project in udemy, demonstrating how real-world network servers work from scratch.  

This server:
- Accepts multiple simultaneous client connections
- Uses `poll()` for scalable multiplexing
- Tracks each connected client individually
- Assigns unique IDs to each client
- Detects and handles client disconnections
- Broadcasts messages to all other clients
- Maintains per-client buffers for partial reads
- Detects newline (`\n`) to determine full messages
- Sends formatted messages and exchange data mong the clients

How It Works:
1. New connection
accept() returns a new socket
A unique ID is assigned
A welcome message is sent

2. Receiving data
Every chunk received is appended to the client's message buffer.
When a newline (\n) appears, the message is treated as complete.

3. Broadcasting messages
A formatted message is sent to all other connected clients:
Client <ID> said: <line>\n

4. Disconnection detection
If recv() returns 0:
The socket is closed
The client is removed
A departure message is sent to remaining clients

Compile using:
gcc server.c -o server

Running the Server:
./server

The server runs on:
127.0.0.1:49500

Connect using telnet:
telnet 127.0.0.1 49500

License:
Project licensed under the MIT License.
Feel free to study, modify, and share.
