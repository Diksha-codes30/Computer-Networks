#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ROLL "24105003"  // Change to your roll number

// Roll: 24105003 Service Type: A d = 0

#define BUFSIZE 2048

int get_d(void)
{
    int n = atoi(ROLL);
    int last = n % 1000;
    return ((last / 100) + (last / 10 % 10) + (last % 10)) % 3;
}

void process_request(char *msg, char *reply)
{
    int d = get_d();

    if (d == 0) {
        char *sep = strchr(msg, '|');

        if (!sep) {
            strcpy(reply, "ERROR: Invalid request");
            return;
        }

        *sep = '\0';
        char *text = sep + 1;

        if (strcmp(msg, "UP") == 0) {
            for (int i = 0; text[i]; i++)
                text[i] = toupper((unsigned char)text[i]);
        } else if (strcmp(msg, "LOW") == 0) {
            for (int i = 0; text[i]; i++)
                text[i] = tolower((unsigned char)text[i]);
        } else if (strcmp(msg, "REV") == 0) {
            int len = strlen(text);
            for (int i = 0; i < len / 2; i++) {
                char temp = text[i];
                text[i] = text[len - 1 - i];
                text[len - 1 - i] = temp;
            }
        } else {
            strcpy(reply, "ERROR: Invalid command");
            return;
        }

        snprintf(reply, BUFSIZE, "%s", text);
    }
    else if (d == 1) {
        char op[20], numbers[BUFSIZE];
        int n;

        if (sscanf(msg, "%19[^|]|%d|%[^\n]",
                   op, &n, numbers) != 3 ||
            n <= 0 || n > 500) {
            strcpy(reply, "ERROR: Invalid request");
            return;
        }

        char *token = strtok(numbers, " ");
        long long sum = 0, value, min = 0, max = 0;
        int count = 0;

        while (token != NULL && count < n) {
            char *end;
            value = strtoll(token, &end, 10);

            if (*end != '\0') {
                strcpy(reply, "ERROR: Invalid integer");
                return;
            }

            if (count == 0)
                min = max = value;

            if (value < min) min = value;
            if (value > max) max = value;

            sum += value;
            count++;
            token = strtok(NULL, " ");
        }

        if (count != n || token != NULL) {
            strcpy(reply, "ERROR: Incorrect number of integers");
        } else if (strcmp(op, "SUM") == 0) {
            snprintf(reply, BUFSIZE, "%lld", sum);
        } else if (strcmp(op, "MAX") == 0) {
            snprintf(reply, BUFSIZE, "%lld", max);
        } else if (strcmp(op, "MIN") == 0) {
            snprintf(reply, BUFSIZE, "%lld", min);
        } else if (strcmp(op, "AVG") == 0) {
            snprintf(reply, BUFSIZE, "%.2f",
                     (double)sum / n);
        } else {
            strcpy(reply, "ERROR: Invalid operation");
        }
    }
    else {
        int len = strlen(msg);
        int vowels = 0, consonants = 0, digits = 0;

        for (int i = 0; msg[i]; i++) {
            unsigned char c = (unsigned char)msg[i];

            if (isalpha(c)) {
                c = tolower(c);
                if (c == 'a' || c == 'e' || c == 'i' ||
                    c == 'o' || c == 'u')
                    vowels++;
                else
                    consonants++;
            } else if (isdigit(c)) {
                digits++;
            }
        }

        snprintf(reply, BUFSIZE,
                 "LEN=%d;VOWELS=%d;CONSONANTS=%d;DIGITS=%d",
                 len, vowels, consonants, digits);
    }
}

int main(void)
{
    int sockfd;
    struct sockaddr_in server, client;
    socklen_t client_len;
    char buffer[BUFSIZE], reply[BUFSIZE];

    int last = atoi(ROLL) % 1000;
    int port = 30000 + last;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server,
             sizeof(server)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    printf("Server running on port %d\n", port);

    while (1) {
        client_len = sizeof(client);

        int bytes = recvfrom(sockfd, buffer, BUFSIZE - 1, 0,
                             (struct sockaddr *)&client,
                             &client_len);

        if (bytes < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[bytes] = '\0';

        printf("\nClient: %s:%d\n",
               inet_ntoa(client.sin_addr),
               ntohs(client.sin_port));

        char *sep = strchr(buffer, '#');

        if (!sep) {
            strcpy(reply, "ERROR: Unauthorized client");
        } else {
            *sep = '\0';

            if (strcmp(buffer, ROLL) != 0) {
                strcpy(reply, "ERROR: Unauthorized client");
            } else {
                char *msg = sep + 1;
                printf("Received: %s\n", msg);

                if (strcmp(msg, "EXIT") == 0) {
                    strcpy(reply, "Server shutting down");
                    sendto(sockfd, reply, strlen(reply), 0,
                           (struct sockaddr *)&client, client_len);
                    printf("Sent: %s\n", reply);
                    break;
                }

                process_request(msg, reply);
            }
        }

        sendto(sockfd, reply, strlen(reply), 0,
               (struct sockaddr *)&client, client_len);

        printf("Sent response: %s\n", reply);
    }

    close(sockfd);
    printf("Server stopped.\n");
    return 0;
}