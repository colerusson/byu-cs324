#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_EVENTS 10
#define MAX_CLIENTS 100
#define MAX_BUFFER_SIZE 1024
#define BUFFER_SIZE 5

enum RequestState {
    READ_REQUEST,
    SEND_REQUEST,
    READ_RESPONSE,
    SEND_RESPONSE,
};

struct request_info {
    int client_fds[MAX_CLIENTS];
    int server_fds[MAX_CLIENTS];
    enum RequestState state[MAX_CLIENTS];
    char client_request_buffer[MAX_CLIENTS][MAX_BUFFER_SIZE];
    char server_response_buffer[MAX_CLIENTS][MAX_BUFFER_SIZE];
    int client_count;
    // Add other necessary information to manage multiple requests
};

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int parse_request(char *, char *, char *, char *, char *);
int open_sfd(int);
void handle_client(int client_fd, struct request_info *requests, int index);
void handle_new_clients(int server_fd, int epoll_fd, struct request_info *requests, int);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int server_fd = open_sfd(port);

    struct request_info requests;
    memset(&requests, 0, sizeof(requests));

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Epoll creation failed");
        exit(EXIT_FAILURE);
    }

    // Rest of the code for creating and binding server_fd

    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("Epoll control failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int num_ready_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_ready_fds == -1) {
            perror("Epoll wait failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_ready_fds; ++i) {
            if (events[i].data.fd == server_fd) {
                handle_new_clients(server_fd, epoll_fd, &requests, i);
            } else {
                int client_fd = events[i].data.fd;
                int index = -1;
                for (int j = 0; j < requests.client_count; ++j) {
                    if (requests.client_fds[j] == client_fd) {
                        index = j;
                        break;
                    }
                }
                if (index != -1) {
                    handle_client(client_fd, &requests, index);
                    close(client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                }
            }
        }
    }

    // Cleanup code
    close(server_fd);
    return 0;
}

void handle_client(int client_fd, struct request_info *requests, int index) {
    char buffer[MAX_BUFFER_SIZE]; // Adjust buffer size as needed
    char method[16], hostname[64], port[8], path[64];
    char wholeRequest[500];
    int startPoint = 0;
    ssize_t bytes_received;

    // Existing code for receiving the request...

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
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

    // Null terminate the request string
    wholeRequest[startPoint] = '\0';

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

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(atoi(port)); // Convert port to network byte order

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

        ssize_t bytes_sent = send(server_fd, modified_request, strlen(modified_request), 0);
        if (bytes_sent < 0) {
            perror("Send error");
            close(server_fd);
            return;
        }

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

        close(server_fd);
        close(client_fd);
        return;
    } else {
        printf("Failed to parse HTTP request\n");
        close(client_fd);
        return;
    }

    if (bytes_received < 0) {
        perror("Client receive error");
    }

    close(client_fd);
}

void handle_new_clients(int server_fd, int epoll_fd, struct request_info *requests, int index) {
    struct epoll_event event;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("Accept failed");
        return;
    }

    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        perror("Epoll control for client_fd failed");
        close(client_fd);
        return;
    }

    // Your existing logic to handle the client request
    handle_client(client_fd, requests, index); // Pass the 'index' parameter
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