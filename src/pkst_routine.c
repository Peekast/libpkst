#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <stdatomic.h>

#include "pkst_iocontext.h"
#include "pkst_mediainfo.h"
#include "pkst_audio.h"
#include "pkst_routine.h"
#include "pkst_msgproto.h"
#include "pkst_strings.h"
#include "pkst_log.h"
#include <unistd.h>
#include <libavformat/avformat.h>

#include "netutils.h"


void pkst_init() {
    avformat_network_init();
    av_log_set_callback(pkst_log_callback);
}

void pkst_deinit() {
    avformat_network_deinit();
}

static int setup_tcp_connection(PKSTEncoderConfig *config) {
    int socket = 0;
    if (config->tcpstats) {
        socket = connect_tcp(config->tcpstats);
    }
    return socket;
}

static void handle_mediainfo_reporting(int socket, PKSTMediaInfo *mi) {
    if (socket) {
        char *msg = pkst_mediainfo_to_json(mi);
        if (msg) {
            int ret = pkst_send_data(socket, msg, MESSAGE_REPORT_START);
            free(msg);
            if (ret < 0 || ret == EAGAIN) {
                pkst_log(NULL,0, "[@MSGPROTO] drop start message\n");
            }
        } 
    }
}

static void handle_end_reporting(int socket, PKSTMultiOutCtx *out, int error) {
    char *msg = dump_multi_out_ctx_json(out, error);
    if (msg) {
        int ret = pkst_send_data(socket, msg, MESSAGE_REPORT_END);
        free(msg);
        if (ret < 0 || ret == EAGAIN)
            pkst_log(NULL,0, "[@MSGPROTO] drop end message\n");
    }
}

static int handle_audio_encoding(PKSTEncoderConfig *config, PKSTInputCtx *ctx, PKSTMediaInfo *mi) {
    if ((mi->audio_codec && strcmp(mi->audio_codec,config->audio_config->codec)!= 0) || mi->sample_rate != config->audio_config->sample_rate) {
        pkst_log(NULL,0, "input require audio encoding");
        AVStream *audio = pkst_get_audio_stream(ctx);
        if (audio) {
            return pkst_open_audio_decoder_encoder(audio, config->audio_config, &(ctx->a_enc_ctx));
        }
    }
    return 0;
}


/**
 * This function represents the main routine for the encoder thread. It takes care of setting up 
 * the input and output contexts, extracting media information, handling audio encoding, and 
 * processing the AV packets, among other tasks. It also handles potential errors and takes care 
 * of the cleanup process. 
 * 
 * @param void_argument A pointer to a PKSTRoutineArg structure containing the encoder configuration and a flag 
 *                      indicating whether to exit the routine.
 * 
 * @return A pointer to an integer that holds the result of the encoding process. This is dynamically allocated 
 *         and must be freed by the caller.
 */
static void *pkst_encoder_routine(void *void_argument) {
    PKSTMultiOutCtx *out = NULL;
    PKSTRoutineArg *arg  = void_argument;
    PKSTMediaInfo *mi    = NULL;
    PKSTInputCtx *in     = NULL;
    AVPacket *pkt        = NULL;
    time_t last_dump_time;
    int socket = 0;
    int error  = 0;
    int *return_value = pkst_alloc(sizeof(int));
    int output_fail;

    pkt = av_packet_alloc();
    if (!pkt) {
        error = AVERROR(ENOMEM);
        goto exit;
    }

    if ((socket = setup_tcp_connection(arg->config)) < 0) {
        error = socket;
        goto cleanup;
    }
   
    if ((error = pkst_open_input_context(arg->config, &in)) < 0)
        goto cleanup;
    
    if ((error = pkst_extract_mediainfo_from_AVFormatContext(in->in_ctx, &mi)) < 0)
        goto cleanup;

    if ((error = handle_audio_encoding(arg->config, in, mi)) != 0)
        goto cleanup;

    if ((error = pkst_open_multiple_ouputs_context(arg->config, in, &out)) < 0)
        goto cleanup;

    handle_mediainfo_reporting(socket, mi);

    pkst_dump_mediainfo(mi);
    // 
    last_dump_time = out->stats->start_time;

    while(!atomic_load(arg->should_exit)) {

        error = pkst_process_av_packet(pkt, in, out, &output_fail);

        if (error < 0) {
            if (error == AVERROR_EOF)
                break;
            else 
                goto cleanup;
        }

        out->stats->current_time = time(NULL);

        if (difftime(out->stats->current_time, last_dump_time) >= 1.0 || output_fail == BOOL_TRUE) {
            dump_multi_out_ctx(out);          
            last_dump_time = out->stats->current_time;
        }

        if (output_fail && socket) {
            output_fail = BOOL_FALSE;
            char *msg = dump_multi_out_ctx_json(out, 0);
            if (pkst_send_data(socket, msg, MESSAGE_REPORT_STATS) < 0) 
                socket = 0;
            if (msg) 
                free(msg);
            msg = NULL;
        }
    }   

    if (in->a_enc_ctx)
        error = pkst_flush_audio_encoder_queue(in, out);

    pkst_write_trailers_multiple_contexts(out->ctx, out->ctx_len);

cleanup:
    if (mi)
        pkst_free_mediainfo(&mi);

    av_packet_free(&pkt);

    if (socket) {
        handle_end_reporting(socket, out, error);
        close(socket);
    }

    pkst_close_outputs_contexts(&out);

    pkst_close_input_context(&in);

//    avformat_network_deinit();
    // Enviar tcp si corresponde
exit:
    if (arg->callback && arg->opaque) {
        arg->callback(arg->opaque);
    }
    *return_value = error;
    free(void_argument);
    return return_value; 
}

/**
 * This function starts the encoder routine in a new thread. It prepares the arguments, 
 * initializes the necessary structures and starts the thread with the encoding routine.
 *
 * @param config Pointer to the PKSTEncoderConfig structure containing the encoder configuration.
 * @param routine Address of a pointer to a PKSTRoutine structure. The function will allocate memory for this structure
 *                and store the address of the new memory in the provided pointer. The caller is responsible for freeing this memory.
 *
 * @return Zero on successful thread creation, or a non-zero error code on failure. In the case of a memory allocation error, 
 *         it returns the AVERROR(ENOMEM) error code. In the case of a pthread creation error, it returns the error code returned by pthread_create.
 */
int pkst_start_encoder_routine(PKSTEncoderConfig  *config, PKSTRoutine **routine, callback_routine_t callback, void *opaque) {    
    int error;
    PKSTRoutineArg *arg = pkst_alloc(sizeof(PKSTRoutineArg));
    *routine = pkst_alloc(sizeof(PKSTRoutine));

    if (!*routine || !arg)
        return AVERROR(ENOMEM);

    arg->config = config;
    arg->should_exit = &((*routine)->should_exit);
    arg->callback = callback;
    arg->opaque = opaque;

    atomic_store(arg->should_exit, 0);

    if((error = pthread_create(&((*routine)->routine), NULL, pkst_encoder_routine, arg))) {
        free(*routine); // No olvides liberar la memoria en caso de error.
        free(arg);
        *routine = NULL;
        return error;
    }

    return 0;
}  

/**
 * This function is used to cancel an ongoing PKSTRoutine.
 * It uses atomic operations to ensure thread safety.
 *
 * @param routine A pointer to the PKSTRoutine to be canceled.
 * This should have been previously initialized and started.
 *
 * When this function is called, it sets the 'should_exit' flag
 * of the routine to 1, signaling to the routine that it should
 * terminate as soon as possible.
 */
void pkst_cancel_routine(PKSTRoutine *routine) {
    atomic_store(&(routine->should_exit), 1);
}

/**
 * This function is used to wait for a PKSTRoutine to finish executing.
 * It is a blocking function, meaning it will not return until the PKSTRoutine 
 * has finished execution or an error occurs.
 *
 * @param routine A double pointer to the PKSTRoutine to wait on.
 * This should have been previously initialized and started.
 *
 * @param ret A pointer to a location where the exit status of the routine 
 * will be stored. If the routine has been canceled, the exit status will 
 * be PTHREAD_CANCELED.
 *
 * @return 0 if the routine successfully joined; otherwise, it will return an error code.
 *
 * After the routine has finished and has been joined, this function 
 * will also free the memory associated with the PKSTRoutine and set the pointer 
 * to NULL to prevent dangling pointers.
 */
int pkst_wait_routine(PKSTRoutine **routine, void *ret) {
    int error;
    if (routine && *routine) {
        if ((error = pthread_join((*routine)->routine, ret)) != 0)
            return error;

        free(*routine);
        *routine = NULL;
    }
    return 0;
}
