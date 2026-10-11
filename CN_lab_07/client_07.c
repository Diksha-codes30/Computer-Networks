#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 2048

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in server;
    socklen_t server_len = sizeof(server);
    char message[BUFSIZE], buffer[BUFSIZE];
    char request[BUFSIZE + 100];

    if (argc != 3) {
        printf("Usage: %s <server_ip> <roll>\n", argv[0]);
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(30000 + atoi(argv[2]) % 1000);

    if (inet_pton(AF_INET, argv[1],
                  &server.sin_addr) != 1) {
        printf("Invalid server IP address\n");
        close(sockfd);
        return 1;
    }

    printf("Connected to server %s\n", argv[1]);
    printf("Enter a request (type EXIT to quit):\n");

    while (1) {
        printf("> ");

        if (fgets(message, sizeof(message), stdin) == NULL) {
            strcpy(message, "EXIT");
        }

        message[strcspn(message, "\n")] = '\0';

        snprintf(request, sizeof(request), "%s#%s",
                 argv[2], message);

        if (sendto(sockfd, request, strlen(request), 0,
                   (struct sockaddr *)&server,
                   server_len) < 0) {
            perror("sendto");
            break;
        }

        int bytes = recvfrom(sockfd, buffer, BUFSIZE - 1, 0,
                             NULL, NULL);

        if (bytes < 0) {
            perror("recvfrom");
            break;
        }

        buffer[bytes] = '\0';
        printf("Server response: %s\n", buffer);

        if (strcmp(message, "EXIT") == 0)
            break;
    }

    close(sockfd);
    return 0;
}