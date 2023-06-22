#ifndef _PKST_MSG_PROTO_H
#define _PKST_MSG_PROTO_H 1

#define PROTOCOL_IDENTIFIER       0xAA
#define MESSAGE_REPORT_START      0x50  /* Send Mediainfo */
#define MESSAGE_REPORT_STATS      0x20  /* Send output status */
#define MESSAGE_REPORT_END        0x40  /* Out report */

#include <sys/types.h>
#include <stdint.h>
#include "pkst_defines.h"

#pragma pack(push, 1)
typedef struct {
    uint8_t  protocol_id;   // Identificador del protocolo.
    uint8_t  msg_type;      // Tipo de mensaje y dirección.
    uint16_t msg_size;      // Tamaño del mensaje.
} PKSTMgsProto;
#pragma pack(pop)

extern int pkst_send_data(int socket, const char *msg, uint8_t type);

#endif


