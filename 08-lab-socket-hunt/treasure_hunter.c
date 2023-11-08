// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823702742

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <server> <port> <level> <seed>\n", argv[0]);
        return 1;
    }

    char *server = argv[1];
    int port = atoi(argv[2]);
    int level = atoi(argv[3]);
    int seed = atoi(argv[4]);

    // Create an 8-byte message buffer as required
    unsigned char message[8];

    // Fill in the message buffer with the specified format
    message[0] = 0; // Byte 0
    message[1] = (unsigned char)level; // Byte 1, level as an integer between 0 and 4
    *((unsigned int *)&message[2]) = htonl(USERID); // Bytes 2 - 5, user ID in network byte order
    *((unsigned short *)&message[6]) = htons((unsigned short)seed); // Bytes 6 - 7, seed in network byte order

    // Print the message using print_bytes
    print_bytes(message, 8);

    // Set up socket variables
    int sockfd;
    struct addrinfo hints, *serverinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // Use UDP

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(server, port_str, &hints, &serverinfo) != 0) {
        perror("getaddrinfo");
        return 2;
    }

    // Iterate through the results and bind to the first suitable socket
    for (p = serverinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        return 3;
    }

    // Send the initial request message to the server
    ssize_t bytes_sent = sendto(sockfd, message, 8, 0, p->ai_addr, p->ai_addrlen);
    if (bytes_sent == -1) {
        perror("sendto");
        close(sockfd);
        freeaddrinfo(serverinfo);
        return 4;
    }

    // Receive the server's response using recvfrom
    unsigned char response[256]; // Max response size is 256 bytes
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;

    ssize_t bytes_received = recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr *)&their_addr, &addr_len);
    if (bytes_received == -1) {
        perror("recvfrom");
        close(sockfd);
        freeaddrinfo(serverinfo);
        return 5;
    }

    // Process the response and extract the fields as before
    if (bytes_received != 8) {
        fprintf(stderr, "Unexpected response size\n");
        return 6;
    }

    unsigned char op_code = response[0];
    unsigned char level_response = response[1];
    unsigned int nonce = ntohl(*(unsigned int *)(response + 4));

    switch (op_code) {
        case 0:
            // Handle op-code 0 (communicate with the server as you did previously)
            break;
        case 1:
            // Handle op-code 1 (communicate with a new remote server-side port)
            break;
        case 2:
            // Handle op-code 2 (communicate with a new local client-side port)
            break;
        case 3:
            // Handle op-code 3 (derive the nonce from remote ports)
            break;
        case 4:
            // Handle op-code 4 (communicate with a new address family, IPv4 or IPv6)
            break;
        default:
            fprintf(stderr, "Unknown op-code received: %d\n", op_code);
            return 7;
    }

    // Print op-code and other information
    printf("Op-Code: %d\n", op_code);
    printf("Level Response: %d\n", level_response);
    printf("Nonce: %u\n", nonce);

    // Clean up and close the socket
    close(sockfd);
    freeaddrinfo(serverinfo);

    return 0;
}

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
}
