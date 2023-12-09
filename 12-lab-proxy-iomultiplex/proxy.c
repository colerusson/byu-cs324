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

// Structure to hold information for managing a single request
struct request_info {
    int client_fd;
    int server_fd;
    int state;
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received_from_client;
    int bytes_to_send_to_server;
    int bytes_sent_to_server;
    int bytes_received_from_server;
    int bytes_sent_to_client;
};

#define READ_REQUEST 1
#define SEND_REQUEST 2
#define READ_RESPONSE 3
#define SEND_RESPONSE 4

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int parse_request(char *, char *, char *, char *, char *);
int open_sfd(int);
void handle_client(struct request_info *);
void handle_new_clients(int, int);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Epoll creation failed");
        exit(EXIT_FAILURE);
    }

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
                handle_new_clients(server_fd, epoll_fd);
            } else {
                struct request_info *request = events[i].data.ptr;
                handle_client(request);
                free(request); // Free the allocated request_info struct
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, request->client_fd, NULL);
            }
        }
    }

    // Cleanup code (not reached in this simplified example)
    close(server_fd);
    return 0;
}

void handle_client(struct request_info *request) {
    ssize_t bytes_received;
    char modified_request[MAX_BUFFER_SIZE];

    switch (request->state) {
        case READ_REQUEST:
            while ((bytes_received = recv(request->client_fd, request->buffer + request->bytes_received_from_client, sizeof(request->buffer) - request->bytes_received_from_client, 0)) > 0) {
                request->bytes_received_from_client += bytes_received;
                // Check for end of headers to determine complete request
                char *end_of_headers = strstr(request->buffer, "\r\n\r\n");
                if (end_of_headers != NULL) {
                    // Parse the request here and set up modified_request
                    // Update request->state accordingly (e.g., request->state = SEND_REQUEST;)
                    request->state = SEND_REQUEST;
                    break;
                }
            }
            if (bytes_received == -1) {
                perror("Receive failed");
                // Handle error
                close(request->client_fd);
                free(request);
                return;
            }
            break;

        case SEND_REQUEST:
            // Create modified request and send to server
            // Update request->state accordingly (e.g., request->state = READ_RESPONSE;)
            sprintf(modified_request, "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n");

            ssize_t bytes_sent = send(request->server_fd, modified_request, strlen(modified_request), 0);
            if (bytes_sent == -1) {
                perror("Send error");
                // Handle error
                close(request->client_fd);
                close(request->server_fd);
                free(request);
                return;
            }
            request->bytes_sent_to_server += bytes_sent;
            request->state = READ_RESPONSE;
            break;

        case READ_RESPONSE:
            // Receive response from server and forward to client
            bytes_received = recv(request->server_fd, request->buffer + request->bytes_received_from_server, sizeof(request->buffer) - request->bytes_received_from_server, 0);
            if (bytes_received > 0) {
                ssize_t response_sent = send(request->client_fd, request->buffer + request->bytes_received_from_server, bytes_received, 0);
                if (response_sent == -1) {
                    perror("Response send error");
                    // Handle error
                    close(request->client_fd);
                    close(request->server_fd);
                    free(request);
                    return;
                }
                request->bytes_received_from_server += bytes_received;
                request->bytes_sent_to_client += response_sent;
            } else if (bytes_received == 0) {
                // Server has closed the connection
                close(request->server_fd);
                request->server_fd = -1;
                request->state = SEND_RESPONSE;
            } else {
                perror("Server receive error");
                // Handle error
                close(request->client_fd);
                close(request->server_fd);
                free(request);
                return;
            }
            break;

        case SEND_RESPONSE:
            // Send server response to client
            while (request->bytes_sent_to_client < request->bytes_received_from_server) {
                ssize_t remaining_data = request->bytes_received_from_server - request->bytes_sent_to_client;
                ssize_t response_sent = send(request->client_fd, request->buffer + request->bytes_sent_to_client, remaining_data, 0);
                if (response_sent == -1) {
                    perror("Response send error");
                    // Handle error
                    close(request->client_fd);
                    close(request->server_fd);
                    free(request);
                    return;
                }
                request->bytes_sent_to_client += response_sent;
            }

            // Reset request state and clean up resources
            close(request->client_fd);
            if (request->server_fd != -1)
                close(request->server_fd);
            free(request); // Free the allocated request_info struct
            return;


        default:
            // Invalid state or completed request
            close(request->client_fd);
            if (request->server_fd != -1)
                close(request->server_fd);
            free(request); // Free the allocated request_info struct
            return;
    }
}


void handle_new_clients(int server_fd, int epoll_fd) {
    struct epoll_event event;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("Accept failed");
        return;
    }

    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = malloc(sizeof(struct request_info));
    if (event.data.ptr == NULL) {
        perror("Memory allocation failed");
        close(client_fd);
        return;
    }

    struct request_info *new_request = event.data.ptr;
    new_request->client_fd = client_fd;
    new_request->server_fd = -1; // Initialize other fields as needed
    new_request->state = 1; // Set the initial state
    // Initialize other fields in the request_info struct

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        perror("Epoll control for client_fd failed");
        close(client_fd);
        free(new_request);
        return;
    }
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