#include <stdio.h>
#include <stdlib.h>


#include "pkst_iocontext.h"
#include "pkst_strings.h"
#include "pkst_stats.h"
#include "pkst_msgproto.h"
#include <jansson.h>
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
    PKSTEncoderConfig *new_enc = malloc(sizeof(PKSTEncoderConfig));
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
    new_enc->in = in ? pkst_strdup(in) : NULL;
    // If input type string is not NULL, duplicate the string, else set to NULL
    new_enc->in_type = in_type ? pkst_strdup(in_type) : NULL;
    // If tcpstats string is not NULL, duplicate the string, else set to NULL
    new_enc->tcpstats = tcpstats ? pkst_strdup(tcpstats) : NULL;

    // If any of the strdup operations failed, free all allocated memory and return -1
    if ((in && new_enc->in == NULL) || (in_type && new_enc->in_type == NULL) || (tcpstats && new_enc->tcpstats == NULL)) {
        if (new_enc->in) free(new_enc->in);
        if (new_enc->in_type) free(new_enc->in_type);
        if (new_enc->tcpstats) free(new_enc->tcpstats);
        free(new_enc);
        return -1;
    }
    new_enc->timeout = timeout;
    new_enc->listen  = listen;
    new_enc->kv_audio_req = NULL;
    new_enc->kv_video_req = NULL;
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
    ctx = malloc(sizeof(PKSTOutputConfig));
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


/* 
 * Sets the Key-Value audio requirements in the Encoder Configuration structure.
 *
 * Parameters:
 * - enc: The pointer to the PKSTEncoderConfig structure in which the audio requirements need to be set.
 * - req: The string containing the Key-Value audio requirements.
 *
 * This function takes the Key-Value audio requirements as a string and stores it in the PKSTEncoderConfig structure. 
 * It does so by duplicating the string (so the original can be safely modified or freed) and storing the copy.
 * 
 * If the passed encoder config or the requirements string is NULL, the function will return -1.
 * 
 * Return value:
 * On success, the function returns 0. 
 * If the encoder config or the requirements string is NULL, the function will return -1.
 */
int pkst_set_kv_audio_requirements(PKSTEncoderConfig *enc, const char *req) {
    if (!enc || !req) 
        return -1;
    
    enc->kv_audio_req = pkst_strdup(req);
    return 0;
}

/**
 * @brief Add an audio encoder configuration to an encoder configuration structure.
 *
 * This function duplicates the input audio configuration and stores it in the encoder
 * configuration structure. The audio configuration includes details such as the audio codec,
 * bitrate, number of channels, and sample rate.
 *
 * @param enc The encoder configuration structure to which the audio configuration is added.
 * @param config The audio configuration to be duplicated and added.
 *
 * @return Returns 0 on successful addition, -1 on error (for instance, if allocation fails or if the inputs are NULL).
 */
int pkst_add_audio_encoder_config(PKSTEncoderConfig *enc, PKSTAudioConfig *config) {
    if (!enc || !config)
        return -1; // Retornar algún código de error o hacer un manejo de errores adecuado.

    enc->audio_config = malloc(sizeof(PKSTAudioConfig));
    if (!enc->audio_config)
        return -1; // Error en la asignación de memoria.

    enc->audio_config->codec = pkst_strdup(config->codec);
    if (!enc->audio_config->codec) {
        free(enc->audio_config);
        enc->audio_config = NULL;
        return -1; // Error al duplicar el string.
    }

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
        printf("Encoder Context: NULL\n");
        return;
    }

    printf("Encoder Context:\n");
    printf("    Input: %s\n", enc->in ? enc->in : "NULL");
    printf("    Input Type: %s\n", enc->in_type ? enc->in_type : "NULL");
    printf("    Listen: %s\n", enc->listen ? "true" : "false");
    printf("    Network timeout: %d (milliseconds)\n", enc->timeout);
    printf("    TCP Stats: %s\n", enc->tcpstats ? enc->tcpstats : "NULL");
    printf("    Audio Requirements: %s\n", enc->kv_audio_req ? enc->kv_audio_req : "NULL");
    printf("    Video Requirements: %s\n", enc->kv_video_req ? enc->kv_video_req : "NULL");
    printf("    Output Length: %d\n", enc->outs_len);

    if (enc->audio_config) {
        printf("    Audio Config:\n");
        printf("        Audio Codec: %s\n", enc->audio_config->codec ? enc->audio_config->codec : "NULL");
        printf("        Audio Bitrate (bps): %d\n", enc->audio_config->bitrate_bps);
        printf("        Audio Channels: %d\n", enc->audio_config->channels);
        printf("        Sample Rate: %d\n", enc->audio_config->sample_rate);
    }

    if (enc->video_config) {
        // Here you can add the printing of video_config fields, similar to audio_config.
    }

    for (i = 0; i < enc->outs_len; i++) {
        printf("    Output[%d]:\n", i);
        printf("        Destination: %s\n", enc->outs[i]->dst ? enc->outs[i]->dst : "NULL");
        printf("        Destination Type: %s\n", enc->outs[i]->dst_type ? enc->outs[i]->dst_type : "NULL");
        printf("        KV Options: %s\n", enc->outs[i]->kv_opts ? enc->outs[i]->kv_opts : "NULL");
        printf("        On Fail Ignore: %d\n", enc->outs[i]->onfail_ignore);
    }
}

/* 
 * This function deallocates a PKSTOutputConfig structure. 
 * It receives a pointer to a PKSTOutputConfig and frees all its members and itself.
 * Note that after calling this function, the pointer is no longer valid and should not be used.
 */
void pkst_free_output_config(PKSTOutputConfig *out) {
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
    if (*enc) {
        if ((*enc)->in) {
            free((*enc)->in);
        }
        if ((*enc)->in_type) {
            free((*enc)->in_type);
        }
        if ((*enc)->tcpstats) {
            free((*enc)->tcpstats);
        }
        if ((*enc)->kv_audio_req) {
            free((*enc)->kv_audio_req);
        }
        if ((*enc)->kv_video_req) {
            free((*enc)->kv_video_req);
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

    if ((*ctx)->in_ctx) {
        avformat_close_input(&((*ctx)->in_ctx));
    }

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
        if (config->timeout) {
            sprintf(timeout, "%d", config->timeout * 1000);
            av_dict_set(&options, "listen_timeout", timeout, 0);
        }
    }

    if (config->in_type) {
        infmt = av_find_input_format(config->in_type);
        if (!infmt) return -1;
    }

    *ctx = malloc(sizeof(PKSTInputCtx));
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
AVFormatContext *pkst_open_output_context(PKSTOutputConfig *config, AVStream *remux[2], AVCodecContext *encoder[2]) {
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
                out_stream->time_base.den = 44100; //input_codec_context->sample_rate;
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
            dump_kv_list(kv_opts);
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
 * @brief Open an output context for remuxing based on the provided configuration and input AVStreams.
 * @param config The output configuration used to initialize the output context.
 * @param in_avstream An array of input AVStreams. Expected to be of size 2, with in_avstream[0] being the video AVStream and in_avstream[1] being the audio AVStream.
 * @return AVFormatContext* The initialized output context. NULL if there was an error during initialization.
 * 
 * This function will allocate an AVFormatContext for output based on the provided PKSTOutputConfig 'config' and input AVStreams. 
 * It will also open the output file (if necessary) and write the output format header.
 * The function will free all allocated resources and return NULL if there's an error at any point.
 * If the function returns a non-NULL AVFormatContext, the caller is responsible for eventually freeing the AVFormatContext using avformat_free_context().
 
AVFormatContext *pkst_open_output_context_for_remux(PKSTOutputConfig *config, AVStream *in_avstream[2]) {
    const AVOutputFormat *ofmt;
    AVFormatContext *ctx = NULL;
    AVDictionary *opts = NULL;
    AVStream *out_stream = NULL;
    KeyValueList *kv_opts;
    int i, ret = 0;

    avformat_alloc_output_context2(&ctx, NULL, config->dst_type, config->dst);
    if (!ctx) return NULL;

    ofmt = ctx->oformat;

    if (in_avstream[PKST_VIDEO_STREAM_OUTPUT]) {
        out_stream = avformat_new_stream(ctx, NULL);
        if (!out_stream) goto cleanup;

        ret = avcodec_parameters_copy(out_stream->codecpar, in_avstream[PKST_VIDEO_STREAM_OUTPUT]->codecpar);
        if (ret < 0) goto cleanup;

        out_stream->codecpar->codec_tag = 0;
        av_dict_set(&(out_stream->metadata), "handler_name",HANDLER_NAME, 0);
    }

    if (in_avstream[PKST_AUDIO_STREAM_OUTPUT]) {
        out_stream = avformat_new_stream(ctx, NULL);
        if (!out_stream) goto cleanup;

        ret = avcodec_parameters_copy(out_stream->codecpar, in_avstream[PKST_AUDIO_STREAM_OUTPUT]->codecpar);
        if (ret < 0) goto cleanup;

        out_stream->codecpar->codec_tag = 0;
        av_dict_set(&(out_stream->metadata), "handler_name", HANDLER_NAME, 0);
    }

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ctx->pb, config->dst, AVIO_FLAG_WRITE);
        if (ret < 0) goto cleanup;
    }

    if (config->kv_opts) {
        kv_opts = parse_kv_list(config->kv_opts, PKST_PAIR_DELIM, PKST_KV_DELIM);
        if (kv_opts) {
            dump_kv_list(kv_opts);
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
*/
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
   AVStream       *streams[2];
   AVCodecContext *encoder[2];
   
   PKSTOutProcCtx *ctx_p;
   int vindex;
   int aindex;
   int i, j;

    *out_ctx = malloc(sizeof(PKSTMultiOutCtx));
    memset(*out_ctx, 0, sizeof(PKSTMultiOutCtx));

    if (*out_ctx == NULL) return -1;

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

        ctx_p = malloc(sizeof(PKSTOutProcCtx));
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
    for (j = 0; j < i; j++) {
        if ((*out_ctx)->ctx[j]) {
            if ((*out_ctx)->ctx[j]->dst)
                free((*out_ctx)->ctx[j]->dst);
            if ((*out_ctx)->ctx[j]->out_ctx) {
                if (!((*out_ctx)->ctx[j]->out_ctx->oformat->flags & AVFMT_NOFILE)) {
                    avio_closep(&((*out_ctx)->ctx[j]->out_ctx->pb));
                }
                avformat_free_context((*out_ctx)->ctx[j]->out_ctx);
            }
            free((*out_ctx)->ctx[j]);
        }
    }
    free(*out_ctx);
    *out_ctx = NULL;
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
int pkst_send_AVPacket_multiple_contexts(AVPacket *packet, AVRational in_timebase, int stream_type, PKSTOutProcCtx *outputs[], size_t out_len) {
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
            if (!out->onfail_ignore) {
                av_packet_unref(pkt);
                av_packet_unref(packet);
                return ret;
            }
        }    
        av_packet_unref(pkt);
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
 * @brief Remuxes an input context into multiple output contexts.
 * 
 * @param in Pointer to the input context (PKSTInputCtx).
 * @param out Pointer to the output contexts (PKSTMultiOutCtx).
 * 
 * @return int Returns 0 on success, AVERROR_EOF when the end of the input stream is reached,
 *         -1 if either input or output context is NULL or packet allocation failed,
 *         or any other error codes from the av_read_frame() or pkst_send_AVPacket_multiple_contexts() functions.
 * 
 * The function loops over the input stream, reading frames and sending them to multiple output contexts.
 * If a packet is not of the video or audio type, it is discarded. If a packet is of the video type, it is sent to 
 * all output contexts that handle video, and if it's of the audio type, it is sent to all output contexts that handle audio.
 * The function breaks out of the loop when an error is encountered in reading a frame or sending a packet to an output context.
 * 
 * Note: The function does not handle the case where the input stream is neither video nor audio, in such case, the packet is simply ignored.

int pkst_audiovideo_remux(PKSTInputCtx *in, PKSTMultiOutCtx *out) {
    AVStream *in_stream;
    AVPacket *pkt = NULL;
    int stream_type;
    int ret;

    if (!in || !out) return -1;

    pkt = av_packet_alloc();
    if (!pkt)
        return -1;
         
    while (1) {
        ret = av_read_frame(in->in_ctx, pkt);
        if (ret < 0)
            break;
    
        if (pkt->stream_index != in->streams[PKST_VIDEO_STREAM_OUTPUT] &&
            pkt->stream_index != in->streams[PKST_AUDIO_STREAM_OUTPUT]) {
            av_packet_unref(pkt);
            continue;
        }
        in_stream = in->in_ctx->streams[pkt->stream_index];
        stream_type = pkt->stream_index == in->streams[PKST_VIDEO_STREAM_OUTPUT] ? PKST_VIDEO_STREAM_OUTPUT : PKST_AUDIO_STREAM_OUTPUT;
        ret = pkst_send_AVPacket_multiple_contexts(pkt, in_stream->time_base, stream_type, out->ctx, out->ctx_len);
        if (ret < 0)
            break;
    }
    av_packet_free(&pkt);
    return (ret == AVERROR_EOF) ? 0 : ret;
}
*/

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
static int pkst_process_audio_packet(AVPacket *pkt, PKSTInputCtx *in, PKSTMultiOutCtx *out, PKSTStats *stats) {
    AVStream *in_stream = in->in_ctx->streams[pkt->stream_index];
    int stream_type = PKST_AUDIO_STREAM_OUTPUT;
    int error;

    stats->input_pkts++;

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
                error = pkst_send_AVPacket_multiple_contexts(pkt, in->a_enc_ctx->out_codec_ctx->time_base,stream_type,out->ctx,  out->ctx_len); // CAMBIAR BORRAR Remux
                if (error <0) return error;
                stats->output_pkts++;
            } 
        }
    } else {
        //Remux
        error = pkst_send_AVPacket_multiple_contexts(pkt, in_stream->time_base, stream_type, out->ctx, out->ctx_len);
        if (error <0) return error;
        stats->output_pkts++;
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
static int pkst_process_video_packet(AVPacket *pkt, PKSTInputCtx *in, PKSTMultiOutCtx *out, PKSTStats *stats) {
    AVStream *in_stream = in->in_ctx->streams[pkt->stream_index];
    int stream_type = PKST_VIDEO_STREAM_OUTPUT;
    int error;

    stats->input_pkts++;

    if (in->v_enc_ctx) {
        // Nothing
    } else {;
        error = pkst_send_AVPacket_multiple_contexts(pkt, in_stream->time_base, stream_type, out->ctx, out->ctx_len);
        if (error <0) return error;
        stats->output_pkts++;
    }
    return 0;
}


static int pkst_flush_audio_encoder_queue(PKSTInputCtx *in, PKSTMultiOutCtx *out) {
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
                                                             out->ctx_len);
                if (error <0) break;
            }
        }
    }
    av_packet_free(&pkt);
    return error;
}

/**
 * @brief This function reads packets from the input context, decodes them, re-encodes (if required), and sends them to multiple output contexts.
 *
 * @param *in This is a pointer to the input context, which includes the AVFormatContext for the input file and other related information.
 * @param *out This is a pointer to the multiple output contexts, each containing the AVFormatContext for an output file and other related information.
 *
 * @return 0 on successful completion of the function. If the function encounters AVERROR_EOF, it implies the end of the input file has been reached, and thus it also returns 0. 
 *         For other error codes, it returns the corresponding FFmpeg error code.
 * 
 * @note This function will ignore packets that are not audio or video packets. In the case of audio packets, if a re-encoding context is present in the input context,
 *       the function will decode the packet, convert the raw data, store it in a FIFO buffer for later encoding, and finally re-encode the audio into packets which are sent to the output contexts.
 *       If no re-encoding context is present, the function will simply send the audio packets to the output contexts.
 *       For video packets, they are directly sent to the output contexts without any decoding or re-encoding.
 */
int pkst_audiovideo_process(PKSTInputCtx *in, PKSTMultiOutCtx *out, int socket) {
    PKSTStats stats;
    char *msg = NULL;
    AVPacket *pkt = NULL;
    int error;
    
    if (!in || !out) return -1;

    stats.input_pkts = 0;
    stats.output_pkts = 0;
    stats.start_time = time(NULL);

    pkt = av_packet_alloc();
    if (!pkt)
        return AVERROR(ENOMEM);

    while (1) {
        error = pkst_read_packet(in, pkt);
        if (error < 0) break;

        if (pkt->stream_index == in->streams[PKST_AUDIO_STREAM_OUTPUT]) {
            error = pkst_process_audio_packet(pkt, in, out, &stats);
        } else {
            error = pkst_process_video_packet(pkt, in, out, &stats);
        }

        if (error < 0) goto cleanup;

        stats.current_time = time(NULL);

        if (difftime(stats.current_time, stats.start_time) >= 5) {            
            if (socket) {
                msg = pkst_stats_to_json(&stats);
                if (pkst_send_data(socket, msg, MESSAGE_TYPE_JSON | MESSAGE_REPORT_STATS) < 0) 
                    socket = 0;
                free(msg);
                msg = NULL;
            } else {
                pkts_print_stats(&stats);
            }
        }
    }

    if (in->a_enc_ctx)
        error = pkst_flush_audio_encoder_queue(in, out);
cleanup:    
    av_packet_free(&pkt);
    return (error == AVERROR_EOF) ? 0 : error;
}


char *pkst_out_report_to_json(const PKSTMultiOutCtx *report, int status) {
    char error[AV_ERROR_MAX_STRING_SIZE] = {0};
    PKSTOutProcCtx *ctx;
    json_t *jobj = json_object();
    json_t *jarray = json_array();
    json_t *jctx;
    char *result = NULL;

    // Asegúrate de comprobar que report no es NULL antes de acceder a sus miembros
    if (report) {
        // Convertir el código de estado a un mensaje de error
        if (av_strerror(status, error, sizeof(error)) < 0) {
            sprintf(error, "Unrecognized error code: %d", status);
        }

        // Establecer campos de nivel raíz
        json_object_set_new(jobj, "status", json_integer(status));
        json_object_set_new(jobj, "error", json_string(error));

        for (int i = 0; i < report->ctx_len; i++) {
            ctx = report->ctx[i];

            if (ctx) {
                jctx = json_object();
                json_object_set_new(jctx, "dst", json_string(ctx->dst));
                json_object_set_new(jctx, "status", json_integer(ctx->status));
                json_object_set_new(jctx, "onfail_ignore", json_integer(ctx->onfail_ignore));

                if (av_strerror(ctx->status, error, AV_ERROR_MAX_STRING_SIZE) < 0) {
                    sprintf(error, "Unrecognized error code: %d", ctx->status);
                }
                json_object_set_new(jctx, "error", json_string(error));

                json_array_append_new(jarray, jctx);
            }
        }
        json_object_set_new(jobj, "OutputReport", jarray);
    }

    result = json_dumps(jobj, JSON_ENCODE_ANY);

    // Libera la memoria del objeto JSON
    json_decref(jobj);

    return result;
}