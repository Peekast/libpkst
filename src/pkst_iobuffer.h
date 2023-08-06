#ifndef _PKST_IOBUFFER_H
#define _PKST_IOBUFFER_H

#include <sys/types.h>

typedef struct {
    const uint8_t *buffer;
    size_t buf_len;
    size_t pos;
} PKSTIOBuffer;
//PKstIOBuffer

extern int ioread_buffer(void *opaque, uint8_t *buf, int buf_size);
#endif