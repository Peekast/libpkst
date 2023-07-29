#ifndef _PKST_IOSOCKET_H
#define _PKST_IOSOCKET_H 1

#include <sys/types.h>

extern int ioread_socket(void *opaque, uint8_t *buf, int buf_size);
extern int iowrite_socket(void *opaque, uint8_t *buf, int buf_size);
#endif