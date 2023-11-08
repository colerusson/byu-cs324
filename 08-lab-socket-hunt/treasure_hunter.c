// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823702742
#define BUFSIZE 8

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>


// Global variables
int verbose = 0;
int IPv4Socket = 2;
int IPv6Socket = 2;
int IPV = AF_INET;
int LOCAL_PORT = 8080;

// Address info
socklen_t addr_len;

// Sockets
struct sockaddr_in remote_addr_in;
struct sockaddr_in6 remote_addr_in6;
struct sockaddr *remote_addr;
struct sockaddr_in local_addr_in;
struct sockaddr_in6 local_addr_in6;
struct sockaddr *local_addr;

// Request message structure
struct RequestMessage {
    unsigned char errorNumber;
    unsigned char opCode;
    unsigned short param;
    unsigned int nonce;
    unsigned char chunk[256];
    unsigned char chunkLength;
    unsigned char finishedHunt;
};

// Function prototypes
void initializeRequest(int level, int seed, unsigned char *request);
void parseResponse(unsigned char message[256], struct RequestMessage *requestMessage);
void updateTreasure(unsigned char chunk[], unsigned char length, unsigned char treasure[256], int *treasureLength);
void updateLocalPort(unsigned short port);
void updateNonce(struct RequestMessage *requestMessage);
void createIPv4();
void createIPv6();
void sendMessage(unsigned char *message, int length, char *address);
int receiveMessage(unsigned char *buf, char *address);
char *errorNumberToString(int errorNum);

int main(int argc, char *argv[]) {
    // Get command line arguments for server, port, level, and seed
    char *server = argv[1];
    int port = atoi(argv[2]);
    int level = atoi(argv[3]);
    int seed = atoi(argv[4]);

    // Create the request
    unsigned char request[BUFSIZE];
    struct RequestMessage requestMessage;

    // Create the response
    int treasureLength = 0;
    unsigned char message[256];
    unsigned char treasure[1024];

    // Create and initialize the sockets
    createIPv4();
    createIPv6();
    initializeRequest(level, seed, request);

    // Send the request
    int firstRequest = 1;
    struct addrinfo hints, *addr;
    char portString[6];

    // Set the address info
    hints.ai_family = IPV;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    // Print the port
    sprintf(portString, "%d", port);

    // Get the address info
    int result = getaddrinfo(server, portString, &hints, &addr);

    // Check for errors
    if (result != 0) {
        // Print the error from getaddrinfo
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(result));
        exit(EXIT_FAILURE);
    }
    if (addr == NULL) {
        // No address found
        fprintf(stderr, "getaddrinfo returned NULL\n");
        exit(EXIT_FAILURE);
    }

    // Set the remote address
    remote_addr_in = *((struct sockaddr_in *)addr->ai_addr);
    remote_addr = (struct sockaddr *)&remote_addr_in;

    // Set the local address
    local_addr = (struct sockaddr *)&local_addr_in;
    addr_len = sizeof(local_addr_in);

    // Send the request
    do {
        if (firstRequest == 1) {
            // Send the request
            sendMessage(request, BUFSIZE, server);
            firstRequest = 0;
        }
        else {
            // Increment the nonce
            uint32_t networkInt = requestMessage.nonce;
            uint32_t hostInt = ntohl(networkInt);
            hostInt += 1;
            networkInt = htonl(hostInt);

            // Send the nonce
            fprintf(stderr, "hostInt = %d\n", hostInt);
            sendMessage((unsigned char *)&networkInt, 4, server);
        }

        // Receive the response
        receiveMessage(message, server);
        parseResponse(message, &requestMessage);

        // Update the treasure
        updateTreasure(requestMessage.chunk, requestMessage.chunkLength, treasure, &treasureLength);

        if (requestMessage.opCode == 1) {
            // New server port
            remote_addr_in.sin_port = requestMessage.param;
        }
        else if (requestMessage.opCode == 2) {
            // New local port
            updateLocalPort(requestMessage.param);
        }
        else if (requestMessage.opCode == 3) {
            // Update nonce
            updateNonce(&requestMessage);
        }
    } while (requestMessage.finishedHunt != 1);

    // Print the treasure
    treasure[treasureLength] = 0;
    printf("%s\n", treasure);
}

void initializeRequest(int level, int seed, unsigned char *request) {
    // Initialize the request
    request[0] = 0;
    // Set the level and user id
    request[1] = level;
    request[2] = (USERID >> 24) & 0xFF;
    request[3] = (USERID >> 16) & 0xFF;
    request[4] = (USERID >> 8) & 0xFF;
    request[5] = USERID & 0xFF;
    // Set the seed
    request[6] = (seed >> 8) & 0xFF;
    request[7] = seed & 0xFF;
}

void parseResponse(unsigned char message[256], struct RequestMessage *requestMessage) {
    // Finished hunt
    if (message[0] == 0) {
        requestMessage->finishedHunt = 1;
    }

    // Set the chunk length to the first byte
    requestMessage->chunkLength = (unsigned char)message[0];

    // Error code
    if (requestMessage->chunkLength > 127) {
        requestMessage->errorNumber = (char)requestMessage->chunkLength;
        fprintf(stderr, "%s\n", errorNumberToString(requestMessage->errorNumber));
        exit(requestMessage->errorNumber);
    }

    // Chunk data
    memcpy(&(requestMessage->chunk), &message[1], requestMessage->chunkLength);

    // OpCode (1 byte after chunk data)
    requestMessage->opCode = (char)message[requestMessage->chunkLength + 1];

    // Param (2 bytes after chunk data)
    memcpy(&(requestMessage->param), &message[requestMessage->chunkLength + 2], 2);

    // Nonce (4 bytes after chunk data)
    memcpy(&(requestMessage->nonce), &message[(int)requestMessage->chunkLength + 4], 4);
}

void updateTreasure(unsigned char chunk[], unsigned char length, unsigned char treasure[256], int *treasureLength) {
    // Copy the chunk into the treasure
    memcpy(treasure + *treasureLength, chunk, length);
    *treasureLength += length;
}

void updateLocalPort(unsigned short port) {
    // Update the local port
    LOCAL_PORT = port;

    // Close the sockets
    close(IPv4Socket);
    close(IPv6Socket);

    // Create the sockets
    createIPv4();
    createIPv6();

    // Set the local address
    local_addr_in.sin_family = AF_INET; // Use AF_INET (IPv4)
    local_addr_in.sin_port = port;      // Specific port
    local_addr_in.sin_addr.s_addr = 0;  // Any/all local addresses

    // Bind the sockets
    if (bind(IPv4Socket, local_addr, addr_len) < 0) {
        perror("bind()");
    }
}

void updateNonce(struct RequestMessage *requestMessage) {
    // Update the nonce
    unsigned short m = ntohs(requestMessage->param);
    unsigned int total = 0;

    // Set up the server address
    struct sockaddr_in server_addr1;
    socklen_t addr_len1 = sizeof(server_addr1);

    // Receive m times
    for (int i = 0; i < m; i++) {
        recvfrom(IPv4Socket, NULL, 0, 0, (struct sockaddr *)&server_addr1, &addr_len1);
        unsigned short port = ntohs(server_addr1.sin_port);
        total += port;
    }

    // Set the nonce
    requestMessage->nonce = htonl(total);
}

void createIPv4() {
    // Create the IPv4 socket
    IPv4Socket = socket(AF_INET, SOCK_DGRAM, 0);
}

void createIPv6() {
    // Create the IPv6 socket
    IPv6Socket = socket(AF_INET6, SOCK_DGRAM, 0);
}

void sendMessage(unsigned char *message, int len, char *address) {
    // Set the remote address if it hasn't been set yet else send the message
    if (IPV == AF_INET) {
        sendto(IPv4Socket, message, len, 0, remote_addr, sizeof(struct sockaddr_in));
    }
    else {
        sendto(IPv6Socket, message, len, 0, remote_addr, sizeof(struct sockaddr_in));
    }
}

int receiveMessage(unsigned char *buf, char *address) {
    // Receive the message
    unsigned int size = sizeof(remote_addr_in);

    // Set the remote address if it hasn't been set yet else receive the message
    if (remote_addr->sa_family == AF_INET) {
        return (int)recvfrom(IPv4Socket, buf, 256, 0, remote_addr, &size);
    }
    else {
        return (int)recvfrom(IPv6Socket, buf, 256, 0, remote_addr, &size);
    }
}

char *errorNumberToString(int errorNum) {
    // Return the error message based on the error number
    switch (errorNum) {
        case 129:
            return "Unexpected source port";
        case 130:
            return "Wrong destination port";
        case 131:
            return "Incorrect length";
        case 132:
            return "Incorrect nonce";
        case 133:
            return "Server unable to bind to address port";
        case 134:
            return "Server unable to detect port for client to bind";
        case 135:
            return "Bad level or non-zero first byte on the first request";
        case 136:
            return "Bad user ID on the first request";
        case 137:
            return "Unknown error detected by the server";
        default:
            return "Unknown error detected by the client";
    }
}
