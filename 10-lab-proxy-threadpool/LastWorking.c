#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <limits.h>

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400
#define MAX_BUFFER_SIZE INT_MAX

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int complete_request_received(char *, ssize_t);
int parse_request(char *, char *, char *, char *, char *);
//int parse_request(char *, ssize_t, char *, char *, char *, char *);
int open_sfd(int);
void handle_client(int);
void test_parser();
void print_bytes(unsigned char *, int);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int server_fd = open_sfd(port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        // Handle client's HTTP request
        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}

int complete_request_received(char *request, ssize_t received_bytes) {
    // Check if we have received at least some data and search for end of headers
    if (received_bytes > 0) {
        char *end_of_headers = strstr(request, "\r\n\r\n");
        if (end_of_headers != NULL) {
            printf("Printing end of headers: %s\n", end_of_headers);
            return 1; // Request is complete
        }
    }
    return 0; // Request is not complete
}

int parse_request(char *request, char *method, char *hostname, char *port, char *path) {
    //print_bytes(request, received_bytes);
    printf("Printing Request: ");
    printf("%s", request);
    printf("\n\n");

    // Check if the request is complete
//    if (!complete_request_received(request, received_bytes)) {
//        printf("Incomplete request received\n");
//        return 0; // Request is incomplete
//    }

    // Extract method
    char *end_of_method = strstr(request, " ");
    if (end_of_method != NULL) {
        strncpy(method, request, end_of_method - request);
        method[end_of_method - request] = '\0';
    } else {
        printf("Method extraction failed\n");
        return 0; // Method extraction failed
    }

    // Extract the URL
    char *start_of_url = strstr(request, "http://");
    if (start_of_url != NULL) {
        char *end_of_url = strstr(start_of_url, " ");
        if (end_of_url != NULL) {
            int url_length = end_of_url - start_of_url;
            char url[url_length + 1];
            strncpy(url, start_of_url, url_length);
            url[url_length] = '\0';

            // Extract hostname, port, and path from the URL
            char *start_of_hostname = start_of_url + 7; // Move past "http://"
            char *end_of_hostname = strchr(start_of_hostname, ':');
            char *end_of_path = strchr(start_of_hostname, '/');

            if (end_of_hostname != NULL && (end_of_path == NULL || end_of_path > end_of_hostname)) {
                int hostname_length = end_of_hostname - start_of_hostname;
                strncpy(hostname, start_of_hostname, hostname_length);
                hostname[hostname_length] = '\0';

                int port_length = (end_of_path != NULL ? end_of_path : start_of_url + url_length) - end_of_hostname - 1;
                strncpy(port, end_of_hostname + 1, port_length);
                port[port_length] = '\0';
            } else {
                strcpy(port, "80");
                int hostname_length = (end_of_path != NULL ? end_of_path : start_of_url + url_length) - start_of_hostname;
                strncpy(hostname, start_of_hostname, hostname_length);
                hostname[hostname_length] = '\0';
            }

            if (end_of_path != NULL) {
                int path_length = start_of_url + url_length - end_of_path;
                strncpy(path, end_of_path, path_length);
                path[path_length] = '\0';
            } else {
                printf("Path extraction failed\n");
                return 0; // Path extraction failed
            }

            return 1; // Parsing successful
        } else {
            printf("URL extraction failed\n");
            return 0; // URL extraction failed
        }
    } else {
        printf("URL not found\n");
        return 0; // URL not found
    }
}

/**WORKING**/
//int parse_request(char *request, ssize_t received_bytes, char *method, char *hostname, char *port, char *path) {
//    //print_bytes(request, received_bytes);
//    printf("Printing Request: ");
//    printf("%s", request);
//    printf("\n\n");
//
//    // Check if the request is complete
//    if (!complete_request_received(request, received_bytes)) {
//        printf("Incomplete request received\n");
//        return 0; // Request is incomplete
//    }
//
//    // Extract method
//    char *end_of_method = strstr(request, " ");
//    if (end_of_method != NULL) {
//        strncpy(method, request, end_of_method - request);
//        method[end_of_method - request] = '\0';
//    } else {
//        printf("Method extraction failed\n");
//        return 0; // Method extraction failed
//    }
//
//    // Extract the URL
//    char *start_of_url = strstr(request, "http://");
//    if (start_of_url != NULL) {
//        char *end_of_url = strstr(start_of_url, " ");
//        if (end_of_url != NULL) {
//            int url_length = end_of_url - start_of_url;
//            char url[url_length + 1];
//            strncpy(url, start_of_url, url_length);
//            url[url_length] = '\0';
//
//            // Extract hostname, port, and path from the URL
//            char *start_of_hostname = start_of_url + 7; // Move past "http://"
//            char *end_of_hostname = strchr(start_of_hostname, ':');
//            char *end_of_path = strchr(start_of_hostname, '/');
//
//            if (end_of_hostname != NULL && (end_of_path == NULL || end_of_path > end_of_hostname)) {
//                int hostname_length = end_of_hostname - start_of_hostname;
//                strncpy(hostname, start_of_hostname, hostname_length);
//                hostname[hostname_length] = '\0';
//
//                int port_length = (end_of_path != NULL ? end_of_path : start_of_url + url_length) - end_of_hostname - 1;
//                strncpy(port, end_of_hostname + 1, port_length);
//                port[port_length] = '\0';
//            } else {
//                strcpy(port, "80");
//                int hostname_length = (end_of_path != NULL ? end_of_path : start_of_url + url_length) - start_of_hostname;
//                strncpy(hostname, start_of_hostname, hostname_length);
//                hostname[hostname_length] = '\0';
//            }
//
//            if (end_of_path != NULL) {
//                int path_length = start_of_url + url_length - end_of_path;
//                strncpy(path, end_of_path, path_length);
//                path[path_length] = '\0';
//            } else {
//                printf("Path extraction failed\n");
//                return 0; // Path extraction failed
//            }
//
//            return 1; // Parsing successful
//        } else {
//            printf("URL extraction failed\n");
//            return 0; // URL extraction failed
//        }
//    } else {
//        printf("URL not found\n");
//        return 0; // URL not found
//    }
//}

void handle_client(int client_fd) {
    char buffer[1024]; // Adjust buffer size as needed
    char method[16], hostname[64], port[8], path[64];
    char wholeRequest[500];
    int startPoint = 0;
    ssize_t bytes_received;

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0 )) > 0) {
        if (bytes_received == -1) {
            perror("Receive failed");
            return;
        }
        //printf("bytes received: %d\n", bytes_received);

        char *end_of_headers = strstr(buffer, "\r\n\r\n");
        memcpy(wholeRequest + startPoint, buffer, bytes_received);
        startPoint += bytes_received;
        if (end_of_headers != NULL) {
            printf("Printing end of headers: %s\n", end_of_headers);
            break; // Request is complete
        }
    }

    printf("Printing whole request: %s\n", wholeRequest);
    wholeRequest[startPoint] = '\0';

    // Read from the socket until the entire HTTP request is received
    if (parse_request(wholeRequest, method, hostname, port, path)) {
        printf("Method: %s, Hostname: %s, Port: %s, Path: %s\n", method, hostname, port, path);

        // Create the modified HTTP request to send to the server
        char modified_request[1024]; // Adjust size as needed
        sprintf(modified_request, "GET %s HTTP/1.0\r\nHost: %s:%s\r\nUser-Agent: %s\r\nConnection: close\r\nProxy-Connection: close\r\n\r\n",
                path, hostname, port, user_agent_hdr);

        // Create a socket to communicate with the server and forward the request
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("Socket creation error");
            return;
        }

        // Set up the server address and port to connect to using getaddrinfo
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(atoi(port)); // Convert port to network byte order

        // Resolve the hostname to an IP address using getaddrinfo
        struct addrinfo hints, *server_info;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; // Use IPv4
        hints.ai_socktype = SOCK_STREAM;

        int status;
        if ((status = getaddrinfo(hostname, port, &hints, &server_info)) != 0) {
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            close(server_fd);
            return;
        }

        // Loop through the addresses returned by getaddrinfo until a successful connection is made
        struct addrinfo *p;
        for (p = server_info; p != NULL; p = p->ai_next) {
            if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
                perror("connect");
                close(server_fd);
                continue;
            }
            break;
        }

        if (p == NULL) {
            fprintf(stderr, "Failed to connect\n");
            freeaddrinfo(server_info);
            close(server_fd);
            return;
        }

        // Send the modified request to the server
        ssize_t bytes_sent = send(server_fd, modified_request, strlen(modified_request), 0);
        if (bytes_sent < 0) {
            perror("Send error");
            close(server_fd);
            return;
        }

        // Receive and forward the server's response to the client
        char server_response[1024]; // Adjust size as needed
        ssize_t server_bytes_received;
        while ((server_bytes_received = recv(server_fd, server_response, sizeof(server_response), 0)) > 0) {
            ssize_t response_sent = send(client_fd, server_response, server_bytes_received, 0);
            if (response_sent < 0) {
                perror("Response send error");
                close(server_fd);
                close(client_fd);
                return;
            }

            memset(server_response, 0, sizeof(server_response));
        }

        if (server_bytes_received < 0) {
            perror("Server receive error");
        }

        // Close the connection to the server
        close(server_fd);

        // Close the client socket
        close(client_fd);
        return;
    } else {
        printf("Failed to parse HTTP request\n");
        // Close the client socket (moved to the end)
        close(client_fd);
        return;
    }

    if (bytes_received < 0) {
        perror("Client receive error");
    }

    // Close the client socket
    close(client_fd);
}


/**WORKING THROUGH MOST OF TEST CASE 1**/
//void handle_client(int client_fd) {
//    char buffer[1024]; // Adjust buffer size as needed
//    ssize_t bytes_received;
//    ssize_t total_received = 0;
//    int request_complete = 0;
//
//    // Read from the socket until the entire HTTP request is received
//    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
//        // Accumulate received data
//        total_received += bytes_received;
//
//        // Check if the request is complete by calling the modified function
//        request_complete = complete_request_received(buffer, total_received);
//
//        // Process the request if it's complete
//        if (request_complete) {
//            // Combine all received chunks to form the complete request
//            char complete_request[total_received + 1];
//            memcpy(complete_request, buffer, total_received);
//            complete_request[total_received] = '\0';
//
//            // Parse the complete request by passing total_received
//            char method[16], hostname[64], port[8], path[64];
//            if (parse_request(complete_request, total_received, method, hostname, port, path)) {
//                printf("Method: %s, Hostname: %s, Port: %s, Path: %s\n", method, hostname, port, path);
//
//                // Create the modified HTTP request to send to the server
//                char modified_request[1024]; // Adjust size as needed
//                sprintf(modified_request, "GET %s HTTP/1.0\r\nHost: %s:%s\r\nUser-Agent: %s\r\nConnection: close\r\nProxy-Connection: close\r\n\r\n",
//                        path, hostname, port, user_agent_hdr);
//
//                // Create a socket to communicate with the server and forward the request
//                int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//                if (server_fd < 0) {
//                    perror("Socket creation error");
//                    return;
//                }
//
//                // Set up the server address and port to connect to using getaddrinfo
//                struct sockaddr_in server_addr;
//                memset(&server_addr, 0, sizeof(server_addr));
//                server_addr.sin_family = AF_INET;
//                server_addr.sin_port = htons(atoi(port)); // Convert port to network byte order
//
//                // Resolve the hostname to an IP address using getaddrinfo
//                struct addrinfo hints, *server_info;
//                memset(&hints, 0, sizeof hints);
//                hints.ai_family = AF_INET; // Use IPv4
//                hints.ai_socktype = SOCK_STREAM;
//
//                int status;
//                if ((status = getaddrinfo(hostname, port, &hints, &server_info)) != 0) {
//                    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
//                    close(server_fd);
//                    return;
//                }
//
//                // Loop through the addresses returned by getaddrinfo until a successful connection is made
//                struct addrinfo *p;
//                for (p = server_info; p != NULL; p = p->ai_next) {
//                    if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
//                        perror("connect");
//                        close(server_fd);
//                        continue;
//                    }
//                    break;
//                }
//
//                if (p == NULL) {
//                    fprintf(stderr, "Failed to connect\n");
//                    freeaddrinfo(server_info);
//                    close(server_fd);
//                    return;
//                }
//
//                // Send the modified request to the server
//                ssize_t bytes_sent = send(server_fd, modified_request, strlen(modified_request), 0);
//                if (bytes_sent < 0) {
//                    perror("Send error");
//                    close(server_fd);
//                    return;
//                }
//
//                // Receive and forward the server's response to the client
//                char server_response[1024]; // Adjust size as needed
//                ssize_t server_bytes_received;
//                while ((server_bytes_received = recv(server_fd, server_response, sizeof(server_response), 0)) > 0) {
//                    ssize_t response_sent = send(client_fd, server_response, server_bytes_received, 0);
//                    if (response_sent < 0) {
//                        perror("Response send error");
//                        close(server_fd);
//                        close(client_fd);
//                        return;
//                    }
//
//                    memset(server_response, 0, sizeof(server_response));
//                }
//
//                if (server_bytes_received < 0) {
//                    perror("Server receive error");
//                }
//
//                // Close the connection to the server
//                close(server_fd);
//
//                // Close the client socket
//                close(client_fd);
//                return;
//            } else {
//                printf("Failed to parse HTTP request\n");
//                // Close the client socket (moved to the end)
//                close(client_fd);
//                return;
//            }
//        }
//    }
//
//    if (bytes_received < 0) {
//        perror("Client receive error");
//    }
//
//    // Close the client socket
//    close(client_fd);
//}

int open_sfd(int port) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, 10) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    return sfd;
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


/**DIFFERENT WORKING PROGRAM**/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400
#define MAX_BUFFER_SIZE 1024

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int complete_request_received(char *, ssize_t);
int parse_request(char *, char *, char *, char *, char *);
int open_sfd(int);
void handle_client(int);
void test_parser();
void print_bytes(unsigned char *, int);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int server_fd = open_sfd(port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        // Handle client's HTTP request
        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}

int complete_request_received(char *request, ssize_t received_bytes) {
    // Check if we have received at least some data and search for end of headers
    if (received_bytes > 0) {
        char *end_of_headers = strstr(request, "\r\n\r\n");
        if (end_of_headers != NULL) {
            printf("Printing end of headers: %s\n", end_of_headers);
            return 1; // Request is complete
        }
    }
    return 0; // Request is not complete
}

int parse_request(char *request, char *method, char *hostname, char *port, char *path) {
    // Extract method
    char *end_of_method = strstr(request, " ");
    if (end_of_method != NULL) {
        strncpy(method, request, end_of_method - request);
        method[end_of_method - request] = '\0';
    } else {
        printf("Method extraction failed\n");
        return 0; // Method extraction failed
    }

    // Extract the URL
    char *start_of_url = strstr(request, "http://");
    if (start_of_url != NULL) {
        char *end_of_url = strstr(start_of_url, " ");
        if (end_of_url != NULL) {
            int url_length = end_of_url - start_of_url;
            char url[url_length + 1];
            strncpy(url, start_of_url, url_length);
            url[url_length] = '\0';

            // Extract hostname, port, and path from the URL
            char *start_of_hostname = start_of_url + 7; // Move past "http://"
            char *end_of_hostname = strchr(start_of_hostname, ':');
            char *end_of_path = strchr(start_of_hostname, '/');

            if (end_of_hostname != NULL && (end_of_path == NULL || end_of_path > end_of_hostname)) {
                int hostname_length = end_of_hostname - start_of_hostname;
                strncpy(hostname, start_of_hostname, hostname_length);
                hostname[hostname_length] = '\0';

                int port_length = (end_of_path != NULL ? end_of_path : start_of_url + url_length) - end_of_hostname - 1;
                strncpy(port, end_of_hostname + 1, port_length);
                port[port_length] = '\0';
            } else {
                strcpy(port, "80");
                int hostname_length = (end_of_path != NULL ? end_of_path : start_of_url + url_length) - start_of_hostname;
                strncpy(hostname, start_of_hostname, hostname_length);
                hostname[hostname_length] = '\0';
            }

            if (end_of_path != NULL) {
                int path_length = start_of_url + url_length - end_of_path;
                strncpy(path, end_of_path, path_length);
                path[path_length] = '\0';
            } else {
                printf("Path extraction failed\n");
                return 0; // Path extraction failed
            }

            return 1; // Parsing successful
        } else {
            printf("URL extraction failed\n");
            return 0; // URL extraction failed
        }
    } else {
        printf("URL not found\n");
        return 0; // URL not found
    }
}

void handle_client(int client_fd) {
    char buffer[MAX_BUFFER_SIZE]; // Adjust buffer size as needed
    char method[16], hostname[64], port[8], path[64];
    char wholeRequest[500];
    int startPoint = 0;
    ssize_t bytes_received;

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0 )) > 0) {
        if (bytes_received == -1) {
            perror("Receive failed");
            return;
        }

        char *end_of_headers = strstr(buffer, "\r\n\r\n");
        memcpy(wholeRequest + startPoint, buffer, bytes_received);
        startPoint += bytes_received;
        if (end_of_headers != NULL) {
            printf("Printing end of headers: %s\n", end_of_headers);
            break; // Request is complete
        }
    }

    wholeRequest[startPoint] = '\0';
    // Read from the socket until the entire HTTP request is received
    if (parse_request(wholeRequest, method, hostname, port, path)) {
        printf("Method: %s, Hostname: %s, Port: %s, Path: %s\n", method, hostname, port, path);

        // Create the modified HTTP request to send to the server
        char modified_request[1024]; // Adjust size as needed
        sprintf(modified_request, "GET %s HTTP/1.0\r\nHost: %s:%s\r\nUser-Agent: %s\r\nConnection: close\r\nProxy-Connection: close\r\n\r\n",
                path, hostname, port, user_agent_hdr);

        // Create a socket to communicate with the server and forward the request
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("Socket creation error");
            return;
        }

        // Set up the server address and port to connect to using getaddrinfo
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(atoi(port)); // Convert port to network byte order

        // Resolve the hostname to an IP address using getaddrinfo
        struct addrinfo hints, *server_info;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; // Use IPv4
        hints.ai_socktype = SOCK_STREAM;

        int status;
        if ((status = getaddrinfo(hostname, port, &hints, &server_info)) != 0) {
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            close(server_fd);
            return;
        }

        // Loop through the addresses returned by getaddrinfo until a successful connection is made
        struct addrinfo *p;
        for (p = server_info; p != NULL; p = p->ai_next) {
            if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
                perror("connect");
                close(server_fd);
                continue;
            }
            break;
        }

        if (p == NULL) {
            fprintf(stderr, "Failed to connect\n");
            freeaddrinfo(server_info);
            close(server_fd);
            return;
        }

        // Send the modified request to the server
        ssize_t bytes_sent = send(server_fd, modified_request, strlen(modified_request), 0);
        if (bytes_sent < 0) {
            perror("Send error");
            close(server_fd);
            return;
        }

        // Receive and forward the server's response to the client
        char server_response[1024]; // Adjust size as needed
        ssize_t server_bytes_received;
        while ((server_bytes_received = recv(server_fd, server_response, sizeof(server_response), 0)) > 0) {
            ssize_t response_sent = send(client_fd, server_response, server_bytes_received, 0);
            if (response_sent < 0) {
                perror("Response send error");
                close(server_fd);
                close(client_fd);
                return;
            }

            memset(server_response, 0, sizeof(server_response));
        }

        if (server_bytes_received < 0) {
            perror("Server receive error");
        }

        // Close the connection to the server
        close(server_fd);

        // Close the client socket
        close(client_fd);
        return;
    } else {
        printf("Failed to parse HTTP request\n");
        // Close the client socket (moved to the end)
        close(client_fd);
        return;
    }

    if (bytes_received < 0) {
        perror("Client receive error");
    }

    // Close the client socket
    close(client_fd);
}

int open_sfd(int port) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, 10) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    return sfd;
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