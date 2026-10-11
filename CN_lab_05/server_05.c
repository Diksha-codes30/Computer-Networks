#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 1024

int count_words(char *str) {
    int count = 0, in_word = 0;

    while (*str) {
        if (!isspace((unsigned char)*str)) {
            if (!in_word) {
                count++;
                in_word = 1;
            }
        } else {
            in_word = 0;
        }
        str++;
    }

    return count;
}

int count_vowels(char *str) {
    int count = 0;

    while (*str) {
        char ch = tolower((unsigned char)*str);

        if (ch == 'a' || ch == 'e' || ch == 'i' ||
            ch == 'o' || ch == 'u') {
            count++;
        }

        str++;
    }

    return count;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    new_socket = accept(server_fd, NULL, NULL);
    if (new_socket < 0) {
        perror("Accept failed");
        close(server_fd);
        return 1;
    }

    printf("Client connected.\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        int valread = recv(new_socket, buffer,
                           sizeof(buffer) - 1, 0);

        if (valread <= 0)
            break;

        buffer[valread] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';

        if (strcmp(buffer, "bye") == 0)
            break;

        char *separator = strchr(buffer, '|');

        if (separator == NULL) {
            char *msg = "Invalid Command";
            send(new_socket, msg, strlen(msg), 0);
            continue;
        }

        *separator = '\0';

        char *command = buffer;
        char *text = separator + 1;

        while (isspace((unsigned char)*command))
            command++;

        char *end = command + strlen(command);
        while (end > command &&
               isspace((unsigned char)*(end - 1))) {
            *(--end) = '\0';
        }

        while (isspace((unsigned char)*text))
            text++;

        if (strcmp(command, "ANALYZE") == 0) {
            int chars = strlen(text);
            int words = count_words(text);
            int vowels = count_vowels(text);

            char response[BUFFER_SIZE];

            snprintf(response, sizeof(response),
                     "Chars = %d, Words = %d, Vowels = %d",
                     chars, words, vowels);

            send(new_socket, response, strlen(response), 0);
        } else {
            char *msg = "Invalid Command";
            send(new_socket, msg, strlen(msg), 0);
        }
    }

    printf("Client disconnected.\n");
    close(new_socket);
    close(server_fd);

    return 0;
}