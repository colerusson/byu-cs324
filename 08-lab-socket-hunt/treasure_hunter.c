// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823702742
#define BUFSIZE 8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>


int verbose = 0;
int IPv4Socket = 2;
int IPv6Socket = 2;
int IPV = AF_INET;
int SELF_PORT = 8080;


socklen_t addr_len;
struct sockaddr_in remote_addr_in;
struct sockaddr_in6 remote_addr_in6;
struct sockaddr *remote_addr;
struct sockaddr_in local_addr_in;
struct sockaddr_in6 local_addr_in6;
struct sockaddr *local_addr;

struct RequestMessage {
    unsigned char errorNumber, huntOver;
    unsigned char opCode;
    unsigned char chunkLen;
    unsigned short param;
    unsigned int nonce;
    unsigned char chunk[256];
};

void print_bytes(unsigned char *bytes, int byteslen);
void initRequest(int level, int seed, unsigned char *request);
void changeLocalPort(unsigned short port);
void updateNonce(struct RequestMessage *rMessage);
void sendMessage(unsigned char* message, int length, char* address);
int receiveMessage(unsigned char *buf, char* address);
void makeIPv4Socket();
void makeIPv6Socket();
char* errorToString(int errorNum);
void parseResponse(unsigned char message[256], struct RequestMessage *rMessage);
void addTreasureChunk(unsigned char chunk[], unsigned char len, unsigned char treasure[256], int *treasureLength);

int main(int argc, char *argv[]) {
    // get the command arguments for server, port, level, and seed
    char *server = argv[1];
    int port  = atoi(argv[2]);
    int level = atoi(argv[3]);
    int seed  = atoi(argv[4]);

    // create the request
    unsigned char request[BUFSIZE];
    struct RequestMessage requestMessage;

    // create the response
    int treasureLength = 0;
    unsigned char message[256];
    unsigned char treasure[1024];

    // create the socket
    makeIPv4Socket();
    makeIPv6Socket();
    initRequest(level, seed, request);

    // send the request
    int isFirst = 1;
    struct addrinfo hints, *addr;
    char portString[6];

    // set the address info
    hints.ai_family = IPV;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    sprintf(portString, "%d", port);

    // get the address info
    int result = getaddrinfo(server, portString, &hints, &addr);
    if (result != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(result));
        exit(EXIT_FAILURE);
    }
    if (addr == NULL) {
        fprintf(stderr, "getaddrinfo returned NULL\n");
        exit(EXIT_FAILURE);
    }

    // set the remote address
    remote_addr_in = *((struct sockaddr_in *) addr->ai_addr);
    remote_addr = (struct sockaddr *) &remote_addr_in;
    local_addr = (struct sockaddr *) &local_addr_in;
    addr_len = sizeof(local_addr_in);

    // send the request
    do {
        if (isFirst == 1) {
            sendMessage(request, BUFSIZE, server);
            isFirst = 0;
        } else {
            uint32_t networkInt = requestMessage.nonce;
            uint32_t hostInt = ntohl(networkInt);
            hostInt += 1;
            networkInt = htonl(hostInt);
            fprintf(stderr, "hostInt = %d\n", hostInt);
            sendMessage((unsigned char *) &networkInt, 4, server);
        }
        receiveMessage(message, server);
        parseResponse(message, &requestMessage);

        addTreasureChunk(requestMessage.chunk, requestMessage.chunkLen, treasure, &treasureLength);
        switch (requestMessage.opCode) {
            case 1: // new server port
                remote_addr_in.sin_port = requestMessage.param;
                break;
            case 2: // new local port
                changeLocalPort(requestMessage.param);
                break;
            case 3: // update nonce
                updateNonce(&requestMessage);
                break;
        }
    } while (requestMessage.huntOver != 1);
    treasure[treasureLength] = 0;
    printf("%s\n", treasure);
}

void parseResponse(unsigned char message[256], struct RequestMessage *rMessage) {
    if (message[0] == 0) {
        rMessage->huntOver = 1;
    }
    rMessage->chunkLen = (unsigned char) message[0];
    // error code
    if (rMessage->chunkLen > 127) {
        rMessage->errorNumber = (char) rMessage->chunkLen;
        fprintf(stderr, "%s\n", errorToString(rMessage->errorNumber));
        exit(rMessage->errorNumber);
    }
    // chunk
    memcpy(&(rMessage->chunk), &message[1], rMessage->chunkLen);
    // opCode
    rMessage->opCode = (char) message[rMessage->chunkLen + 1];
    // param
    memcpy(&(rMessage->param), &message[rMessage->chunkLen + 2], 2);
    // nonce
    memcpy(&(rMessage->nonce), &message[(int) rMessage->chunkLen + 4], 4);
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


void updateNonce(struct RequestMessage *rMessage) {
    unsigned short m = ntohs(rMessage->param);
    unsigned int total = 0;
    struct sockaddr_in server_addr1;
    socklen_t addr_len1 = sizeof(server_addr1);
    for (int i = 0; i < m; i++) {
        recvfrom(IPv4Socket, NULL, 0, 0, (struct sockaddr *)&server_addr1, &addr_len1);
        unsigned short port = ntohs(server_addr1.sin_port);
        total += port;
    }
    rMessage->nonce = htonl(total);
}

void initRequest(int level, int seed, unsigned char *request) {
    // set the value of the request to the following format
    request[0] = 0;
    request[1] = level;
    request[2] = (USERID >> 24) & 0xFF;
    request[3] = (USERID >> 16) & 0xFF;
    request[4] = (USERID >> 8) & 0xFF;
    request[5] =  USERID & 0xFF;
    request[6] = (seed >> 8) & 0xFF;
    request[7] =  seed & 0xFF;
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
}

void makeIPv4Socket() {
    IPv4Socket = socket(AF_INET, SOCK_DGRAM, 0);
}

void makeIPv6Socket() {
    IPv6Socket = socket(AF_INET6, SOCK_DGRAM, 0);
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
