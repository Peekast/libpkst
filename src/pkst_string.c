
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *pkst_strdup(const char *s) {
    char *d = calloc(1,strlen(s) + 1);   // Asigna memoria para la cadena
    if (d == NULL) return NULL;        // No se pudo asignar memoria
    strcpy(d, s);                      // Copia la cadena
    return d;                          // Devuelve la nueva cadena
}

void *pkst_alloc(size_t len) {
    return calloc(1, len);
}