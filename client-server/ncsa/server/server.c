// #include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ncsa/common/common.h>
#include <ncsa/common/log.h>
#include <ncsa/common/protocol.h>

/* Defines */

#define MAX_CONNS_QUE 3

/* Global variables */
volatile bool termination = false;

void signal_handler(int sig) {
    // TODO: Handle SIGPIPE for broken streams (reopen socket?)
    // TODO: Investigate IP_RECVERR for protocol specific errors on socket

    switch (sig) {
        case SIGTERM:
        case SIGINT:
            log_info("Cleaning up before termination");
            if (termination) {
                /* Received a termination signal while handling the previous one, must be
                 * urgent */
                exit(RETURN_OK);
            } else {
                termination = true;
                signal(SIGINT, SIG_DFL);
                signal(SIGTERM, SIG_DFL);
            }
            break;
        default:
            break;
    }
}

int main(void) {
    char buf[10] = {'\0'};
    int ret, sock, in_sock = RETURN_ERR;
    int conns_cnt = 0;
    int conn_fds[MAX_CONNS_QUE] = {0};
    struct sockaddr_in addr;
    struct sockaddr in_addr;
    socklen_t addr_len = 0;
    struct timespec ts;
    msg_t msg = {0};

    log_info("Server init");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == RETURN_ERR) {
        log_error("Failed top create socket: %s", strerror(errno));
        return RETURN_ERR;
    } else {
        log_info("Socket created");
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8888);

    int yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
            RETURN_ERR) {
        log_error("Socket configuration failed: %s", strerror(errno));
        return RETURN_ERR;
    }
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == RETURN_ERR) {
        log_error("Socket binding failed: %s", strerror(errno));
        return RETURN_ERR;
    } else {
        log_info("Socket binded");
    }

    if (listen(sock, MAX_CONNS_QUE) == RETURN_ERR) {
        log_error("Failed to put socket in listening mode: %s", strerror(errno));
        return RETURN_ERR;
    } else {
        log_info("Socket is ready to accept connections");
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        log_error("Failed to register signal handler - aborting: %s",
                strerror(errno));
    }

    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        log_error("Failed to register signal handler - aborting: %s",
                strerror(errno));
    }

    ts.tv_sec = 2;
    ts.tv_nsec = 0;

    fd_set fdset, fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    FD_SET(STDIN_FILENO, &fds);
    addr_len = sizeof(in_addr);
    while (!termination) {
        fdset = fds;
        if ((ret = pselect(sock + conns_cnt + 1, &fdset, NULL, NULL, &ts, NULL)) < 0) {
            if (errno == EINTR) {
                break;
            } else {
                log_error("Main pselect failed: %s", strerror(errno));
            }
        } else if (ret == 0) {
            continue;
        } else if (ret > 0) {
            if (FD_ISSET(STDIN_FILENO, &fdset)) {
                read(STDIN_FILENO, buf, 1);
                if (strcmp(buf, "q") == 0) {
                    log_info("Quitting");
                    termination = true;
                    break;
                }
            } else if (FD_ISSET(sock, &fdset)) {
                log_debug("Received a new connection");
                in_sock = accept(sock, &in_addr, &addr_len);
                if (conns_cnt == MAX_CONNS_QUE) {
                    log_error("Can't receive more connections - ignoring");
                    send_msg(in_sock, "Can't accept more connections\n");
                    close(in_sock);
                    continue;
                }
                FD_SET(in_sock, &fds);
                log_debug("New connection #%d", conns_cnt);
                send_msg(in_sock, "Welcome Client!");
                conn_fds[conns_cnt] = in_sock;
                conns_cnt++;
            } else {
                for (int c = 0; c < conns_cnt; c++) {
                    if (FD_ISSET(conn_fds[c], &fdset)) {
                        if ((ret = recv_msg(conn_fds[c], &msg)) == RETURN_ERR) {
                            log_error("Failed to receive message");
                        } else if (ret == RETURN_TERM) {
                            log_info("Connection #%d closed", c);
                            FD_CLR(conn_fds[c], &fds);
                            if ((ret = close(conn_fds[c])) == RETURN_ERR) {
                                log_error("Failed to close connection #%d - %s", c,
                                        strerror(errno));
                            }
                            conn_fds[c] = -1;
                            conns_cnt--;
                        } else {
                            printf("[%s]\nC: %s", time(msg.header.timestamp), msg.body);
                            free(msg.body);
                            msg.body = NULL;
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < conns_cnt; i++) {
        log_debug("Closing connection #%d", i);
        if ((ret = close(conn_fds[i])) == RETURN_ERR) {
            log_error("Failed to close connection #%d - %s", i, strerror(errno));
        }
    }
    log_info("Closing main socket");
    close(sock);

    return RETURN_OK;
}
