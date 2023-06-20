#include <string.h>

#include <libavutil/error.h>
#include <libavutil/common.h>


#include "pkst_iobuffer.h"


int ioread_buffer(void *opaque, uint8_t *buf, int buf_size) {
    PKSTIOBuffer *p = opaque;

    // Verificar si hemos alcanzado el final del búfer de entrada
    // Tendrás que reemplazar INPUT_BUFFER_SIZE con el tamaño real de tu búfer
    if (p->pos >= p->buf_len) {
        // Hemos alcanzado el final del búfer, no hay más datos para leer
        return AVERROR_EOF;
    }

    // Calcular cuántos datos quedan por leer en el búfer
    size_t bytes_remaining = p->buf_len - p->pos;

    // Calcular cuántos bytes vamos a leer en esta llamada
    // No queremos leer más allá del final del búfer, así que leemos solo
    // la cantidad más pequeña entre buf_size y bytes_remaining
    size_t bytes_to_read = FFMIN(buf_size, bytes_remaining);

    // Copiar los datos del búfer de entrada al búfer de FFmpeg
    memcpy(buf, p->buffer + p->pos, bytes_to_read);

    // Actualizar la posición actual en el búfer
    p->pos += bytes_to_read;

    // Devolver el número de bytes que leímos
    return bytes_to_read;
}
