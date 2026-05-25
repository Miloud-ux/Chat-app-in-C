# Multi-Client Chat Application in C

A real-time multi-client chat application built in C using TCP sockets and POSIX threads. The application allows multiple users to connect simultaneously and chat in real-time, with additional features like user tracking and special commands.

## Features

### Core Functionality
- Up to 10 concurrent users
- Instant message broadcasting to all connected users
- Non-blocking message handling using pthreads
- Username-based identification

### Commands and Formatting
- Type `:online` to see all connected users
- Type `:end` to exit cleanly
- All messages include HH:MM:SS timestamps
- Color-coded output for usernames, timestamps, and status messages
- Server monitors active and inactive clients

### Technical Features
- `SO_REUSEADDR` option so you don't get blocked on a server restart
- Dynamic memory allocation with cleanup
- Message parsing to handle multiple messages in a single TCP stream

## Requirements

- GCC compiler
- Linux, macOS, or any POSIX-compliant system
- pthread library support

## Installation and Compilation

1. Clone the repository:
```bash
git clone <repository-url>
cd chat-application
```

2. Compile the server and client:
```bash
gcc -o server server.c -lpthread
gcc -o client client.c -lpthread
```

## Usage

### Starting the Server
```bash
./server
```
The server will start listening on port 2000 and print connection updates.

### Connecting Clients
```bash
./client
```
1. Enter your username when prompted
2. Start chatting

### Chat Commands
- `:online` - shows a list of everyone online
- `:end` - exits the chat application
- Regular messages - just type your text and hit enter

## Example Session

**Server Output:**
```
Server listening on port 2000...
Connection succeeded
Client connected from 127.0.0.1:45678
Alice has connected Alice: Hello everyone!
=== ONLINE COMMAND DETECTED ===
User Bob requested online users list
```

**Client Output:**
```
Connected to server at 127.0.0.1:2000
Enter your username: Alice
You: Hello everyone! Bob: Hi Alice!
You: :online

=== Online Users ===
1. Alice
2. Bob
==================
Total: 2 users online

You: 
```

## Architecture

### Server (`server.c`)
- **Main thread**: listens for and accepts new client connections
- **Worker threads**: handles individual client communication, spinning up one thread per client
- **Client management**: keeps track of active clients and their names
- **Message broadcasting**: forwards incoming messages to every user except the sender

### Client (`client.c`)
- **Main thread**: reads what you type and sends it out
- **Receiver thread**: waits around for incoming messages from the server
- **Non-blocking input**: keeps the prompt open so you can type while text is rolling in

## Technical Details

### Data Structures
```c
typedef struct {
    int client_socket;
    char username[50];
    bool is_active;
} client;
```

### Network Configuration
- **Protocol**: TCP (SOCK_STREAM)
- **Port**: 2000
- **Address**: 127.0.0.1 (localhost)
- **Max clients**: 10 concurrent connections

### Message Format
```
[HH:MM:SS] Username: Message content
```

## Contributing

Feel free to fork this project and submit pull requests for improvements or bug fixes.

## License

This project is open source and available under the MIT License.

