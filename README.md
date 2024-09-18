# Peer Coordination Program

This program implements a peer-to-peer communication system over UDP sockets where multiple peers send heartbeats to each other and acknowledge receipt. Once all peers have successfully exchanged heartbeats and acknowledgments, they print "READY".

## Requirements

- Docker
- Docker Compose

## Usage

1. Clone the repository and navigate to the implementation.

2. Build the Docker image:
   ```
   docker build -t prj1 .
   ```

3. Run the program using Docker Compose:
   ```
   docker-compose up
   ```

   This will start 5 peer containers as defined in the `docker-compose.yml` file.

4. To run in debug mode, modify the `docker-compose.yml` file to add the `-d` flag to each peer's command:
   ```yaml
   command: -h hostsfile.txt -d
   ```

5. To change the number of peers, modify the `docker-compose.yml` file by adding or removing peer services as needed.

6. To stop the program:
   ```
   docker-compose down
   ```

## Configuration

- `hostsfile.txt`: Contains the list of peer hostnames, one per line.
- `coordinate.c`: The main program file.
- `Dockerfile`: Defines the Docker image for the program.
- `docker-compose.yml`: Defines the multi-container Docker application.

## Troubleshooting

If you encounter any issues:
1. Ensure all required files are present in the directory.
2. Check that the `hostsfile.txt` contains the correct hostnames.
3. Verify that Docker and Docker Compose are installed and running correctly.
4. Run in debug mode for more detailed logging.

For more detailed information about the implementation, please refer to the REPORT.md file.