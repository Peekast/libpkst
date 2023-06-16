#ifndef _PKST_MSG_PROTO_H
#define _PKST_MSG_PROTO_H 1

#define PROTOCOL_IDENTIFIER       0xAA
#define MESSAGE_TYPE_KV           0x01
#define MESSAGE_TYPE_JSON         0x02
#define MESSAGE_REPORT_START      0x50  /* Send Mediainfo */
#define MESSAGE_REPORT_STATUS     0x60  /* General status at the beginning at error and before end message */
#define MESSAGE_REPORT_STATS      0x20  /* Send speed each 10 seconds */
#define MESSAGE_REPORT_END        0x40  /* Out report */

#include <sys/types.h>

#include "pkst_defines.h"


typedef struct {
    uint8_t  protocol_id;   // Identificador del protocolo.
    uint8_t  msg_type;      // Tipo de mensaje y dirección.
    uint16_t msg_size;      // Tamaño del mensaje.
} PKSTMgsProto;


extern int pkst_send_data(int socket, const char *msg, uint8_t type);

#endif


