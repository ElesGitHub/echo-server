#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <da.h>

#define exit_error(msg)                          \
    do {                                         \
        perror(msg);                             \
        exit(EXIT_FAILURE);                      \
    } while (0)

#define new_pollfd(_fd, _events) ((struct pollfd) { .fd = _fd, .events = _events })

#define PORT 7373
#define QUEUE_SIZE 20
#define BUFF_SIZE 1024

typedef struct {
    struct pollfd *items;
    size_t         count;
    size_t         capacity;
} PollFDs;

int tcp_listen(int32_t address, int16_t port, int queue_size)
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) exit_error("socket()");

    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(address),
        .sin_port        = htons(port),
    };
    
    int opt = 1;
    if (setsockopt(
                sfd,
                SOL_SOCKET,
                SO_REUSEADDR,
                &opt,
                sizeof(opt))) {
        exit_error("setsockopts()");
    }

    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == -1) {
        exit_error("bind()");
    }

    if (listen(sfd, queue_size) == -1) {
        exit_error("listen()");
    }

    return sfd;
}

int tcp_accept(int server_socket)
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
    int server_fd = tcp_listen(INADDR_ANY, PORT, QUEUE_SIZE);

    printf("Server listening on port %d...\n", PORT);

    PollFDs pfds = {0};
    da_append(&pfds, new_pollfd(server_fd, POLLIN));

    char buff[BUFF_SIZE];
    for (;;) {
        if (poll(pfds.items, pfds.count, -1) == -1) break;

        // Handle server socket inputs
        if ((pfds.items[0].revents & POLLIN) != 0) {
            int cfd = tcp_accept(pfds.items[0].fd);
            da_append(&pfds, new_pollfd(cfd, POLLIN));
        }

        // Handle connections sequentially
        for (int i = 1; i < pfds.count; ++i) {
            if ((pfds.items[i].revents & POLLIN) == 0) continue;
            int cfd = pfds.items[i].fd;

            bzero(buff, BUFF_SIZE);
            ssize_t nbytes = read(cfd, buff, BUFF_SIZE);
            if (nbytes == 0) {
                close(pfds.items[i].fd);

                for (int j = i; j < pfds.count - 1; ++j) {
                    pfds.items[j] = pfds.items[j + 1];
                }

                --i;
                --pfds.count;
                continue;
            }
            printf("Client sent: %s", buff);

            write(cfd, buff, nbytes);
        }
    }

    for (int i = 0; i < pfds.count; ++i) {
        if (close(pfds.items[i].fd) == -1) {
            exit_error("close()");
        }
    }

    free(pfds.items);
}
