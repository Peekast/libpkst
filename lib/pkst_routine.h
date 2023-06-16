#ifndef _PKST_ROUTINE_H
#define _PKST_ROUTINE_H

#include <pthread.h>

typedef struct {
    pthread_t routine;
} PKSTRoutine;

#include "pkst_iocontext.h"


extern int pkst_start_encoder_routine(PKSTEncoderConfig  *config, PKSTRoutine **routine);
extern int pkst_wait_routine(PKSTRoutine *routine);
#endif