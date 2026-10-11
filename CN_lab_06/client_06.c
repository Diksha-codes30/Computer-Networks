#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PORT 8080
#define DATA_SIZE 1024
#define ID_SIZE 20
#define RESPONSE_SIZE 200

struct Request {
    char id[ID_SIZE];
    char data[DATA_SIZE];
};

int read_full(int fd, void *buf, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t n = read(fd, (char *)buf + total, size - total);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

int write_full(int fd, const void *buf, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t n = write(fd, (const char *)buf + total,
                          size - total);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

void handle_client(int client_fd) {
    struct Request req;
    char response[RESPONSE_SIZE] = "Record Not Found";

    if (read_full(client_fd, &req, sizeof(req)) < 0) {
        close(client_fd);
        _exit(1);
    }

    req.id[ID_SIZE - 1] = '\0';
    req.data[DATA_SIZE - 1] = '\0';

    char *line = strtok(req.data, "\n");

    while (line != NULL) {
        int sid;
        char name[50], branch[50];

        if (sscanf(line, "%d %49s %49s",
                   &sid, name, branch) == 3) {
            char requested_id[ID_SIZE];
            snprintf(requested_id, sizeof(requested_id),
                     "%d", sid);

            if (strcmp(requested_id, req.id) == 0) {
                snprintf(response, sizeof(response),
                         "Record Found : %s", line);
                break;
            }
        }

        line = strtok(NULL, "\n");
    }

    write_full(client_fd, response, sizeof(response));
    close(client_fd);
    _exit(0);
}

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    signal(SIGCHLD, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
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
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd,
                           (struct sockaddr *)&address,
                           &addrlen);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            close(client_fd);
        } else if (pid == 0) {
            close(server_fd);
            handle_client(client_fd);
        } else {
            close(client_fd);
        }

        addrlen = sizeof(address);
    }

    close(server_fd);
    return 0;
}