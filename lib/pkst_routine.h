#ifndef _PKST_ROUTINE_H
#define _PKST_ROUTINE_H

#include <pthread.h>
#include <stdatomic.h>

typedef struct {
    pthread_t  routine;
    atomic_int should_exit;
} PKSTRoutine;

typedef struct {
    PKSTEncoderConfig *config;
    atomic_int   *should_exit;
} PKSTRoutineArg;

#include "pkst_iocontext.h"


extern int pkst_start_encoder_routine(PKSTEncoderConfig  *config, PKSTRoutine **routine);
extern int pkst_wait_routine(PKSTRoutine **routine, void *ret);
extern void pkst_cancel_routine(PKSTRoutine *routine);
#endif