# Multi-Client Chat Application in C

A real-time multi-client chat application built in C using TCP sockets and POSIX threads. The application allows multiple users to connect simultaneously and chat in real-time, with user tracking, special commands, and Lamport clock based message ordering.

## Features

### Core Functionality
- Up to 10 concurrent users
- Instant message broadcasting to all connected users
- Non-blocking message handling using pthreads
- Username-based identification via a fixed-size handshake frame
- Lamport clocks for a consistent total order of events across clients

### Concurrency Safety
- Shared client list protected by mutex locks
- Copy-then-send pattern so blocking I/O never runs while holding a lock
- Detached worker threads with clean per-client cleanup

### Commands and Formatting
- Type `:online` to see all connected users
- Type `:end` to exit cleanly
- All messages include a Lamport clock value `[LC:n]` and an HH:MM:SS timestamp
- Color-coded output for usernames, timestamps, and status messages

### Technical Features
- `SO_REUSEADDR` option so you don't get blocked on a server restart
- `read_full` and `write_full` helpers that retry until all bytes are transferred
- `die` helper for consistent fatal error handling
- Newline-delimited message framing to handle multiple messages in a single TCP stream
- `SIGPIPE` ignored so a dead peer returns an error instead of killing the process
- Clean shutdown via `shutdown(SHUT_WR)` and `pthread_join`

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

2. Build the server and client:
```bash
make
```

3. Remove the binaries (optional):
```bash
make clean
```

## Usage

### Starting the Server
```bash
./server
```
The server will start listening on port 8080 and print connection updates.

### Connecting Clients
```bash
./client
```
1. Enter your username when prompted
2. Start chatting

### Chat Commands
- `:online` shows a list of everyone online
- `:end` exits the chat application
- Regular messages are just text followed by Enter

## Testing

Run the integration test suite:
```bash
./test.sh
```

The suite covers message broadcast between clients, the `:online` command, clean `:end` disconnects, Lamport clock monotonicity, and raw socket clients that send messages without a Lamport prefix.

## Architecture

### Server (`server.c`)
- **Main thread**: listens for and accepts new client connections
- **Worker threads**: one thread per client, handling receive, broadcast, and commands
- **Client management**: a mutex-protected list tracks active clients and their names
- **Message broadcasting**: forwards incoming messages to every user except the sender
- **Lamport clock**: `server_clock` advances to `max(incoming, local) + 1` on each message

### Client (`client.c`)
- **Main thread**: reads what you type, tags it with a Lamport value, and sends it out
- **Receiver thread**: waits for incoming messages and updates the local Lamport clock
- **Non-blocking input**: keeps the prompt open so you can type while text is rolling in

### Helpers (`helpers.c` / `helpers.h`)
- `die`: prints the error and exits on fatal failures
- `read_full`: loops `read` until exactly `n` bytes arrive, retrying on `EINTR`
- `write_full`: loops `write` until all `n` bytes are sent
- `MAX`: safe maximum macro used by the Lamport clock logic

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
- **Port**: 8080
- **Address**: 127.0.0.1 (localhost)
- **Max clients**: 10 concurrent connections

### Message Format
```
[LC:2] [HH:MM:SS] Username: Message content
```

Clients send messages as `lamport_clock|message\n`, and the server broadcasts them with a `[LC:n]` prefix.

## Contributing

Feel free to fork this project and submit pull requests for improvements or bug fixes.

## License

This project is open source and available under the MIT License.
