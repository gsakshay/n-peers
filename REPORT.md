# Peer Coordination Program

## Overview

This program implements a peer-to-peer communication system over UDP sockets where multiple peers send heartbeats to each other and acknowledge receipt. Once all peers have successfully exchanged heartbeats and acknowledgments, they print "READY".

## Key Components

1. Peer Structure
2. Socket Creation and Binding
3. Address Resolution
4. Message Sending and Receiving
5. Timeout and Interval Management
6. Logging and Debugging

## Main Algorithm

The program uses a combination of UDP socket programming and the `select()` event to handle multiple peer communications efficiently. Here's a breakdown of the main algorithm:

1. Initialize peers from the hosts file.
2. Resolve peer addresses.
3. Create and bind a UDP socket.
4. Enter the main loop:
   a. Use `select()` to wait for incoming messages or timeout.
   b. Send heartbeats to peers that haven't acknowledged.
   c. Process incoming messages (heartbeats or acknowledgments).
   d. Check if all communications are complete.
   e. If complete, print "READY" and continue listening until the total timeout. Further we can build on top of it, the main distributed logic.

## Key Functions and Their Uses

1. `select()`: Used for I/O multiplexing. It allows the program to monitor multiple file descriptors, waiting until one or more become ready for I/O operations.

   ```c
   int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
   ```

   This function is crucial for efficiently handling multiple peer communications without blocking.

2. `socket()`: Creates a new socket of a certain type.

   ```c
   int socket(int domain, int type, int protocol);
   ```

   Used to create a UDP socket for communication.

3. `bind()`: Assigns an address to a socket.

   ```c
   int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
   ```

   Binds the socket to a specific port to listen for incoming messages.

4. `sendto()`: Sends a message on a socket to a specific destination.

   ```c
   ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
   ```

   Used to send heartbeat and acknowledgment messages to peers.

5. `recvfrom()`: Receives a message from a socket.

   ```c
   ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
   ```

   Used to receive heartbeat and acknowledgment messages from peers.

6. `getaddrinfo()`: Resolves hostnames and service names to socket addresses.

   ```c
   int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
   ```

   Used to resolve hostnames to IP addresses for communication.

## Timeout and Interval Management

The program uses two key timeouts:

1. `TOTAL_TIMEOUT` (120 seconds): The total duration the program will run before exiting.
2. `SEND_INTERVAL` (2 seconds): The interval between sending heartbeat messages to peers.

These timeouts are managed using the `time()` function and comparing the elapsed time in the main loop.

## Logging and Debugging

The program includes a logging function that prepends timestamps to messages. Debug mode can be enabled with the `-d` flag, which provides more detailed logging of events such as sending and receiving messages.

## Conclusion

This peer coordination program demonstrates effective use of UDP socket programming, efficient I/O multiplexing with `select()`, and containerization for easy deployment and scaling. The algorithm ensures reliable communication between peers and provides a clear indication when all peers have successfully exchanged messages.