#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1",
                  &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr,
                sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    printf("Connected to server.\n");
    printf("Enter ANALYZE | TEXT or bye to exit.\n");

    while (1) {
        printf("\nClient: ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\r\n")] = '\0';

        if (send(sock, input, strlen(input), 0) < 0) {
            perror("Send failed");
            break;
        }

        if (strcmp(input, "bye") == 0)
            break;

        memset(buffer, 0, sizeof(buffer));

        int valread = recv(sock, buffer,
                           sizeof(buffer) - 1, 0);

        if (valread <= 0) {
            printf("Server disconnected.\n");
            break;
        }

        buffer[valread] = '\0';
        printf("Server: %s\n", buffer);
    }

    close(sock);

    return 0;
}