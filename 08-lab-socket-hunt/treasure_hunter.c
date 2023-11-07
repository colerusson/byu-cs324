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

    // Parse command line arguments
    char *server = argv[1];
    int port = atoi(argv[2]);

    // Create an 8-byte message buffer as in previous steps

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

    // Send the message to the server using sendto as before

    // Receive the server's response using recvfrom
    unsigned char response[256]; // Max response size is 256 bytes
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;

    ssize_t bytes_received = recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr *)&their_addr, &addr_len);
    if (bytes_received == -1) {
        perror("recvfrom");
        return 4;
    }

    // Process the response and extract the fields as before

    // Clean up and close the socket
    freeaddrinfo(serverinfo);
    close(sockfd);

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
