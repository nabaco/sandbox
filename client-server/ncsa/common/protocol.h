#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#define NCSA_PROTOCOL_VERSION 1

typedef struct {
    short int version;
    int timestamp;
    int msg_len;
} __attribute__((packed)) msg_hdr_t;


typedef struct {
    msg_hdr_t header;
    char* body;
} __attribute__((packed)) msg_t;

int recv_msg(int sock, msg_t* msg);
int send_msg(int sock, char* msg);

#endif /* ifndef _PROTOCOL_H_ */
