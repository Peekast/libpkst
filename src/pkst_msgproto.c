#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "pkst_msgproto.h"


int pkst_send_msg(int socket, PKSTMgsProto *hdr, const char *msg_body) {
    int n, len = strlen(msg_body) + 1 + sizeof(PKSTMgsProto);
    char *buffer = calloc(1,len);
    if (!buffer) {
        return -1;
    }
    
    memcpy(buffer, hdr, sizeof(PKSTMgsProto));
    memcpy(&buffer[sizeof(PKSTMgsProto)], msg_body, hdr->msg_size);

    n = write(socket, buffer, len);
    free(buffer);
    if (n < 0) 
        return errno;
    if (n != len)
        return -1;
    return 0;   
}

int pkst_send_data(int socket, const char *msg, uint8_t type) {
    PKSTMgsProto hdr;

    hdr.protocol_id = PROTOCOL_IDENTIFIER;
    hdr.msg_type = type;
    hdr.msg_size = strlen(msg) + 1;

    return pkst_send_msg(socket, &hdr, msg);
}