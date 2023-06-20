#ifndef _PKST_LOG_H
#define _PKST_LOG_H 1

#include <stdio.h>
#include <stdarg.h>


#define logFile stderr

extern void pkst_log_callback(void* ptr, int level, const char* fmt, va_list vl);

extern void pkst_log(void *ptr, int level, const char* fmt, ...);
#endif