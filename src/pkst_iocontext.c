#include <stdio.h>
#include <stdlib.h>


#include "pkst_iocontext.h"
#include "pkst_strings.h"
#include "pkst_msgproto.h"
#include "pkst_log.h"
#include <jansson.h>
#include <stdarg.h>
#include <time.h>
#include "keyvalue.h"

// Includes the header file for the libavformat library.
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>


/* 
 * Function to allocate and initialize a new PKSTEncoderConfig structure. 
 * The function takes as input the address of a pointer to a PKSTEncoderConfig 
 * structure, along with a set of character strings representing the input,
 * the input type, and tcp stats. 
 * 
 * The function returns 0 if it succeeds and -1 if an error occurs.
 */
int pkst_alloc_encoder_config(PKSTEncoderConfig **enc, char *in, int listen, int timeout, char *in_type, char *tcpstats) {
    // Allocate memory for the new PKSTEncoderConfig structure
    PKSTEncoderConfig *new_enc = pkst_alloc(sizeof(PKSTEncoderConfig));
    // If allocation fails, return -1
    if (new_enc == NULL) {
        return -1; 
    }

    // Initialize the number of outputs to 0
    new_enc->outs_len = 0;
    // Initialize all output configurations to NULL
    for (int i = 0; i < MAX_OUTPUTS; i++) {
        new_enc->outs[i] = NULL;
    }

    // If input string is not NULL, duplicate the string, else set to NULL
    new_enc->in = in && strlen(in) != 0 ? pkst_strdup(in) : NULL;
    // If input type string is not NULL, duplicate the string, else set to NULL
    new_enc->in_type = in_type && strlen(in_type) != 0 ? pkst_strdup(in_type) : NULL;
    // If tcpstats string is not NULL, duplicate the string, else set to NULL
    new_enc->tcpstats = tcpstats && strlen(tcpstats) != 0 ? pkst_strdup(tcpstats) : NULL;

    // If any of the strdup operations failed, free all allocated memory and return -1
    if ((in && strlen(in) != 0 && new_enc->in == NULL) || 
        (in_type && strlen(in_type) != 0 && new_enc->in_type == NULL) || 
        (tcpstats && strlen(tcpstats) != 0 && new_enc->tcpstats == NULL)) {
        if (new_enc->in) free(new_enc->in);
        if (new_enc->in_type) free(new_enc->in_type);
        if (new_enc->tcpstats) free(new_enc->tcpstats);
        free(new_enc);
        return -1;
    }
    new_enc->timeout = timeout;
    new_enc->listen  = listen;
    new_enc->audio_config = NULL;
    new_enc->video_config = NULL;

    // Point the input encoder config pointer to the newly allocated structure
    *enc = new_enc;

    // Return 0 to indicate successful execution
    return 0; 
}

/* 
 * This function adds a new output configuration to an existing PKSTEncoderConfig. 
 * It takes as arguments a pointer to a PKSTEncoderConfig and a pointer to a PKSTOutputConfig.
 *
 * The function returns 0 if it succeeds and -1 if an error occurs.
 */
int pkst_add_output_encoder_config(PKSTEncoderConfig *enc, PKSTOutputConfig *out) {
    // Declare a pointer for the new output context
    PKSTOutputConfig *ctx;
    
    // Check if the number of outputs has reached the maximum. If so, return error.
    if (enc->outs_len == MAX_OUTPUTS)
        return -1;

    // Allocate memory for the new output context. If allocation fails, return error.
    ctx = pkst_alloc(sizeof(PKSTOutputConfig));
    if (!ctx)
        return -1;

    // Copy the onfail_ignore flag from the input to the new output context
    ctx->onfail_ignore = out->onfail_ignore;
    
    // If dst string is not NULL, duplicate the string, else set to NULL
    ctx->dst = out->dst ? pkst_strdup(out->dst) : NULL;
    // If dst_type string is not NULL, duplicate the string, else set to NULL
    ctx->dst_type = out->dst_type ? pkst_strdup(out->dst_type) : NULL;
    // If kv_opts string is not NULL, duplicate the string, else set to NULL
    ctx->kv_opts = out->kv_opts ? pkst_strdup(out->kv_opts) : NULL;

    // If any of the strdup operations failed, free all allocated memory and return -1
    if (ctx->dst == NULL) {
        if (ctx->dst) free(ctx->dst);
        if (ctx->dst_type) free(ctx->dst_type);
        if (ctx->kv_opts) free(ctx->kv_opts);
        free(ctx);
        return -1;
    }

    // Add the new output context to the encoder config's outputs array and increment the outputs count
    enc->outs[enc->outs_len++] = ctx;
    
    // Return 0 to indicate successful execution
    return 0;
}

/**
 * @brief Add an audio encoder configuration to an encoder configuration structure.
 *
 * This function duplicates the provided audio configuration and appends it to the specified 
 * encoder configuration structure. The audio configuration encompasses attributes such as 
 * the audio codec, bitrate, channel count, and sample rate. Additionally, the function allows
 * the user to specify if audio encoding should be forcibly applied regardless of the current 
 * configuration.
 *
 * @param enc The encoder configuration structure where the duplicated audio configuration will be stored.
 * @param config The audio configuration that will be cloned and attached to the encoder configuration.
 * @param force_encoding A flag indicating whether to enforce audio encoding irrespective of the existing setup.
 *
 * @return Returns 0 if the addition is successful, -1 if an error occurs (e.g., memory allocation issues or NULL inputs).
 */
int pkst_add_audio_encoder_config(PKSTEncoderConfig *enc, PKSTAudioConfig *config, int force_encoding) {
    if (!enc || !config)
        return -1; // Retornar algún código de error o hacer un manejo de errores adecuado.

    enc->audio_config = pkst_alloc(sizeof(PKSTAudioConfig));
    if (!enc->audio_config)
        return -1; // Error en la asignación de memoria.

    enc->audio_config->codec = pkst_strdup(config->codec);
    if (!enc->audio_config->codec) {
        free(enc->audio_config);
        enc->audio_config = NULL;
        return -1; // Error al duplicar el string.
    }
    enc->force_audio_encoding = force_encoding;
    enc->audio_config->bitrate_bps = config->bitrate_bps;
    enc->audio_config->channels = config->channels;
    enc->audio_config->sample_rate = config->sample_rate;

    return 0; // Retornar 0 si todo salió bien.
}

/* 
 * This function prints the details of a PKSTEncoderConfig structure. 
 * It receives a pointer to a PKSTEncoderConfig and prints each of its elements to stdout.
 */
void pkst_dump_encoder_config(PKSTEncoderConfig *enc) {
    int i;

    if (enc == NULL) {
        pkst_log(NULL,0,"Encoder Context: NULL\n");
        return;
    }

    pkst_log(NULL,0,"Encoder Context:\n");
    pkst_log(NULL,0,"    Input: %s\n", enc->in ? enc->in : "NULL");
    pkst_log(NULL,0,"    Input Type: %s\n", enc->in_type ? enc->in_type : "NULL");
    pkst_log(NULL,0,"    Listen: %s\n", enc->listen ? "true" : "false");
    pkst_log(NULL,0,"    Network timeout: %d (seconds)\n", enc->timeout);
    pkst_log(NULL,0,"    TCP Stats: %s\n", enc->tcpstats ? enc->tcpstats : "NULL");
    pkst_log(NULL,0,"    Output Length: %d\n", enc->outs_len);

    if (enc->audio_config) {
        pkst_log(NULL,0,"    Audio Config:\n");
        pkst_log(NULL,0,"        Audio Codec: %s\n", enc->audio_config->codec ? enc->audio_config->codec : "NULL");
        pkst_log(NULL,0,"        Audio Bitrate (bps): %d\n", enc->audio_config->bitrate_bps);
        pkst_log(NULL,0,"        Audio Channels: %d\n", enc->audio_config->channels);
        pkst_log(NULL,0,"        Sample Rate: %d\n", enc->audio_config->sample_rate);
    }

    if (enc->video_config) {
        // Here you can add the printing of video_config fields, similar to audio_config.
    }

    for (i = 0; i < enc->outs_len; i++) {
        pkst_log(NULL,0,"    Output[%d]:\n", i);
        pkst_log(NULL,0,"        Destination: %s\n", enc->outs[i]->dst ? enc->outs[i]->dst : "NULL");
        pkst_log(NULL,0,"        Destination Type: %s\n", enc->outs[i]->dst_type ? enc->outs[i]->dst_type : "NULL");
        pkst_log(NULL,0,"        KV Options: %s\n", enc->outs[i]->kv_opts ? enc->outs[i]->kv_opts : "NULL");
        pkst_log(NULL,0,"        On Fail Ignore: %d\n", enc->outs[i]->onfail_ignore);
    }
}

/* 
 * This function deallocates a PKSTOutputConfig structure. 
 * It receives a pointer to a PKSTOutputConfig and frees all its members and itself.
 * Note that after calling this function, the pointer is no longer valid and should not be used.
 */
static void pkst_free_output_config(PKSTOutputConfig *out) {
    if (out) {
        if (out->dst) {
            free(out->dst);
            out->dst = NULL;
        }
        if (out->dst_type) {
            free(out->dst_type);
            out->dst_type = NULL;
        }
        if (out->kv_opts) {
            free(out->kv_opts);
            out->kv_opts = NULL;
        }
        free(out);
    }
}

/* 
 * This function deallocates a PKSTEncoderConfig structure. 
 * It receives a pointer to a PKSTEncoderConfig and frees all its members and itself.
 * This includes freeing each output configuration within the encoder config.
 * Note that after calling this function, the pointer is no longer valid and should not be used.
 */
void pkst_free_encoder_config(PKSTEncoderConfig **enc) {
    int i;
    if (enc && *enc) {
        if ((*enc)->in) {
            free((*enc)->in);
        }
        if ((*enc)->in_type) {
            free((*enc)->in_type);
        }
        if ((*enc)->tcpstats) {
            free((*enc)->tcpstats);
        }
        if ((*enc)->audio_config) {
            if ((*enc)->audio_config->codec) free((*enc)->audio_config->codec);
            free((*enc)->audio_config);
        }
        if ((*enc)->video_config) {
            free((*enc)->video_config);
        }
        for (i = 0; i < (*enc)->outs_len; i++) {
            pkst_free_output_config((*enc)->outs[i]);
        }
        free(*enc);
        *enc = NULL;
    }
}


/* 
 * This function closes an input context and deallocates its memory. 
 * It takes a double pointer to a PKSTInputCtx. 
 * If the pointer or the context itself is NULL, the function returns without doing anything. 
 * Otherwise, it closes the AVFormatContext contained in the PKSTInputCtx (if it's not NULL) using the avformat_close_input function. 
 * It then frees the PKSTInputCtx and sets the pointer to NULL. 
 * After this function is called, the pointer that was passed to it should not be used.
 */
void pkst_close_input_context(PKSTInputCtx **ctx) {
    if (!ctx || !*ctx) return;

    if ((*ctx)->in_ctx)
        avformat_close_input(&((*ctx)->in_ctx));

    if ((*ctx)->a_enc_ctx) 
        pkst_cleanup_decoder_encoder(&((*ctx)->a_enc_ctx));

    free(*ctx);
    *ctx = NULL;
}

/* 
 * This function opens an input context for media data based on the configuration specified in the PKSTEncoderConfig structure.
 * It takes a pointer to a PKSTEncoderConfig and a double pointer to a PKSTInputCtx.
 *
 * The function first checks if there is a specific input format requested in the encoder config.
 * If so, it uses av_find_input_format to get the appropriate input format.
 * If this fails, the function returns -1.
 *
 * Then it allocates memory for a new PKSTInputCtx and initializes its fields.
 * If any of these steps fail (e.g., if malloc or avformat_alloc_context return NULL), 
 * the function cleans up any resources already allocated, sets the PKSTInputCtx pointer to NULL, and returns -1.
 *
 * Next, it opens the input using avformat_open_input. If this fails, it cleans up and returns -1.
 * It also calls avformat_find_stream_info to fill in important information about the input streams.
 *
 * Then the function iterates over all streams in the input context, searching for audio and video streams.
 * It sets the respective indices in the PKSTInputCtx->streams array to the indices of the first audio and video streams it finds.
 *
 * If the function is successful, it returns 0.
 * If any errors occur during the process, the function performs cleanup: it closes the AVFormatContext, frees the PKSTInputCtx,
 * sets the PKSTInputCtx pointer to NULL, and returns -1.
 */
int pkst_open_input_context(PKSTEncoderConfig *config, PKSTInputCtx **ctx) {
    char timeout[32];
    AVDictionary *options = NULL;
    AVFormatContext *in_ctx;
    AVCodecParameters *in_codecpar;
    const AVInputFormat *infmt;
    AVStream *in_stream;
    int ret, i;



    if (config->listen) {
        av_dict_set(&options, "listen", "1", 0);
    }

    if (config->timeout) {
        if (config->in_type) {
            if (!strcmp(config->in_type, "flv")) {
                sprintf(timeout, "%d", config->timeout);
                av_dict_set(&options, "timeout", timeout, 0);
            } else if (!strcmp(config->in_type, "sdp")) {
                sprintf(timeout, "%d", config->timeout * 1000000);
                av_dict_set(&options, "rw_timeout", timeout, 0);
            }
        } else {
            sprintf(timeout, "%d", config->timeout * 1000);
            av_dict_set(&options, "listen_timeout", timeout, 0);
        }
    }

    if (config->in_type && !strcmp(config->in_type, "flv")) {
        infmt = av_find_input_format(config->in_type);
        if (!infmt) return -1;
    }

    if (config->in_type && !strcmp(config->in_type, "sdp")) {
        av_dict_set(&options, "protocol_whitelist", "file,udp,rtp", 0);
    }

    *ctx = pkst_alloc(sizeof(PKSTInputCtx));
    if (*ctx == NULL) return -1;

    (*ctx)->streams[PKST_AUDIO_STREAM_OUTPUT] = -1;
    (*ctx)->streams[PKST_VIDEO_STREAM_OUTPUT] = -1;

    in_ctx = NULL; 

    ret = avformat_open_input(&in_ctx, config->in, NULL, options ? &options : NULL);
    if (ret) goto cleanup;


    ret = avformat_find_stream_info(in_ctx, NULL);
    if (ret) goto close;

    for (i = 0; i < in_ctx->nb_streams; i++) {
        in_stream = in_ctx->streams[i];
        in_codecpar = in_stream->codecpar;
        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }
        if (in_codecpar->codec_type == AVMEDIA_TYPE_AUDIO && (*ctx)->streams[PKST_AUDIO_STREAM_OUTPUT] == -1)
            (*ctx)->streams[PKST_AUDIO_STREAM_OUTPUT] = i;
        if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO && (*ctx)->streams[PKST_VIDEO_STREAM_OUTPUT] == -1)
            (*ctx)->streams[PKST_VIDEO_STREAM_OUTPUT] = i;
    }
    (*ctx)->in_ctx = in_ctx;

    return 0;

close:
    avformat_close_input(&in_ctx);
    in_ctx = NULL;
cleanup:
    if (!in_ctx)
        avformat_free_context(in_ctx);
    free(*ctx);
    *ctx = NULL;
    return ret;
}

AVStream *pkst_get_audio_stream(PKSTInputCtx *ctx) {
    AVStream *audio = NULL;
    if (ctx) {
        if (ctx->streams[PKST_AUDIO_STREAM_OUTPUT] != -1) {
            audio = ctx->in_ctx->streams[ctx->streams[PKST_AUDIO_STREAM_OUTPUT]];
        }
    }
    return audio;
}


/**
 * @brief Initialize an output context for remuxing or encoding AV streams.
 * 
 * This function creates an AVFormatContext for output, based on the provided configuration 
 * and input AV streams. It handles both remuxing and encoding of video and audio streams.
 *
 * @param config Pointer to a PKSTOutputConfig structure containing the configuration for the output context.
 * @param remux Array of AVStream pointers for streams to be remuxed. Should contain 2 elements: 
 *        the video stream at index PKST_VIDEO_STREAM_OUTPUT and the audio stream at index PKST_AUDIO_STREAM_OUTPUT.
 * @param enc Array of AVCodecContext pointers for streams to be encoded. Should contain 2 elements:
 *        the video context at index PKST_VIDEO_STREAM_OUTPUT and the audio context at index PKST_AUDIO_STREAM_OUTPUT.
 *
 * @note At each index, either the remux or enc pointer should be non-null, but not both. If both are non-null, 
 *       the function returns NULL.
 * @note If the function fails at any point, it will free all resources it has allocated up to that point and return NULL.
 * @note If the function is successful, it returns a pointer to the created AVFormatContext. The caller is 
 *       responsible for freeing this with avformat_free_context().
 *
 * @return On success, a pointer to the created AVFormatContext. On failure, returns NULL.
 */
static AVFormatContext *pkst_open_output_context(PKSTOutputConfig *config, AVStream *remux[2], AVCodecContext *encoder[2]) {
    const AVOutputFormat *ofmt;
    AVFormatContext *ctx = NULL;
    AVDictionary *opts = NULL;
    AVStream *out_stream = NULL;
    KeyValueList *kv_opts;
    int i, ret = 0;


    if ((remux[PKST_VIDEO_STREAM_OUTPUT] != NULL && encoder[PKST_VIDEO_STREAM_OUTPUT] != NULL) || 
        (remux[PKST_AUDIO_STREAM_OUTPUT] != NULL && encoder[PKST_AUDIO_STREAM_OUTPUT] != NULL)) {
        return NULL;
    }

    avformat_alloc_output_context2(&ctx, NULL, config->dst_type, config->dst);
    if (!ctx) return NULL;

    ofmt = ctx->oformat;

    for (i = 0; i < 2; i++) {
        if (remux[i]) {
            out_stream = avformat_new_stream(ctx, NULL);
            if (!out_stream) goto cleanup;

            ret = avcodec_parameters_copy(out_stream->codecpar, remux[i]->codecpar);
            if (ret < 0) goto cleanup;

            out_stream->codecpar->codec_tag = 0;            
            av_dict_set(&(out_stream->metadata), "handler_name",HANDLER_NAME, 0);
        }
    }

    for (i = 0; i < 2; i++) {
        if (encoder[i]) {
            out_stream = avformat_new_stream(ctx, NULL);
            if (!out_stream) goto cleanup;

            ret = avcodec_parameters_from_context(out_stream->codecpar, encoder[i]);
            if (ret < 0) goto cleanup;

            if (i == PKST_AUDIO_STREAM_OUTPUT) {
                out_stream->time_base.den = encoder[i]->sample_rate;//44100; //input_codec_context->sample_rate;
                out_stream->time_base.num = 1;
            }
            if (ofmt->flags & AVFMT_GLOBALHEADER)
                encoder[i]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            av_dict_set(&(out_stream->metadata), "handler_name",HANDLER_NAME, 0);
        }
    }


    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ctx->pb, config->dst, AVIO_FLAG_WRITE);
        if (ret < 0) goto cleanup;
    }

    if (config->kv_opts) {
        kv_opts = parse_kv_list(config->kv_opts, PKST_PAIR_DELIM, PKST_KV_DELIM);
        if (kv_opts) {
            for (i=0; i < kv_opts->count; i++) 
                av_dict_set(&opts, kv_opts->items[i].key, kv_opts->items[i].value, 0);
            free_kv_list(kv_opts);
            kv_opts = NULL;
        }
    }

    if (opts) {
        ret = avformat_write_header(ctx, &opts);
        av_dict_free(&opts);
    } else {
        ret = avformat_write_header(ctx, NULL);
    }
    if (ret < 0) goto close;

    return ctx;

close:
    if (ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ctx->pb);
cleanup:
    if (ctx)
        avformat_free_context(ctx);
    return NULL;
} 

/**
 * @brief Open multiple output contexts for remuxing based on the provided configuration and input context.
 * @param config The encoder configuration, which contains information about the outputs.
 * @param in_ctx The input context, which contains information about the input streams.
 * @param out_ctx The pointer to the output context array to be allocated and initialized.
 * @return int Returns 0 if successful, -1 if an error occurs.
 * 
 * This function will allocate an array of PKSTOutProcCtx and initialize each of them by calling 
 * pkst_open_output_context_for_remux() for each output specified in 'config'. The function also sets the 
 * 'status' and 'onfail_ignore' fields of each PKSTOutProcCtx based on the corresponding fields in the 'config'.
 * In case of error during initialization of any of the output contexts, the function will clean up all allocated 
 * resources (including those allocated for previously initialized output contexts) and return -1. 
 * If the function returns 0, the caller is responsible for eventually freeing the allocated PKSTOutProcCtx array.
 */
int pkst_open_multiple_ouputs_context(PKSTEncoderConfig *config, PKSTInputCtx *in_ctx, PKSTMultiOutCtx **out_ctx) {
   AVStream       *streams[2] = { 0 };
   AVCodecContext *encoder[2] = { 0 };
   
   PKSTOutProcCtx *ctx_p;
   int vindex;
   int aindex;
   int i; //, j;

    *out_ctx = pkst_alloc(sizeof(PKSTMultiOutCtx));
    memset(*out_ctx, 0, sizeof(PKSTMultiOutCtx));

    if (*out_ctx == NULL) return -1;

    (*out_ctx)->stats = pkst_alloc(sizeof(PKSTStats));
    if (!(*out_ctx)->stats) return -1;


    (*out_ctx)->stats->input_pkts  = 0;
    (*out_ctx)->stats->output_pkts = 0;
    (*out_ctx)->stats->start_time  = time(NULL);


    vindex = in_ctx->streams[PKST_VIDEO_STREAM_OUTPUT];
    aindex = in_ctx->streams[PKST_AUDIO_STREAM_OUTPUT];

    if (vindex != -1)
        streams[PKST_VIDEO_STREAM_OUTPUT] = in_ctx->in_ctx->streams[vindex];

    if (aindex != -1)
        streams[PKST_AUDIO_STREAM_OUTPUT] = in_ctx->in_ctx->streams[aindex];
 

    if (in_ctx->a_enc_ctx) {
        streams[PKST_AUDIO_STREAM_OUTPUT] = NULL;
        encoder[PKST_AUDIO_STREAM_OUTPUT] = in_ctx->a_enc_ctx->out_codec_ctx;
    } else {
        encoder[PKST_AUDIO_STREAM_OUTPUT] = NULL;
    }

    if (in_ctx->v_enc_ctx) {
        streams[PKST_VIDEO_STREAM_OUTPUT] = NULL;
        encoder[PKST_VIDEO_STREAM_OUTPUT] = in_ctx->a_enc_ctx->out_codec_ctx;
    } else {
        encoder[PKST_VIDEO_STREAM_OUTPUT] = NULL;
    }

    for ( i = 0; i < config->outs_len; i++) {

        ctx_p = pkst_alloc(sizeof(PKSTOutProcCtx));
        if (!ctx_p) goto cleanup;

        ctx_p->out_ctx = pkst_open_output_context(config->outs[i], streams, encoder);
        if (!ctx_p->out_ctx) goto cleanup;

        ctx_p->status = 0;
        ctx_p->onfail_ignore = config->outs[i]->onfail_ignore;
        ctx_p->dst = pkst_strdup(config->outs[i]->dst);
        (*out_ctx)->ctx[i] = ctx_p;
    }

    (*out_ctx)->ctx_len = i;
    return 0; 

cleanup:
    (*out_ctx)->ctx_len = i;
    pkst_close_outputs_contexts(out_ctx);

    return -1;
}

/**
 * @brief Clean up and close a PKSTOutputsCtxs.
 * @param out_ctxs A double pointer to the output contexts to be cleaned up and closed.
 * 
 * This function frees up the resources associated with a PKSTOutputsCtxs. 
 * It iterates through each PKSTOutProcCtx in the array, closing the associated 
 * AVFormatContext's pb if necessary, and frees the AVFormatContext. 
 * Finally, it frees each PKSTOutProcCtx and the PKSTOutputsCtxs itself. 
 * It also sets the double pointer to the PKSTOutputsCtxs to NULL.
 */
void pkst_close_outputs_contexts(PKSTMultiOutCtx **out_ctxs) {
    if (out_ctxs && *out_ctxs) {
        for (int i = 0; i < (*out_ctxs)->ctx_len; i++) {
            PKSTOutProcCtx *out_ctx = (*out_ctxs)->ctx[i];
            if (out_ctx) {
                if (out_ctx->out_ctx) {
                    if (!(out_ctx->out_ctx->oformat->flags & AVFMT_NOFILE)) {
                        avio_closep(&(out_ctx->out_ctx->pb));
                    }
                    avformat_free_context(out_ctx->out_ctx);
                }
                if (out_ctx->dst)
                    free(out_ctx->dst);
                free(out_ctx);
            }
        }
        if ((*out_ctxs)->stats)
            free((*out_ctxs)->stats);
        free(*out_ctxs);
        *out_ctxs = NULL;
    }
}

/**
 * @brief Send a packet to multiple output contexts.
 * 
 * This function sends a given AVPacket to multiple output contexts. It only sends the packet
 * to output contexts that have a status of 0 (indicating success). If an error occurs while
 * writing the frame to an output context, the status of the context is updated to the error
 * code.
 * 
 * @param packet The AVPacket to send.
 * @param in_timebase The input timebase from which the packet was read.
 * @param stream_type The type of the stream (audio, video, etc.).
 * @param outputs The array of output contexts.
 * @param out_len The length of the array of output contexts.
 * @return 0 on success, -1 if an error occurred.
 */
static int pkst_send_AVPacket_multiple_contexts(AVPacket *packet, AVRational in_timebase, int stream_type, PKSTOutProcCtx *outputs[], size_t out_len, int *fail) {
    AVStream *out_stream;
    AVPacket *pkt;

    PKSTOutProcCtx *out;
    int ret, i;

    if (!packet) return -1;

    for ( i = 0; i < out_len; i++ ) {
        out = outputs[i];
        if (out->status < 0)
            continue;
        pkt = av_packet_clone(packet);
        if (!pkt) 
            return -1;

        out_stream = out->out_ctx->streams[stream_type];
        pkt->stream_index = stream_type;
        av_packet_rescale_ts(pkt, in_timebase, out_stream->time_base);
        pkt->pos = -1;
        ret = av_interleaved_write_frame(out->out_ctx, pkt);
        if (ret < 0) {
            out->status = ret;
            if (fail)
                *fail = BOOL_TRUE;
            if (!out->onfail_ignore) {
                av_packet_free(&pkt);
                av_packet_unref(packet);
                return ret;
            }
        }    
        av_packet_free(&pkt);

    }
    av_packet_unref(packet);
    return 0;
}

/**
 * @brief Write trailers to multiple output contexts.
 * 
 * This function writes the trailer to each output context that has a status of 0 (indicating success).
 * If an error occurs while writing the trailer, the status of the context is updated to the error code.
 * 
 * @param outputs The array of output contexts.
 * @param out_len The length of the array of output contexts.
 */
void pkst_write_trailers_multiple_contexts(PKSTOutProcCtx *outputs[], size_t out_len) {
    PKSTOutProcCtx *out;
    int i;

    for (i = 0; i < out_len; i++) {
        out = outputs[i];
        if (out && out->status == 0 && out->out_ctx) {
            out->status = av_write_trailer(out->out_ctx);  
        }
    }
}

/**
 * @brief Reads a video or audio frame from the specified input context.
 * 
 * This function reads frames from the input context until it finds a frame
 * that belongs to either the video stream or the audio stream. Frames that
 * do not meet this criteria are unreferenced and discarded.
 * 
 * @param in The input context from which the frames will be read.
 * @param pkt The packet where the read frame will be stored.
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int pkst_read_packet(PKSTInputCtx *in, AVPacket *pkt) {
    int error;

    while(1) {
        error = av_read_frame(in->in_ctx, pkt);
        if (error < 0)
            return error;

        if (pkt->stream_index != in->streams[PKST_VIDEO_STREAM_OUTPUT] &&
            pkt->stream_index != in->streams[PKST_AUDIO_STREAM_OUTPUT]) {
            av_packet_unref(pkt);
            continue;
        }

        break;
    }
    return 0;
}


/**
 * @brief Process an audio packet from the input context and sends it to the output contexts.
 * 
 * This function decodes, converts, and stores an audio packet if the input context has an audio encoder context.
 * If the input context does not have an audio encoder context, it sends the packet to multiple output contexts.
 * In both scenarios, the function ensures that all the packets have been processed from the FIFO before returning.
 * 
 * @param pkt The audio packet to be processed.
 * @param in The input context from which the packet came from.
 * @param out The output contexts where the processed packet(s) will be sent.
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int pkst_process_audio_packet(AVPacket *pkt, PKSTInputCtx *in, PKSTMultiOutCtx *out, int *fail) {
    AVStream *in_stream = in->in_ctx->streams[pkt->stream_index];
    int stream_type = PKST_AUDIO_STREAM_OUTPUT;
    int error;

    out->stats->input_pkts++;

    if (in->a_enc_ctx) {
        // Reencode
        error = pkst_decode_convert_and_store(pkt, in->a_enc_ctx);
        av_packet_unref(pkt);
        if (error < 0) 
            return error;

        while(check_fifo_size(in->a_enc_ctx)) {
            error = pkst_load_encode_get_package(pkt, in->a_enc_ctx);
            if (error < 0) {
                return error;
            } else if (error > 0) {
                error = pkst_send_AVPacket_multiple_contexts(pkt, in->a_enc_ctx->out_codec_ctx->time_base,stream_type,out->ctx,  out->ctx_len, fail); // CAMBIAR BORRAR Remux
                if (error <0) return error;
                out->stats->output_pkts++;
            } 
        }
    } else {
        //Remux
        error = pkst_send_AVPacket_multiple_contexts(pkt, in_stream->time_base, stream_type, out->ctx, out->ctx_len, fail);
        if (error <0) return error;
        out->stats->output_pkts++;
    }
    return 0;
}


/**
 * Processes a video packet. If the video encoder context exists,
 * the function will perform a specific set of operations (currently not implemented).
 * If not, it will remux the packet and send it to multiple output contexts.
 *
 * @param pkt The packet to be processed
 * @param in The input context, which contains the packet's stream and encoder context
 * @param out The multi-output context, which contains the output contexts for the packet
 * @return 0 if successful, an error code otherwise
 */
static int pkst_process_video_packet(AVPacket *pkt, PKSTInputCtx *in, PKSTMultiOutCtx *out, int *fail) {
    AVStream *in_stream = in->in_ctx->streams[pkt->stream_index];
    int stream_type = PKST_VIDEO_STREAM_OUTPUT;
    int error;

    out->stats->input_pkts++;

    if (in->v_enc_ctx) {
        // Nothing
    } else {;
        error = pkst_send_AVPacket_multiple_contexts(pkt, in_stream->time_base, stream_type, out->ctx, out->ctx_len, fail);
        if (error <0) return error;
        out->stats->output_pkts++;
    }
    return 0;
}


int pkst_flush_audio_encoder_queue(PKSTInputCtx *in, PKSTMultiOutCtx *out) {
    int stream_type = PKST_AUDIO_STREAM_OUTPUT;
    AVPacket *pkt = NULL;
    int error = 0;
    
    pkt = av_packet_alloc();
    if (!pkt)
        return -1;

    error = pkst_flush_audio_decoder(in->a_enc_ctx);
    if (error == 0) {
        error = pkst_flush_audio_fifo(in->a_enc_ctx);
        if (error == 0) {
            while (avcodec_receive_packet(in->a_enc_ctx->out_codec_ctx, pkt) == 0) {
                error = pkst_send_AVPacket_multiple_contexts(pkt, 
                                                             in->a_enc_ctx->out_codec_ctx->time_base,
                                                             stream_type, 
                                                             out->ctx, 
                                                             out->ctx_len, NULL);
                if (error <0) break;
            }
        }
    }
    av_packet_free(&pkt);
    return error;
}

int pkst_process_av_packet(PKSTts *ts, AVPacket *pkt, PKSTInputCtx *in, PKSTMultiOutCtx *out, int *output_fail) {
    int error;

    if (!(error = pkst_read_packet(in, pkt))) { 
        if (ts->first_packet) {
            ts->start_pts = pkt->pts;
            ts->start_dts = pkt->dts;
            ts->first_packet = 0;
        }
        pkt->pts = pkt->pts - ts->start_pts;
        pkt->dts = pkt->dts - ts->start_dts;
        if (pkt->stream_index == in->streams[PKST_AUDIO_STREAM_OUTPUT]) {
            error = pkst_process_audio_packet(pkt, in, out, output_fail);
        } else {
            error = pkst_process_video_packet(pkt, in, out, output_fail);
        }
    }
    return error;
}

void dump_multi_out_ctx(const PKSTMultiOutCtx *multi_out_ctx) {
    int i;

    if (!multi_out_ctx) return;

    time_t current_time;
    struct tm * local_time_info;
    char time_str[26];  
    

    time(&current_time);
    local_time_info = localtime(&current_time);

    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", local_time_info);
    pkst_log(NULL,0,"%s: [STATS] \n", time_str);

    PKSTStats *stats = multi_out_ctx->stats;
    double elapsed_time = difftime(stats->current_time, stats->start_time);
    double diff_time = difftime(stats->current_time, stats->start_time);
    double input_speed = diff_time > 0 ? stats->input_pkts / diff_time : 0;
    double output_speed = diff_time > 0 ? stats->output_pkts / diff_time : 0;

    for (i = 0; i < multi_out_ctx->ctx_len; i++) {
        PKSTOutProcCtx *out_proc_ctx = multi_out_ctx->ctx[i];

        // Obtain error string from av_strerror function
        char error[AV_ERROR_MAX_STRING_SIZE] = {0};
        if (out_proc_ctx->status != 0)
            av_strerror(out_proc_ctx->status, error, AV_ERROR_MAX_STRING_SIZE);

        pkst_log(NULL,0,"\t%d - Destination: %s\tStatus: %d\tIgnore Fail: %d\tError: %s\n", i,
                out_proc_ctx->dst, 
                out_proc_ctx->status, 
                out_proc_ctx->onfail_ignore, 
                error);
    }
    pkst_log(NULL,0,"\tInput packets: %8d, Speed: %8.2f packets/s, Output packets: %8d, Speed: %8.2f packets/s, Elapsed time: %8.2f s\n", 
            stats->input_pkts, input_speed, stats->output_pkts, output_speed, elapsed_time);
}

char *dump_multi_out_ctx_json(const PKSTMultiOutCtx *multi_out_ctx, int error) {
    int i;

    if (!multi_out_ctx) return NULL;

    time_t current_time;

    time(&current_time);

    PKSTStats *stats = multi_out_ctx->stats;
    double elapsed_time = difftime(stats->current_time, stats->start_time);
    double diff_time = difftime(stats->current_time, stats->start_time);
    double input_speed = diff_time > 0 ? stats->input_pkts / diff_time : 0;
    double output_speed = diff_time > 0 ? stats->output_pkts / diff_time : 0;

    // Create a new json object
    json_t *jobj = json_object();

    // Add current time
    json_object_set_new(jobj, "timestamp", json_integer((long int)current_time));

    // Add stats
    json_t *jarray = json_array();

    for (i = 0; i < multi_out_ctx->ctx_len; i++) {
        PKSTOutProcCtx *out_proc_ctx = multi_out_ctx->ctx[i];

        // Obtain error string from av_strerror function
        char error[AV_ERROR_MAX_STRING_SIZE] = {0};
        if (out_proc_ctx->status != 0)
            av_strerror(out_proc_ctx->status, error, AV_ERROR_MAX_STRING_SIZE);

        // Create a new json object for each context
        json_t *jctx = json_object();
        json_object_set_new(jctx, "destination", json_string(out_proc_ctx->dst));
        json_object_set_new(jctx, "status", json_integer(out_proc_ctx->status));
        json_object_set_new(jctx, "ignore_fail", json_boolean(out_proc_ctx->onfail_ignore));
        json_object_set_new(jctx, "error", json_string(error));

        // Add the context to the array
        json_array_append_new(jarray, jctx);
    }

    // Add the array of contexts to the main object
    json_object_set_new(jobj, "outputs", jarray);

    // Add speed and elapsed time to the main object
    json_object_set_new(jobj, "error", json_integer(error));
    json_object_set_new(jobj, "input_packets", json_integer(stats->input_pkts));
    json_object_set_new(jobj, "input_Speed", json_real(input_speed));
    json_object_set_new(jobj, "output_packets", json_integer(stats->output_pkts));
    json_object_set_new(jobj, "output_Speed", json_real(output_speed));
    json_object_set_new(jobj, "eta", json_real(elapsed_time));

    // Dump the json object to a string
    char *json_str = json_dumps(jobj, JSON_ENCODE_ANY);

    // Decrease the reference count to the json object because we don't need it anymore
    json_decref(jobj);

    return json_str;
}
