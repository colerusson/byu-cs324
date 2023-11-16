#include <stdio.h>
#include <string.h>

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int complete_request_received(char *);
int parse_request(char *, char *, char *, char *, char *);
void test_parser();
void print_bytes(unsigned char *, int);


int main(int argc, char *argv[]) {
	test_parser();
	printf("%s\n", user_agent_hdr);
	return 0;
}

int complete_request_received(char *request) {
    char *end_of_headers = strstr(request, "\r\n\r\n");
    if (end_of_headers != NULL) {
        return 1; // Request is complete
    } else {
        return 0; // Request is not complete
    }
}

int parse_request(char *request, char *method, char *hostname, char *port, char *path) {
    // Check if the request is complete
    if (!complete_request_received(request)) {
        return 0; // Request is incomplete
    }

    // Extract method
    char *end_of_method = strstr(request, " ");
    if (end_of_method != NULL) {
        strncpy(method, request, end_of_method - request);
        method[end_of_method - request] = '\0';
    } else {
        return 0; // Method extraction failed
    }

    // Find the start of URL (after the first space)
    char *start_of_url = end_of_method + 1;

    // Extract the URL
    char *end_of_url = strstr(start_of_url, " ");
    if (end_of_url != NULL) {
        int url_length = end_of_url - start_of_url;
        char url[url_length + 1];
        strncpy(url, start_of_url, url_length);
        url[url_length] = '\0';

        // Extract hostname
        char *start_of_hostname = strstr(url, "://");
        if (start_of_hostname != NULL) {
            start_of_hostname += 3;
            char *end_of_hostname = strstr(start_of_hostname, ":");
            char *end_of_path = strstr(start_of_hostname, "/");

            if (end_of_hostname != NULL && (end_of_path == NULL || end_of_path > end_of_hostname)) {
                int hostname_length = end_of_hostname - start_of_hostname;
                strncpy(hostname, start_of_hostname, hostname_length);
                hostname[hostname_length] = '\0';

                // Extract port
                int port_length = end_of_path - end_of_hostname - 1;
                strncpy(port, end_of_hostname + 1, port_length);
                port[port_length] = '\0';
            } else {
                // Default port if not specified
                strcpy(port, "80");
                int hostname_length = (end_of_path != NULL) ? end_of_path - start_of_hostname : strlen(start_of_hostname);
                strncpy(hostname, start_of_hostname, hostname_length);
                hostname[hostname_length] = '\0';
            }

            // Extract path
            if (end_of_path != NULL) {
                int path_length = strlen(end_of_path);
                strncpy(path, end_of_path, path_length);
                path[path_length] = '\0';
            } else {
                return 0; // Path extraction failed
            }

            return 1; // Parsing successful
        } else {
            return 0; // Hostname extraction failed
        }
    } else {
        return 0; // URL extraction failed
    }
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
		printf("Testing %s", reqs[i]);
		if (parse_request(reqs[i], method, hostname, port, path)) {
			printf("METHOD: %s\n", method);
			printf("HOSTNAME: %s\n", hostname);
			printf("PORT: %s\n", port);
			printf("PATH: %s\n", path);
            printf("REQUEST COMPLETE\n\n");
		} else {
			printf("REQUEST INCOMPLETE\n\n");
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
