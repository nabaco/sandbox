#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ncsa/common/common.h>
#include <ncsa/common/log.h>
#include <ncsa/common/protocol.h>
/* TODO: Add log4c */

/* Defines */
#define INPUT_BUFFER_SIZE 10

/* Global variables */
volatile bool termination = false;

void signal_handler(int sig) {
    //TODO: Handle SIGPIPE for broken streams (reopen socket?)
    //TODO: Investigate IP_RECVERR for protocol specific errors on socket

    switch(sig){
        case SIGTERM:
        case SIGINT:
            log_info("Cleaning up before termination");
            if (termination) {
                /* Received a termination signal while handling the previous one, must be urgent */
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
    char buf[INPUT_BUFFER_SIZE] = { '\0' };
    int ret, sock = RETURN_ERR;
    msg_t msg;
    struct sockaddr_in addr;
    struct timespec ts;
    fd_set fdset, fds;

    log_info("Starting Client\n");

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == RETURN_ERR) {
        log_error("Failed top create socket: %s", strerror(errno));
        return RETURN_ERR;
    } else {
        log_info("Socket created");
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons( 8888 );

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < RETURN_OK) {
        log_error("Failed to connect to server: %s\n", strerror(errno));
        return RETURN_ERR;
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        log_error("Failed to register signal handler - aborting: %s", strerror(errno));
        return RETURN_ERR;
    }

    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        log_error("Failed to register signal handler - aborting: %s", strerror(errno));
        return RETURN_ERR;
    }

    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    FD_SET(STDIN_FILENO, &fds);
    while(!termination) {
        fdset = fds;
        if ((ret = pselect(sock + 1, &fdset, NULL, NULL, &ts, NULL)) < 0) {
            if (errno == EINTR) {
                break;
            } else {
                log_error("Main pselect failed: %s", strerror(errno));
            }
        } else if (ret == 0) {
                continue;
        } else if (ret > 0) {
            if (FD_ISSET(STDIN_FILENO, &fdset)) {
                //buf = { '\0' };
                read(STDIN_FILENO, buf, 9);
                if (strcmp(buf,"q") == 0) {
                    log_info("Quitting");
                    termination = true;
                    break;
                } else {
                    send_msg(sock, buf);
                    memset(buf, 0, INPUT_BUFFER_SIZE);
                }
            } else if (FD_ISSET(sock, &fdset)) {
                memset((void*)&msg, 0, sizeof(msg_t));
                if (recv_msg(sock, &msg) == RETURN_OK) {
                    printf("S: %s", msg.body);
                } else {
                    log_error("Failed to receive message");
                }
            }
        }
    }

    log_info("Disconnecting");
    if ((ret = close(sock)) != RETURN_OK) {
        log_error("Failed to disconnect - %s", strerror(errno));
    }

    return ret;
}
