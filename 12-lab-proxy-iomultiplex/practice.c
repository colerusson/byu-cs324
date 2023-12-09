#include <stdio.h>

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int complete_request_received(char *);
int parse_request(char *, char *, char *, char *, char *);
void test_parser();
void print_bytes(unsigned char *, int);


int main(int argc, char *argv[])
{
    test_parser();
    printf("%s\n", user_agent_hdr);
    return 0;
}

int complete_request_received(char *request) {
    // TODO: Implement this function
    return 0;
}

int parse_request(char *request, char *method,
                  char *hostname, char *port, char *path) {
    return 0;
}

void test_parser() {
    int i;
    char method[16], hostname[64], port[8], path[64];

    char *reqs[] = {
            "GET http://www.example.com/index.html HTTP/1.0\r\n"
            "Host: www.example.com\r\n"
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
            "Accept-Language: en-US,en;q=0.5\r\n\r\n",

            "GET http://www.example.com:8080/index.html?foo=1&bar=2 HTTP/1.0\r\n"
            "Host: www.example.com:8080\r\n"
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
            "Accept-Language: en-US,en;q=0.5\r\n\r\n",

            "GET http://localhost:1234/home.html HTTP/1.0\r\n"
            "Host: localhost:1234\r\n"
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
            "Accept-Language: en-US,en;q=0.5\r\n\r\n",

            "GET http://www.example.com:8080/index.html HTTP/1.0\r\n",

            NULL
    };

    for (i = 0; reqs[i] != NULL; i++) {
        printf("Testing %s\n", reqs[i]);
        if (parse_request(reqs[i], method, hostname, port, path)) {
            printf("METHOD: %s\n", method);
            printf("HOSTNAME: %s\n", hostname);
            printf("PORT: %s\n", port);
            printf("PATH: %s\n", path);
        } else {
            printf("REQUEST INCOMPLETE\n");
        }
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




#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

/* Recommended max object size */
#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENTS 100

#define READ_REQUEST 0
#define SEND_REQUEST 1

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

struct request_info {
    int client_fd;           // client-to-proxy socket
    int server_fd;           // proxy-to-server socket
    int state;               // current state of the request (READ_REQUEST, SEND_REQUEST, etc.)
    char buffer[MAX_BUFFER_SIZE];  // buffer to read into and write from
    // other members to track bytes read/written, method, hostname, port, path, etc.
};

int parse_request(char *, char *, char *, char *, char *);
int open_sfd(int);
void handle_client(int, int, struct request_info *);
void handle_new_clients(int, int, struct epoll_event *, struct request_info *);

// TODO: Modify this function as needed
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int server_fd = open_sfd(port);

    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        perror("Epoll creation failed");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN; // Define the event type as EPOLLIN
    event.data.fd = server_fd; // Assuming server_fd has been defined earlier

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("Epoll add error");
        exit(EXIT_FAILURE);
    }

    struct epoll_event *events = malloc(MAX_CLIENTS * sizeof(struct epoll_event));
    if (events == NULL) {
        perror("Events malloc error");
        exit(EXIT_FAILURE);
    }

    struct request_info *requests = calloc(MAX_CLIENTS, sizeof(struct request_info));
    if (requests == NULL) {
        perror("Requests calloc error");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int num_ready = epoll_wait(epoll_fd, events, MAX_CLIENTS, -1);
        if (num_ready == -1) {
            perror("Epoll wait error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_ready; ++i) {
            if (events[i].data.fd == server_fd) {
                handle_new_clients(epoll_fd, server_fd, events, requests);
            } else {
                handle_client(epoll_fd, events[i].data.fd, requests);
            }
        }
    }

    free(events);
    free(requests);
    close(server_fd);
    close(epoll_fd);
    return 0;
}

// TODO: Modify this function as needed
void handle_client(int epoll_fd, int client_fd, struct request_info *requests) {
    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (requests[i].client_fd == client_fd) {
            break;
        }
    }

    if (i == MAX_CLIENTS) {
        fprintf(stderr, "Client not found\n");
        return;
    }

    int server_fd = open_sfd(epoll_fd);
    if (server_fd == -1) {
        fprintf(stderr, "Failed to connect to server\n");
        return;
    }

    requests[i].server_fd = server_fd;
    requests[i].state = READ_REQUEST;

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("Epoll add error");
        close(server_fd);
        return;
    }

    event.events = EPOLLIN;
    event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        perror("Epoll add error");
        close(client_fd);
        return;
    }

    printf("Connected to server\n");
}


// TODO: Modify this function as needed
void handle_new_clients(int epoll_fd, int server_fd, struct epoll_event *events, struct request_info *requests) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("Accept failed");
        return;
    }

    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (requests[i].client_fd == 0) {
            requests[i].client_fd = client_fd;
            break;
        }
    }

    if (i == MAX_CLIENTS) {
        fprintf(stderr, "Too many clients\n");
        close(client_fd);
        return;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        perror("Epoll add error");
        close(client_fd);
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
