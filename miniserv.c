#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>


const int BUF_SIZE = 42 * 4096;
typedef struct client {
    int id;
    char msg[110000];
} t_client;

t_client clients[1024];

int max = 0, next_id = 0;
fd_set fds, wfds, rfds;
char bufRead[BUF_SIZE], bufWrite[BUF_SIZE];

void fatal_error() {
    write(2, "Fatal error\n", 12);
    exit(1);
}

void send_all(int s) {
    for (int i = 0; i <= max; i++)
        if (FD_ISSET(i, &rfds) && i != s)
            send(i, bufWrite, strlen(bufWrite), 0);
}

int main(int ac, char **av) {
    if (ac != 2) {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

    bzero(&clients, sizeof(clients));
    FD_ZERO(&fds);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        fatal_error();

    max = sockfd;
    FD_SET(sockfd, &fds);

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(2130706433);
    addr.sin_port = htons(atoi(av[1]));

    if ((bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr))) < 0)
        fatal_error();
    if (listen(sockfd, 128) < 0)
        fatal_error();

    while (1) {
        wfds = rfds = fds;
        if (select(max + 1, &wfds, &rfds, NULL, NULL) < 0)
            continue ;
        for (int s = 0; s <= max; s++) {
            if (FD_ISSET(s, &wfds) && s == sockfd) {
                int clientSock = accept(sockfd, (struct sockaddr *)&addr, &addr_len);
                if (clientSock < 0)
                    continue ;
                max = (clientSock > max) ? clientSock : max;
                clients[clientSock].id = next_id++;
                FD_SET(clientSock, &fds);
                sprintf(bufWrite, "server: client %d just arrived\n", clients[clientSock].id);
                send_all(clientSock);
                break ;
            }
            if (FD_ISSET(s, &wfds) && s != sockfd) {
                int res = recv(s, bufRead, 42*4096, 0);
                if (res <= 0) {
                    sprintf(bufWrite, "server: client %d just left\n", clients[s].id);
                    send_all(s);
                    FD_CLR(s, &fds);
                    close(s);
                    break ;
                }
                else {
                    for (int i = 0, j = strlen(clients[s].msg); i < res; i++, j++) {
                        clients[s].msg[j] = bufRead[i];
                        if (clients[s].msg[j] == '\n')
                        {
                            clients[s].msg[j] = '\0';
                            sprintf(bufWrite, "client %d: %s\n", clients[s].id, clients[s].msg);
                            send_all(s);
                            bzero(&clients[s].msg, strlen(clients[s].msg));
                            j = -1;
                        }
                    }
                    break;
                }
            }
        }
    }
    return 0;
}