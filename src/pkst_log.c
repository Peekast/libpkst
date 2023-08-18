#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <libavutil/log.h>
#include <pthread.h>

#include "pkst_log.h"

void pkst_log_callback(void* ptr, int level, const char* fmt, va_list vl) {
    pthread_t id;
    if (level <= av_log_get_level()) {
        id = pthread_self();

        time_t rawtime;
        struct tm *timeinfo;
        char buffer[80];

        time(&rawtime);
        timeinfo = gmtime(&rawtime);  // Usar gmtime para obtener el tiempo en UTC

        strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S.000000 +0000 UTC]", timeinfo);
        fprintf(logFile, "%s Pthread: %lx - ", buffer, (unsigned long) id);
        vfprintf(logFile, fmt, vl);
    }
}

void pkst_log(void *ptr, int level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pkst_log_callback(NULL, level, fmt, args);
    va_end(args);
}