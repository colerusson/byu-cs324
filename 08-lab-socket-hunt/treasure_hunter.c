// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823702742

#include <stdio.h>

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
    int level = atoi(argv[3]);
    int seed = atoi(argv[4]);

    // Create an 8-byte message buffer
    unsigned char message[8];

    // Byte 0: 0
    message[0] = 0;

    // Byte 1: Level (0 to 4)
    message[1] = (unsigned char)level;

    // Bytes 2-5: User ID (in network byte order)
    unsigned int userid_network = htonl(USERID);
    memcpy(&message[2], &userid_network, 4);

    // Bytes 6-7: Seed (in network byte order)
    unsigned short seed_network = htons(seed);
    memcpy(&message[6], &seed_network, 2);

    // Print the message in the desired format
    print_bytes(message, 8);

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
