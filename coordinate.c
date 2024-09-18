#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>

#define MAX_HOSTNAME_LENGTH 256
#define MAX_HOSTS 10
#define MAX_MESSAGE_LENGTH 1024
#define LOG_BUFFER_SIZE 1024
#define UDP_PORT "8888"
#define TOTAL_TIMEOUT 300
#define SEND_INTERVAL 2
#define HEARTBEAT_MESSAGE "HEARTBEAT"
#define ACK_MESSAGE "HEARTBEAT_ACK"

// Peer information
typedef struct {
    char hostname[MAX_HOSTNAME_LENGTH];
    struct sockaddr_storage addr;
    socklen_t addrlen;
    int heartbeat_received;
    int ack_received;
} Peer;

// Global variables
Peer peers[MAX_HOSTS];
int num_peers = 0;
int current_peer_index = -1;

// Log messages with timestamp (STDERR)
void log_message(const char* message) {
    time_t now;
    char timestamp[64];
    time(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(stderr, "[%s] %s\n", timestamp, message);
}

// Read hosts from a file
int read_hosts(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        log_message("Error: Unable to open hosts file");
        return -1;
    }

    num_peers = 0;
    while (fgets(peers[num_peers].hostname, MAX_HOSTNAME_LENGTH, file) != NULL) {
        size_t len = strlen(peers[num_peers].hostname);
        if (len > 0 && peers[num_peers].hostname[len - 1] == '\n') {
            peers[num_peers].hostname[len - 1] = '\0';
        }

        if (strlen(peers[num_peers].hostname) > 0) {
            peers[num_peers].heartbeat_received = 0;
            peers[num_peers].ack_received = 0;
            num_peers++;
        }

        if (num_peers >= MAX_HOSTS) {
            log_message("Warning: Maximum number of hosts reached");
            break;
        }
    }

    fclose(file);
    return 0;
}

// Resolve peer addresses
int resolve_peers() {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    for (int i = 0; i < num_peers; i++) {
        status = getaddrinfo(peers[i].hostname, UDP_PORT, &hints, &res);
        if (status != 0) {
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            return -1;
        }

        memcpy(&peers[i].addr, res->ai_addr, res->ai_addrlen);
        peers[i].addrlen = res->ai_addrlen;

        freeaddrinfo(res);
    }

    return 0;
}

// Create and bind UDP socket
int create_socket() {
    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, UDP_PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        freeaddrinfo(res);
        return -1;
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

// Send message to a peer
void send_message(int sockfd, int peer_index, char* message) {
    if (sendto(sockfd, message, strlen(message), 0,
               (struct sockaddr*)&peers[peer_index].addr,
               peers[peer_index].addrlen) == -1) {
        perror("sendto");
    } else {
        char log_buffer[LOG_BUFFER_SIZE];
        snprintf(log_buffer, sizeof(log_buffer), "Sent message: %s to peer%d (%s)",
                 message, peer_index + 1, peers[peer_index].hostname);
        log_message(log_buffer);
    }
}

int main(int argc, char *argv[]) {
    char hosts_file[256] = "";
    char current_hostname[MAX_HOSTNAME_LENGTH];
    int opt;

    while ((opt = getopt(argc, argv, "h:")) != -1) {
        switch (opt) {
            case 'h':
                strncpy(hosts_file, optarg, sizeof(hosts_file) - 1);
                hosts_file[sizeof(hosts_file) - 1] = '\0';
                break;
            default:
                log_message("Usage: arguments: [-h hostsfile]");
                exit(EXIT_FAILURE);
        }
    }

    char log_buffer[LOG_BUFFER_SIZE];
    snprintf(log_buffer, sizeof(log_buffer), "Using hosts file: %s", hosts_file);
    log_message(log_buffer);

    if (read_hosts(hosts_file) != 0) {
        exit(EXIT_FAILURE);
    }

    if (gethostname(current_hostname, sizeof(current_hostname)) != 0) {
        log_message("Error getting hostname");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_peers; i++) {
        if (strcmp(peers[i].hostname, current_hostname) == 0) {
            current_peer_index = i;
            break;
        }
    }

    if (current_peer_index == -1) {
        snprintf(log_buffer, sizeof(log_buffer), "Error: Current host %s not found in hosts file", current_hostname);
        log_message(log_buffer);
        exit(EXIT_FAILURE);
    }

    snprintf(log_buffer, sizeof(log_buffer), "I am %s", current_hostname);
    log_message(log_buffer);

    log_message("Expecting messages from: ");
    for (int i = 0; i < num_peers; i++) {
        if (i != current_peer_index) {
            snprintf(log_buffer, sizeof(log_buffer), "%s", peers[i].hostname);
            log_message(log_buffer);
        }
    }

    if (resolve_peers() != 0) {
        log_message("Error resolving peer addresses");
        exit(EXIT_FAILURE);
    }

    int sockfd = create_socket();
    if (sockfd == -1) {
        log_message("Error creating socket");
        exit(EXIT_FAILURE);
    }

    fd_set readfds;
    struct timeval tv;
    time_t start_time = time(NULL);
    time_t last_send_time[MAX_HOSTS] = {0};

    int all_communication_complete = 0;
    while (!all_communication_complete) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);

        if (activity == -1) {
            perror("select");
            break;
        } else {
            time_t current_time = time(NULL);
            
            if (current_time - start_time >= TOTAL_TIMEOUT) {
                log_message("Total timeout reached. Exiting. Please run the program again.");
                break;
            }

            for (int i = 0; i < num_peers; i++) {
                if (i != current_peer_index && (current_time - last_send_time[i] >= SEND_INTERVAL) && !peers[i].ack_received) {
                    send_message(sockfd, i, HEARTBEAT_MESSAGE);
                    last_send_time[i] = current_time;
                }
            }

            if (FD_ISSET(sockfd, &readfds)) {
                char buffer[MAX_MESSAGE_LENGTH];
                struct sockaddr_storage sender_addr;
                socklen_t addr_len = sizeof(sender_addr);

                ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                                  (struct sockaddr*)&sender_addr, &addr_len);
                if (bytes_received == -1) {
                    perror("recvfrom");
                } else {
                    buffer[bytes_received] = '\0';
                    
                    for (int i = 0; i < num_peers; i++) {
                        if (memcmp(&peers[i].addr, &sender_addr, addr_len) == 0) {
                            if (strcmp(buffer, HEARTBEAT_MESSAGE) == 0) {
                                if (!peers[i].heartbeat_received) {
                                    peers[i].heartbeat_received = 1;
                                    char log_buffer[LOG_BUFFER_SIZE];
                                    snprintf(log_buffer, sizeof(log_buffer), "Received heartbeat from %s", peers[i].hostname);
                                    log_message(log_buffer);
                                    // Send ACK
                                    send_message(sockfd, i, ACK_MESSAGE);
                                }
                            } else if (strcmp(buffer, ACK_MESSAGE) == 0) {
                                if (!peers[i].ack_received) {
                                    peers[i].ack_received = 1;
                                    char log_buffer[LOG_BUFFER_SIZE];
                                    snprintf(log_buffer, sizeof(log_buffer), "Received ACK from %s", peers[i].hostname);
                                    log_message(log_buffer);
                                }
                            }
                            break;
                        }
                    }
                }
            }

            // Check if all communication is complete
            all_communication_complete = 1;
            for (int i = 0; i < num_peers; i++) {
                if (i != current_peer_index) {
                    if (!peers[i].ack_received || !peers[i].heartbeat_received) {
                    all_communication_complete = 0;
                    break;
                    }
                } 
            }
        }
    }

    if (all_communication_complete) {
        char log_buffer[LOG_BUFFER_SIZE];
        snprintf(log_buffer, sizeof(log_buffer), "Communication Completed. %s going to sleep", current_hostname);
        log_message(log_buffer);
    }

    close(sockfd);
    return 0;
}