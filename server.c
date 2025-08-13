#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>

#include <netinet/in.h>

#define exit_error(msg)                          \
    do {                                         \
        perror(msg);                             \
        exit(EXIT_FAILURE);                      \
    } while (0)

#define PORT 7373
#define QUEUE_SIZE 20
#define BUFF_SIZE 1024

int create_tcp_server_socket(int port, int queue_size)
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) exit_error("socket()");

    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(port),
    };
    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == -1) {
        exit_error("bind()");
    }

    if (listen(sfd, queue_size) == -1) {
        exit_error("listen()");
    }

    return sfd;
}

int accept_connection(int server_socket)
{
    int peer_len = sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr;
    
    int cfd = accept(server_socket, (struct sockaddr *) &client_addr, &peer_len);
    if (cfd == -1) {
        exit_error("accept()");
    }

    assert(peer_len == sizeof(struct sockaddr_in));
    return cfd;
}

int main()
{
    int sfd = create_tcp_server_socket(PORT, QUEUE_SIZE);

    printf("Server listening on port %d...\n", PORT);

    struct pollfd *pfds = calloc(5, sizeof(struct pollfd));
    if (!pfds) exit_error("calloc()");

    pfds[0] = (struct pollfd) {
        .fd     = sfd,
        .events = POLLIN,
    };
    int npfds = 1;

    unsigned int peer_len;
    char buff[BUFF_SIZE];
    for (;;) {
        if (poll(pfds, npfds, -1) == -1) break;

        // Handle server socket inputs
        if ((pfds[0].revents & POLLIN) != 0) {
            int cfd = accept_connection(pfds[0].fd);
            pfds[npfds] = (struct pollfd) {
                .fd     = cfd,
                .events = POLLIN,
            };
            ++npfds;
        }

        // Handle connections sequentially
        for (int i = 1; i < npfds; ++i) {
            if ((pfds[i].revents & POLLIN) == 0) continue;
            int cfd = pfds[i].fd;

            bzero(buff, BUFF_SIZE);
            ssize_t nbytes = read(cfd, buff, BUFF_SIZE);
            if (nbytes == 0) {
                close(pfds[i].fd);

                for (int j = i; j < npfds - 1; ++j) {
                    pfds[j] = pfds[j + 1];
                }

                --i;
                --npfds;
                continue;
            }
            printf("Client sent: %s", buff);

            write(cfd, buff, nbytes);
        }
    }

    for (int i = 0; i < npfds; ++i) {
    if (close(pfds[i].fd) == -1) {
        exit_error("close()");
    }
    }

    free(pfds);
}
