#include <sys/socket.h>
#include <sys/types.h>
#include <libavutil/error.h>
#include <libavutil/common.h>

int ioread_socket(void *opaque, uint8_t *buf, int buf_size) {
    int socketfd = *((int*)opaque); // Asegúrate de que opaque es un puntero al descriptor de archivo del socket

    // Intentar leer buf_size bytes del socket
    ssize_t bytes_read = recv(socketfd, buf, buf_size, 0);

    // Si recv devuelve -1, ocurrió un error
    if (bytes_read == -1) {
        return AVERROR(EIO);
    }

    // Si recv devuelve 0, significa que el otro extremo cerró la conexión
    if (bytes_read == 0) {
        return AVERROR_EOF;
    }

    // Si todo fue bien, devolver el número de bytes que leímos
    return bytes_read;
}


int iowrite_socket(void *opaque, uint8_t *buf, int buf_size) {
    int socketfd = *((int*)opaque); // Asegúrate de que opaque es un puntero al descriptor de archivo del socket

    // Intentar escribir buf_size bytes al socket
    ssize_t bytes_written = send(socketfd, buf, buf_size, 0);

    // Si send devuelve -1, ocurrió un error
    if (bytes_written == -1) {
        return AVERROR(EIO);
    }

    // Si todo fue bien, devolver el número de bytes que escribimos
    return bytes_written;
}
