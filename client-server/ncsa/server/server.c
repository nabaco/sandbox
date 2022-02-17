#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>

#include <ncsa/common/common.h>
#include <ncsa/common/log.h>

/* Defines */

#define MAX_CONNS_QUE 3

/* Global variables */

volatile bool termination = false;
volatile unsigned int conns_cnt = 0;
pthread_mutex_t conns_mutex = PTHREAD_MUTEX_INITIALIZER;


void* conn_handler(void* args) {
    int sock = *(int*)args;
    int cnt = 0;
    int ret = RETURN_ERR;
	int error = 0;
    struct timespec ts;
    char buf[10] = { '0' };
    fd_set fdset, fds;
	socklen_t len;

    pthread_mutex_lock(&conns_mutex);
	cnt = conns_cnt;
    conns_cnt++;
    pthread_mutex_unlock(&conns_mutex);

    log_info("Thread #%d started", cnt);
    send(sock, "Hi, how may I help?\n", 21, 0);

    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    while(!termination) {
        fdset = fds;
		len = sizeof(error);
		ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);

		if (ret != RETURN_OK) {
			/* there was a problem getting the error code */
			log_error("Error getting socket error code: %s", strerror(ret));
			break;
		}

		if(error == EPIPE) {
			log_info("Client #%d terminated the connection", cnt);
			break;
		} else if(error != 0) {
			/* socket has a non zero error status */
			log_error("Socket error: %s", strerror(error));
			break;
		}

        if((ret = pselect(sock + 1, &fdset, NULL, NULL, &ts, NULL)) == RETURN_ERR) {
            if(errno == EINTR) {
				/* We were interupted, let's clean up */
                break;
            } else {
                log_error("Pselect on client socket failed: %s", strerror(errno));
            }
        } else if(ret != 0) {
            /* The unexpected affect this design is that I don't need
             * a big buffer. What ever wasn't read from the socket
             * stays in que, and in the next iteration pselect will
             * activate again, causing the next part to be read,
             * and so on. */
            if((ret = recv(sock, buf, 10, 0)) == RETURN_ERR) {
                log_error("recv in #%d failed: %s", cnt, strerror(errno));
            } else if (ret == 0) {
                log_info("Client #%d terminated the connection", cnt);
                break;
            } else {
                log_debug("Received in #%d: %s", cnt, buf);
                if((ret = send(sock, buf, 10, 0)) == RETURN_ERR) {
                    if(errno == EPIPE) {
                        log_info("Client #%d terminated the connection", cnt);
                        break;
                    } else {
                        log_error("send in #%d failed: %s", cnt, strerror(errno));
                    }
                }
                memset(buf,0,10);
            }
        }
    }

    if(error == 0 || termination == true) {
		send(sock, "Bye\n", 5, 0);
	}
    if(close(sock) == RETURN_ERR) {
        log_error("Failed to close socket");
    }
    pthread_mutex_lock(&conns_mutex);
    conns_cnt--;
    pthread_mutex_unlock(&conns_mutex);
    return 0;
}

void signal_handler(int sig) {
    //TODO: Handle SIGPIPE for broken streams (reopen socket?)
    //TODO: Investigate IP_RECVERR for protocol specific errors on socket

    switch(sig){
        case SIGTERM:
        case SIGINT:
            log_info("Cleaning up before termination");
            if(termination) {
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
    char buf[10] = { '\0' };
    int ret, sock, in_sock = RETURN_ERR;
    int cnt = 0;
    struct sockaddr_in addr;
    struct sockaddr in_addr;
    socklen_t addr_len = 0;
    pthread_t conn_pt = 0;
    pthread_t conn_threads[MAX_CONNS_QUE];
    //pthread_t pt_id = 0;
    struct timespec ts;

    log_info("Server init");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == RETURN_ERR) {
        log_error("Failed top create socket: %s", strerror(errno));
        return RETURN_ERR;
    } else {
        log_info("Socket created");
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons( 8888 );

    if(bind(sock,(struct sockaddr *)&addr , sizeof(addr)) == RETURN_ERR) {
        log_error("Socket binding failed: %s", strerror(errno));
        return RETURN_ERR;
    } else {
        log_info("Socket binded");
    }

    if(listen(sock, MAX_CONNS_QUE) == RETURN_ERR) {
        log_error("Failed to put socket in listening mode: %s", strerror(errno));
        return RETURN_ERR;
    } else {
        log_info("Socket is ready to accept connections");
    }

    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        log_error("Failed to register signal handler - aborting: %s", strerror(errno));
    }

    if(signal(SIGTERM, signal_handler) == SIG_ERR) {
        log_error("Failed to register signal handler - aborting: %s", strerror(errno));
    }

    ts.tv_sec = 2;
    ts.tv_nsec = 0;

    fd_set fdset, fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    FD_SET(STDIN_FILENO, &fds);
    addr_len = sizeof(in_addr);
    while(!termination) {
        fdset = fds;
        if ((ret = pselect(sock + 1, &fdset, NULL, NULL, &ts, NULL)) == RETURN_ERR) {
            if(errno == EINTR) {
                break;
            } else {
                log_error("Main pselect failed: %s", strerror(errno));
            }
        } else if(ret > 0) {
            if(FD_ISSET(STDIN_FILENO, &fdset)) {
                read(STDIN_FILENO, buf, 1);
                if(strcmp(buf,"q") == 0) {
                    log_info("Quitting");
                    termination = true;
                    break;
                }
            } else if(FD_ISSET(sock, &fdset)) {
                log_debug("Sock is ready for reading");
                log_info("Received a new connection");
                in_sock = accept(sock, &in_addr, &addr_len);
                if(conns_cnt == MAX_CONNS_QUE) {
                    log_error("Can't receive more connections - ignoring");
                    send(in_sock,"Can't accept more connections\n", 31, 0);
                    close(in_sock);
                    continue;
                }
                if(pthread_create(&conn_pt, NULL, conn_handler, (void *)&in_sock) == RETURN_ERR) {
                    log_error("Failed to start connection handler thread");
                } else {
                    log_debug("Created thread #%d",cnt);
                    cnt = conns_cnt;
                    conn_threads[cnt] = conn_pt;
                }
            }
        }
    }
    cnt = conns_cnt;
    for(int i=0; i<cnt; i++) {
        log_debug("Waiting for thread #%d to finish", i);
        pthread_join(conn_threads[i], NULL);
    }
    log_info("Closing main socket");
    close(sock);

    return RETURN_OK;
}
