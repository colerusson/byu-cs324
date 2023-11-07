#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Retrieve environment variables
    char *content_length_str = getenv("CONTENT_LENGTH");
    char *query_string = getenv("QUERY_STRING");

    // Check if the environment variables are set
    if (content_length_str == NULL || query_string == NULL) {
        printf("Content-Type: text/plain\r\n");
        printf("\r\n");
        printf("Environment variables not set");
        return 1;
    }

    // Convert CONTENT_LENGTH to an integer
    int content_length = atoi(content_length_str);

    // Read the request body
    char *request_body = (char *)malloc(content_length + 1);
    fread(request_body, 1, content_length, stdin);
    request_body[content_length] = '\0';

    // Create the response body
    char response_body[1024]; // Adjust the size as needed
    snprintf(response_body, sizeof(response_body),
             "Hello CS324\nQuery string: %s\nRequest body: %s\n", query_string, request_body);

    // Calculate the response length
    int response_length = strlen(response_body);

    // Send response headers
    printf("Content-Type: text/plain\r\n");
    printf("Content-Length: %d\r\n", response_length);
    printf("\r\n"); // End of headers

    // Output the response body
    printf("%s", response_body);

    return 0;
}
