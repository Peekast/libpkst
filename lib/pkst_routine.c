#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "pkst_iocontext.h"
#include "pkst_mediainfo.h"
#include "pkst_audio.h"
#include "pkst_routine.h"
#include "pkst_msgproto.h"

#include <unistd.h>
#include <libavformat/avformat.h>

#include "netutils.h"


void *pkst_encoder_routine(void *arg) {
    AVStream          *audio = NULL;
    PKSTMediaInfo     mi;
    PKSTEncoderConfig *config = arg;
    PKSTMultiOutCtx   *out_ctx = NULL;
    char *msg = NULL;
    int socket = 0;
    int *ret = malloc(sizeof(int));


    PKSTInputCtx *ctx = NULL;

    avformat_network_init();

    if (config->tcpstats) {
        socket = connect_tcp(config->tcpstats);
        if (socket < 0) {
            *ret = socket;
            goto cleanup;
        }
    }

    *ret = pkst_open_input_context(config, &ctx);
    if (*ret < 0) goto cleanup;


    *ret = pkst_extract_mediainfo_from_AVFormatContext(ctx->in_ctx, &mi);
    if (*ret < 0) goto cleanup;

    if (socket) {
        msg = pkst_mediainfo_to_json(&mi);
        if (!msg) {
            *ret = pkst_send_data(socket, msg, MESSAGE_TYPE_JSON | MESSAGE_REPORT_START);
            free(msg);
            msg = NULL;
            if (*ret < 0) goto cleanup;
            if (*ret == EAGAIN)
                fprintf(stdout, "[@MSGPROTO] drop start message\n");
        }
    } else {
        // Dump mediainfo
        pkst_dump_mediainfo(&mi);
    }

    // There is a specific configuration
    if (config->audio_config) {
        if (strcmp(mi.audio_codec,config->audio_config->codec)!= 0 || mi.sample_rate != config->audio_config->sample_rate) {
            audio = pkst_get_audio_stream(ctx);
            if (audio) {
                *ret = pkst_open_audio_decoder_encoder(audio, config->audio_config, &(ctx->a_enc_ctx));
                if (*ret < 0) goto cleanup;
            }
        }
    }

    *ret = pkst_open_multiple_ouputs_context(config, ctx, &out_ctx);
    if (*ret < 0) goto cleanup;

    *ret = pkst_audiovideo_process(ctx, out_ctx, socket);

    if (socket) {
        msg = pkst_out_report_to_json(out_ctx, *ret);
        if (!msg) {
            *ret = pkst_send_data(socket, msg, MESSAGE_TYPE_JSON | MESSAGE_REPORT_END);
            free(msg);
            if (*ret == EAGAIN)
                fprintf(stdout, "[@MSGPROTO] drop end message\n");
        }
    }

    pkst_write_trailers_multiple_contexts(out_ctx->ctx, out_ctx->ctx_len);
    pkst_close_outputs_contexts(&out_ctx);



cleanup:
    if (socket) 
        close(socket);
    if (ctx) {
        pkst_cleanup_decoder_encoder(ctx->a_enc_ctx);
        ctx->a_enc_ctx = NULL;
        pkst_close_input_context(&ctx);
    }
    // Enviar tcp si corresponde
    return ret;
}


int pkst_start_encoder_routine(PKSTEncoderConfig  *config, PKSTRoutine **routine) {    
    *routine = malloc(sizeof(PKSTRoutine));
    if (!*routine)
        return -1;

    if(pthread_create(&((*routine)->routine), NULL, pkst_encoder_routine, config)) {
        free(*routine); // No olvides liberar la memoria en caso de error.
        return -1;
    }

    return 0;
}  

int pkst_wait_routine(PKSTRoutine *routine) {
    void *ret;
    int i;
    if (pthread_join(routine->routine, &ret) != 0) {
        return 1;
    }
    free(routine);
    i = *(int *)ret;
    free(ret);
    return i;
}