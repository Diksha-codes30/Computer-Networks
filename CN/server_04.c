#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void find_student(char *reg_no, char *response)
{
    FILE *fp = fopen("students.txt", "r");

    if (fp == NULL) {
        strcpy(response, "Error opening file");
        return;
    }

    char file_reg[50], name[50];
    char branch[50], college[100];
    int found = 0;

    while (fscanf(fp, "%49s %49s %49s %99s",
                  file_reg, name, branch, college) == 4) {

        if (strcmp(file_reg, reg_no) == 0) {
            snprintf(response, BUFFER_SIZE,
                     "Name: %s, Branch: %s, College: %s",
                     name, branch, college);
            found = 1;
            break;
        }
    }

    if (!found) {
        strcpy(response, "Data not found");
    }

    fclose(fp);
}

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

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

    client_fd = accept(server_fd,
                       (struct sockaddr *)&address, &addrlen);

    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        return 1;
    }

    printf("Client connected.\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        int n = recv(client_fd, buffer,
                     sizeof(buffer) - 1, 0);

        if (n <= 0) {
            break;
        }

        buffer[n] = '\0';

        if (strcmp(buffer, "bye") == 0) {
            printf("Client disconnected.\n");
            break;
        }

        find_student(buffer, response);

        if (send(client_fd, response,
                 strlen(response), 0) < 0) {
            perror("Send failed");
            break;
        }
    }

    close(client_fd);
    close(server_fd);

    printf("Connection closed.\n");
    return 0;
}