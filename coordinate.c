#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_HOSTNAME_LENGTH 256
#define MAX_HOSTS 10

int read_hosts(const char *filename, char hosts[MAX_HOSTS][MAX_HOSTNAME_LENGTH], int *num_hosts) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file %s\n", filename);
        return -1;
    }

    *num_hosts = 0;
    while (fgets(hosts[*num_hosts], MAX_HOSTNAME_LENGTH, file) != NULL) {
        // Remove empty or newline character if present
        size_t len = strlen(hosts[*num_hosts]);
        if (len > 0 && hosts[*num_hosts][len - 1] == '\n') {
            hosts[*num_hosts][len - 1] = '\0';
        }

        // with valid hostname, update the count
        if (strlen(hosts[*num_hosts]) > 0) {
            (*num_hosts)++;
        }

        if (*num_hosts >= MAX_HOSTS) {
            fprintf(stderr, "Warning: Maximum number of hosts reached\n");
            break;
        }
    }

    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    char hosts_file[256] = "";
    char current_hostname[MAX_HOSTNAME_LENGTH];
    char hosts[MAX_HOSTS][MAX_HOSTNAME_LENGTH];
    int num_hosts = 0;
    int opt;

    // parsing command line args - h -> makes sure -h is recognized and : indicates it requires an argument
    while ((opt = getopt(argc, argv, "h:")) != -1) {
        switch (opt) {
            case 'h':
                strncpy(hosts_file, optarg, sizeof(hosts_file) - 1);
                hosts_file[sizeof(hosts_file) - 1] = '\0'; // Ensure null-termination
                break;
            default:
                fprintf(stderr, "Usage: %s [-h hostsfile]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    fprintf(stderr, "Using hosts file: %s\n", hosts_file);

    // Read host names from file
    if (read_hosts(hosts_file, hosts, &num_hosts) != 0) {
        exit(EXIT_FAILURE);
    }

    // Get current hostname
    if (gethostname(current_hostname, sizeof(current_hostname)) != 0) {
        perror("Error getting hostname");
        exit(EXIT_FAILURE);
    }

    // Discovering itself
    int current_host_index = -1;
    for (int i = 0; i < num_hosts; i++) {
        if (strcmp(hosts[i], current_hostname) == 0) {
            current_host_index = i;
            break;
        }
    }

    if (current_host_index == -1) {
        fprintf(stderr, "Error: Current host %s not found in hosts file\n", current_hostname);
        exit(EXIT_FAILURE);
    }

    // Self-identification message
    fprintf(stderr, "I am peer%d (%s)\n", current_host_index + 1, current_hostname);

    // Print all hosts
    fprintf(stderr, "All hosts:\n");
    for (int i = 0; i < num_hosts; i++) {
        fprintf(stderr, "peer%d: %s\n", i + 1, hosts[i]);
    }

    // TODO - Move forward
    fprintf(stderr, "Peer is ready. Waiting for further instructions...\n");
    while(1) {
        sleep(60);
    }

    return 0;
}