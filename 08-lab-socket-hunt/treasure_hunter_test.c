#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define USERID 1823702742
#define BUFSIZE 8

int verbose = 0;
int IPv4Socket = 2;
int IPv6Socket = 2;
int SERVER_PORT = 8080;
int SELF_PORT = 8080;
int IPV = AF_INET;

int addr_fam;
socklen_t addr_len;

struct sockaddr_in remote_addr_in;
struct sockaddr_in6 remote_addr_in6;
struct sockaddr *remote_addr;

struct sockaddr_in local_addr_in;
struct sockaddr_in6 local_addr_in6;
struct sockaddr *local_addr;


struct ParsedMessage {
    unsigned char errorNumber, huntOver;
    unsigned char opCode;
    unsigned char chunkLen;
    unsigned short param;
    unsigned int nonce;
    unsigned char chunk[256];
};

void print_bytes(unsigned char *bytes, int byteslen);
void initRequest(int level, int seed, unsigned char *request);
void changeServerPort(unsigned short port);
void changeLocalPort(unsigned short port);
void updateNonce(struct ParsedMessage *pMessage);
void swapIPv();
void sendMessage(unsigned char* message, int length, char* address);
int receiveMessage(unsigned char *buf, char* address);
void makeIPv4Socket();
void makeIPv6Socket();
char* errorToString(int errorNum);
void parseResponse(unsigned char message[256], struct ParsedMessage *pMessage);
void addTreasureChunk(unsigned char chunk[], unsigned char len, unsigned char treasure[256], int *treasureLength);

int main(int argc, char *argv[]) {
    // parse command args
    char *server = argv[1];
    int seed, level;
    SERVER_PORT  = atoi(argv[2]);
    level = atoi(argv[3]);
    seed  = atoi(argv[4]);
    // create our request
    unsigned char request[BUFSIZE];
    struct ParsedMessage parsedMessage;

    unsigned char message[256];
    unsigned char treasure[1024];
    int treasureLength = 0;

    makeIPv4Socket();
    makeIPv6Socket();
    initRequest(level, seed, request);
    int isFirst = 1;
    struct addrinfo hints, *addr;
    char portString[6];
    hints.ai_family = IPV;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    sprintf(portString, "%d", SERVER_PORT);

    int result = getaddrinfo(server, portString, &hints, &addr);
    if (result != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(result));
        exit(EXIT_FAILURE);
    }
    if (addr == NULL) {
        fprintf(stderr, "getaddrinfo returned NULL\n");
        exit(EXIT_FAILURE);
    }

    remote_addr_in = *((struct sockaddr_in *) addr->ai_addr);
    remote_addr = (struct sockaddr *) &remote_addr_in;
    local_addr = (struct sockaddr *) &local_addr_in;
    addr_len = sizeof(local_addr_in);

    do {
        if (isFirst == 1) {
            sendMessage(request, BUFSIZE, server);
            isFirst = 0;
        } else {
            uint32_t networkInt = parsedMessage.nonce;
            uint32_t hostInt = ntohl(networkInt);
            hostInt += 1;
            networkInt = htonl(hostInt);
            fprintf(stderr, "hostInt = %d\n", hostInt);
            sendMessage((unsigned char *) &networkInt, 4, server);
        }
        receiveMessage(message, server);
        parseResponse(message, &parsedMessage);

        addTreasureChunk(parsedMessage.chunk, parsedMessage.chunkLen, treasure, &treasureLength);
        switch (parsedMessage.opCode) {
            case 1: // new server port
                remote_addr_in.sin_port = parsedMessage.param;
                break;
            case 2: // new local port
                changeLocalPort(parsedMessage.param);
                break;
            case 3: // update nonce
                updateNonce(&parsedMessage);
                break;
            case 4: // switch IPV
                swapIPv();
                break;
        }
    } while (parsedMessage.huntOver != 1);
    treasure[treasureLength] = 0;
    printf("%s\n", treasure);
}
void parseResponse(unsigned char message[256], struct ParsedMessage *pMessage) {
    if (message[0] == 0) {
        pMessage->huntOver = 1;
    }
    pMessage->chunkLen = (unsigned char) message[0];
    // error code
    if (pMessage->chunkLen > 127) {
        pMessage->errorNumber = (char) pMessage->chunkLen;
        fprintf(stderr, "%s\n", errorToString(pMessage->errorNumber));
        exit(pMessage->errorNumber);
    }
    // chunk
    memcpy(&(pMessage->chunk), &message[1], pMessage->chunkLen);
    // opCode
    pMessage->opCode = (char) message[pMessage->chunkLen + 1];
    // param
    memcpy(&(pMessage->param), &message[pMessage->chunkLen + 2], 2);
    // nonce
    memcpy(&(pMessage->nonce), &message[(int) pMessage->chunkLen + 4], 4);
}
void addTreasureChunk(unsigned char chunk[], unsigned char len, unsigned char treasure[256], int* treasureLength) {
    memcpy(treasure + *treasureLength, chunk, len);
    *treasureLength += len;
}
char* errorToString(int errorNum){
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
            return "Bad level or non-zero first byte on first request";
        case 136:
            return "Bad user ID on first request";
        case 137:
            return "Unknown error detected by server";
        default:
            return "Unknown error detected by client";
    }
}


void updateNonce(struct ParsedMessage *pMessage) {
    unsigned short m = ntohs(pMessage->param);  // correct afaik
    unsigned int total = 0;                     // correct type
    struct sockaddr_in server_addr1;            // works
    socklen_t addr_len1 = sizeof(server_addr1); // works
    for (int i = 0; i < m; i++) {
        recvfrom(IPv4Socket, NULL, 0, 0, (struct sockaddr *)&server_addr1, &addr_len1);
        unsigned short port = ntohs(server_addr1.sin_port); // appears to work idk how to know if it's the correct value though
//        fprintf(stderr, "port = %d\n", port);
        total += port;
    }
    pMessage->nonce = htonl(total);
}

void initRequest(int level, int seed, unsigned char *request) {
    // set the value of our request to the following format
    request[0] = 0;                     // set the first byte to 0
    request[1] = level;                 // set the second byte to contain the 4-bit value of level in the most significant nibble
    request[2] = (USERID >> 24) & 0xFF; // set the fourth byte to the most significant byte of USERID
    request[3] = (USERID >> 16) & 0xFF; // set the fifth byte to the second most significant byte of USERID
    request[4] = (USERID >> 8) & 0xFF;  // set the sixth byte to the second least significant byte of USERID
    request[5] =  USERID & 0xFF;        // set the seventh byte to the least significant byte of USERID
    request[6] = (seed >> 8) & 0xFF;    // set the eighth byte to the most significant byte of seed
    request[7] =  seed & 0xFF;          // set the ninth byte to the least significant byte of seed
}
void sendMessage(unsigned char* message, int len, char* address){
    if(IPV == AF_INET){
        sendto(IPv4Socket, message, len, 0, remote_addr, sizeof(struct sockaddr_in));
    }else{
        sendto(IPv6Socket, message, len, 0, remote_addr, sizeof(struct sockaddr_in));
    }
}
int receiveMessage(unsigned char *buf, char* address) {
    unsigned int size = sizeof(remote_addr_in);
    if (remote_addr->sa_family == AF_INET) {
        return (int) recvfrom(IPv4Socket, buf, 256, 0, remote_addr, &size);
    } else {
        return (int) recvfrom(IPv6Socket, buf, 256, 0, remote_addr, &size);
        }
}
void changeLocalPort(unsigned short port){
    SELF_PORT = port;
    close(IPv4Socket);
    close(IPv6Socket);

    makeIPv4Socket();
    makeIPv6Socket();

    local_addr_in.sin_family = AF_INET; // use AF_INET (IPv4)
    local_addr_in.sin_port = port; // specific port
    local_addr_in.sin_addr.s_addr = 0; // any/all local addresses
    if (bind(IPv4Socket, local_addr, addr_len) < 0) {
        perror("bind()");
    }

//    if (IPV == AF_INET) {

//    } else {
//        local_addr_in6.sin6_family = AF_INET6; // IPv6 (AF_INET6)
//        local_addr_in6.sin6_port = htons(SELF_PORT); // specific port
//        bzero(local_addr_in6.sin6_addr.s6_addr, 16); // any/all local addresses
//        if (bind(IPv6Socket, local_addr, addr_len) < 0) {
//            perror("bind()");
//        }
//    }

}
void makeIPv4Socket() {
    IPv4Socket = socket(AF_INET, SOCK_DGRAM, 0);
}
void makeIPv6Socket() {
    IPv6Socket = socket(AF_INET6, SOCK_DGRAM, 0);
}
void swapIPv() {
    if (IPV == AF_INET) {
        IPV = AF_INET6;
    } else {
        IPV = AF_INET;
    }
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
