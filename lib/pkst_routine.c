#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <stdatomic.h>

#include "pkst_iocontext.h"
#include "pkst_mediainfo.h"
#include "pkst_audio.h"
#include "pkst_routine.h"
#include "pkst_msgproto.h"
#include "pkst_log.h"
#include <unistd.h>
#include <libavformat/avformat.h>

#include "netutils.h"


static void init_network_and_logging() {
    avformat_network_init();
    av_log_set_callback(pkst_log_callback);
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
    if (strcmp(mi->audio_codec,config->audio_config->codec)!= 0 || mi->sample_rate != config->audio_config->sample_rate) {
        pkst_log(NULL,0, "input require audio encoding");
        AVStream *audio = pkst_get_audio_stream(ctx);
        if (audio) {
            return pkst_open_audio_decoder_encoder(audio, config->audio_config, &(ctx->a_enc_ctx));
        }
    }
    return 0;
}



static void *pkst_encoder_routine(void *void_argument) {
    PKSTMultiOutCtx *out = NULL;
    PKSTRoutineArg *arg  = void_argument;
    PKSTMediaInfo *mi    = NULL;
    PKSTInputCtx *in     = NULL;
    AVPacket *pkt        = NULL;
    time_t last_dump_time;
    int socket = 0;
    int error  = 0;
    int *return_value = malloc(sizeof(int));
    int output_fail;

    pkt = av_packet_alloc();
    if (!pkt) {
        error = AVERROR(ENOMEM);
        goto exit;
    }

    init_network_and_logging();

    if ((socket = setup_tcp_connection(arg->config)) < 0) {
        error = socket;
        goto cleanup;
    }
   
    if ((error = pkst_open_input_context(arg->config, &in)) < 0)
        goto cleanup;
    
    if ((error = pkst_extract_mediainfo_from_AVFormatContext(in->in_ctx, &mi)) < 0)
        goto cleanup;

    handle_mediainfo_reporting(socket, mi);

    pkst_dump_mediainfo(mi);

    // There is a specific configuration
    error = handle_audio_encoding(arg->config, in, mi);
    pkst_free_mediainfo(&mi);
    if (error != 0)
        goto cleanup;

    if ((error = pkst_open_multiple_ouputs_context(arg->config, in, &out)) < 0)
        goto cleanup;

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
            free(msg);
            msg = NULL;
        }


    }   

    if (in->a_enc_ctx)
        error = pkst_flush_audio_encoder_queue(in, out);

    pkst_write_trailers_multiple_contexts(out->ctx, out->ctx_len);

cleanup:
    av_packet_free(&pkt);

    if (socket) {
        handle_end_reporting(socket, out, error);
        close(socket);
    }

    pkst_close_outputs_contexts(&out);

    pkst_close_input_context(&in);

    avformat_network_deinit();
    // Enviar tcp si corresponde
exit:
    free(void_argument);
    *return_value = error;
    return return_value; 
}


int pkst_start_encoder_routine(PKSTEncoderConfig  *config, PKSTRoutine **routine) {    
    int error;
    PKSTRoutineArg *arg = malloc(sizeof(PKSTRoutineArg));
    *routine = malloc(sizeof(PKSTRoutine));

    if (!*routine || !arg)
        return AVERROR(ENOMEM);

    arg->config = config;
    arg->should_exit = &((*routine)->should_exit);

    atomic_store(arg->should_exit, 0);

    if((error = pthread_create(&((*routine)->routine), NULL, pkst_encoder_routine, arg))) {
        free(*routine); // No olvides liberar la memoria en caso de error.
        free(arg);
        *routine = NULL;
        return error;
    }

    return 0;
}  

void pkst_cancel_routine(PKSTRoutine *routine) {
    atomic_store(&(routine->should_exit), 1);
}


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
