# Multi-Client Chat Application in C

A real-time multi-client chat application built in C using TCP sockets and POSIX threads (pthreads). The application allows multiple users to connect simultaneously and communicate in real-time with additional features like user management and special commands.

## Features

### 🚀 Core Functionality
- **Multi-client support**: Up to 10 concurrent users
- **Real-time messaging**: Instant message broadcasting to all connected users
- **Multi-threaded architecture**: Non-blocking message handling using pthreads
- **Username-based identification**: Each user has a unique username

### 🎯 Advanced Features
- **Online users command**: Type `:online` to see all connected users
- **Graceful disconnection**: Type `:end` to exit cleanly
- **Timestamped messages**: All messages include HH:MM:SS timestamps
- **Color-coded output**: Different colors for usernames, timestamps, and status messages
- **Client status tracking**: Server monitors active/inactive clients

### 🛡️ Technical Features
- **Socket reuse**: `SO_REUSEADDR` option for better server restarts
- **Proper memory management**: Dynamic allocation with cleanup
- **Error handling**: Comprehensive error checking and reporting
- **Message parsing**: Handles multiple messages in single TCP stream
- **Thread safety**: Safe concurrent operations

## Requirements

- GCC compiler
- POSIX-compliant system (Linux, macOS, Unix)
- pthread library support

## Installation & Compilation

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
The server will start listening on port 2000 and display connection status.

### Connecting Clients
```bash
./client
```
1. Enter your username when prompted
2. Start chatting!

### Chat Commands
- `:online` - Display list of all connected users
- `:end` - Exit the chat application
- Regular messages - Just type and press Enter

## Example Session

**Server Output:**
```
Server listening on port 2000...
Connection succeeded
Client connected from 127.0.0.1:45678
Alice has connected
[14:30:15] Alice: Hello everyone!
=== ONLINE COMMAND DETECTED ===
User Bob requested online users list
```

**Client Output:**
```
Connected to server at 127.0.0.1:2000
Enter your username: Alice
You: Hello everyone!
[14:30:20] Bob: Hi Alice!
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
- **Main thread**: Accepts new client connections
- **Worker threads**: Handle individual client communication (one per client)
- **Client management**: Maintains list of active clients with usernames
- **Message broadcasting**: Forwards messages to all connected clients except sender

### Client (`client.c`)
- **Main thread**: Handles user input and sending messages
- **Receiver thread**: Listens for incoming messages from server
- **Non-blocking I/O**: User can type while receiving messages

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
Messages are formatted with timestamps and usernames:
```
[HH:MM:SS] Username: Message content
```

## Contributing

Feel free to fork this project and submit pull requests for improvements or bug fixes.

## License

This project is open source and available under the MIT License.

## Author

Built with ❤️ in C
