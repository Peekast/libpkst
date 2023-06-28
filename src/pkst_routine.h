#ifndef _PKST_ROUTINE_H
#define _PKST_ROUTINE_H


// pthread.h is a header file for multithreading with POSIX threads (pthreads)
#include <pthread.h>

// stdatomic.h is a header file for atomic operations in C
#include <stdatomic.h>

// PKSTRoutine struct contains pthread_t routine, which is a POSIX thread identifier, 
// and atomic_int should_exit, which is an atomic integer flag used to indicate whether the routine should exit
typedef struct {
    pthread_t  routine;
    atomic_int should_exit;
} PKSTRoutine;

// PKSTRoutineArg struct contains a pointer to the PKSTEncoderConfig struct and an atomic_int should_exit flag. 
// This struct is used as an argument when starting the encoder routine.
typedef struct {
    PKSTEncoderConfig *config;
    atomic_int   *should_exit;
} PKSTRoutineArg;

// pkst_iocontext.h is a header file containing declarations related to the I/O context in PKST
#include "pkst_iocontext.h"

// Declaration of function to start the encoder routine. It takes a pointer to PKSTEncoderConfig and a pointer to a PKSTRoutine struct.
// Returns an int indicating the status of the operation.
extern int pkst_start_encoder_routine(PKSTEncoderConfig  *config, PKSTRoutine **routine);

// Declaration of function to wait for a routine to finish. It takes a pointer to a PKSTRoutine struct and a pointer to the return value.
// Returns an int indicating the status of the operation.
extern int pkst_wait_routine(PKSTRoutine **routine, void *ret);

// Declaration of function to cancel a routine. It takes a PKSTRoutine struct.
extern void pkst_cancel_routine(PKSTRoutine *routine);

// Init loggin and Network
extern void pkst_init();

// De init network
extern void pkst_deinit();
#endif