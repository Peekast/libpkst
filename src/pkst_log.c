#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <libavutil/log.h>

#include "pkst_log.h"

void pkst_log_callback(void* ptr, int level, const char* fmt, va_list vl) {
    if (level <= av_log_get_level()) {
        time_t current_time;
        char* c_time_string;
        
        current_time = time(NULL);
        c_time_string = ctime(&current_time);
        char* pos = strchr(c_time_string, '\n');
        if (pos) {
            *pos = '\0';
        }
        /* Note that ctime() has already added a terminating newline character */
        fprintf(logFile, "[%s] ", c_time_string);
        vfprintf(logFile, fmt, vl);
    }
}

void pkst_log(void *ptr, int level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pkst_log_callback(NULL, level, fmt, args);
    va_end(args);
}