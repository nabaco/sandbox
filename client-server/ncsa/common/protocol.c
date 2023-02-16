#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <time.h>
#include "common.h"
#include "log.h"
#include "protocol.h"

int recv_msg(int sock, msg_t* msg) {
    unsigned int ret = RETURN_ERR;
    int error = 0;
    socklen_t len = sizeof(error);
    char* buf = NULL;

    ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);

    if(ret != RETURN_OK) {
        /* there was a problem getting the error code */
        log_error("Error getting socket error code: %s", strerror(ret));
        goto err;
    }

    if(error == 0 || len == 0) {
        log_debug("No errors on socket");
    } else if(error == EPIPE) {
        log_info("Client terminated the connection");
        ret = RETURN_TERM;
        goto err;
    } else if(error != 0) {
        /* socket has a non zero error status */
        log_error("Socket error: %s", strerror(error));
        ret = RETURN_ERR;
        goto err;
    }

    if((ret = recv(sock, &msg->header, sizeof(msg_hdr_t), 0)) == RETURN_ERR) {
        log_error("recv failed: %s", strerror(errno));
        ret = RETURN_ERR;
        goto err;
    } else if(ret == 0) {
        log_info("Client terminated the connection");
        ret = RETURN_TERM;
        goto err;
    } else if(ret < sizeof(msg_hdr_t)) {
        log_error("Received bad packet, dropping");
        ret = RETURN_ERR;
        goto err;
    } else {
        log_info("ret: %d\n", ret);
        //log_debug("Received in #%d: %s", cnt, buf);
        if(msg->header.version && msg->header.timestamp && msg->header.msg_len) {
            buf = (char*)malloc(msg->header.msg_len);
            if((ret = recv(sock, buf, msg->header.msg_len, 0)) == RETURN_ERR) {
                log_error("recv of message body failed: %s", strerror(errno));
                free(buf);
                goto err;
            }
            msg->body = buf;
            buf = NULL;
            log_debug("Recieved: version - %d, timestamp - %d, msg_len - %d, msg - %s",
                    msg->header.version, msg->header.timestamp, msg->header.msg_len, msg->body);
            ret = RETURN_OK;
        } else {
            log_error("Back packet");
            ret = RETURN_ERR;
        }
    }
err:
    return ret;
}

int send_msg(int sock, char* text) {
    int ret = RETURN_ERR;
    msg_hdr_t msg;
    time_t ts;

    if(!text) {
        log_error("Received an empty text");
        return RETURN_ERR;
    }

    ts = time(NULL);

    msg.version = NCSA_PROTOCOL_VERSION;
    msg.timestamp = ts;
    msg.msg_len = strlen(text);

    send(sock, &msg, sizeof(msg_hdr_t), 0);
    send(sock, text, msg.msg_len, 0);

    return ret;
}
