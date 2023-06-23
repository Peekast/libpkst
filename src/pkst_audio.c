#include "pkst_audio.h"

/**
 * @brief Open an audio decoder using an input AVStream.
 * 
 * @param in_audio_stream The input AVStream used to configure the decoder.
 * @param in_codec_ctx A pointer to a pointer to the AVCodecContext structure to be initialized.
 * 
 * @return int A return code. 0 indicates success. Any other value indicates an error.
 * 
 * This function will find the appropriate decoder based on the input AVStream's codec_id,
 * allocate and initialize an AVCodecContext structure for the decoder, and open the decoder.
 * It will also set the packet timebase for the decoder.
 * On success, it will save a pointer to the AVCodecContext structure in *in_codec_ctx.
 * If the function fails at any point, it will free any resources that were allocated during its execution and return a non-zero error code.
 * If the function returns a non-zero error code, *in_codec_ctx will be NULL.
 * If the function returns 0, *in_codec_ctx will be a pointer to the initialized AVCodecContext structure.
 * The caller is responsible for eventually freeing the AVCodecContext structure using avcodec_free_context().
 */
static int pkst_open_audio_decoder(const AVStream *in_audio_stream, AVCodecContext **in_codec_ctx) {
    AVCodecContext *in_avctx = NULL;
    const AVCodec  *input_codec = NULL;
    int error;

    /* Busca el codec de entrada */
    if (!(input_codec = avcodec_find_decoder(in_audio_stream->codecpar->codec_id)))
        return AVERROR_EXIT;

    /* Guarda espacio para el contexto */
    in_avctx = avcodec_alloc_context3(input_codec);
    if (!in_avctx)
        return AVERROR(ENOMEM);

    /* Copia los parametros del contexto */
    error = avcodec_parameters_to_context(in_avctx, in_audio_stream->codecpar);
    if (error < 0) {
        avcodec_free_context(&in_avctx);
        return error;
    }

    /* Open the decoder for the audio stream to use it later. */
    error = avcodec_open2(in_avctx, input_codec, NULL);
    if (error < 0) {
        avcodec_free_context(&in_avctx);
        return error;
    }

    /* Set the packet timebase for the decoder. */
    in_avctx->pkt_timebase = in_audio_stream->time_base;

    in_avctx->time_base = in_audio_stream->time_base; // BORRAR
    /* Save the decoder context for easier access later. */
    *in_codec_ctx = in_avctx;

    return 0;
}

/**
 * Initialize the audio resampler based on the input and output codec settings.
 * If the input and output sample formats differ, a conversion is required
 * libswresample takes care of this, but requires initialization.
 * @param      input_codec_context  Codec context of the input file
 * @param      output_codec_context Codec context of the output file
 * @param[out] resample_context     Resample context for the required conversion
 * @return Error code (0 if successful)
 */
static int pkst_init_resampler(AVCodecContext *input_codec_context,
                               AVCodecContext *output_codec_context,
                               SwrContext **resample_context)
{
        int error;

        /*
         * Create a resampler context for the conversion.
         * Set the conversion parameters.
         */
        error = swr_alloc_set_opts2(resample_context,
                                    &output_codec_context->ch_layout,
                                    output_codec_context->sample_fmt,
                                    output_codec_context->sample_rate,
                                    &input_codec_context->ch_layout,
                                    input_codec_context->sample_fmt,
                                    input_codec_context->sample_rate,
                                    0, NULL);
        if (error < 0)
            return error;
        /*
        * Perform a sanity check so that the number of converted samples is
        * not greater than the number of samples to be converted.
        * If the sample rates differ, this case has to be handled differently
        */
        //av_assert0(output_codec_context->sample_rate == input_codec_context->sample_rate);

        /* Open the resampler with the specified parameters. */
        if ((error = swr_init(*resample_context)) < 0) {
            swr_free(resample_context);
            return error;
        }
    return 0;
}

/**
 * Initialize a FIFO buffer for the audio samples to be encoded.
 * @param[out] fifo                 Sample buffer
 * @param      output_codec_context Codec context of the output file
 * @return Error code (0 if successful)
 */
static int pkst_init_audio_fifo(AVAudioFifo **fifo, AVCodecContext *output_codec_context)
{
    /* Create the FIFO buffer based on the specified output sample format. */
    if (!(*fifo = av_audio_fifo_alloc(output_codec_context->sample_fmt,
                                      output_codec_context->ch_layout.nb_channels, 1))) {
        return AVERROR(ENOMEM);
    }
    return 0;
}

static int pkst_open_audio_encoder(AVCodecContext **out_codec_ctx, const PKSTAudioConfig *config) {
    AVCodecContext *out_avctx = NULL;
    const AVCodec  *output_codec = NULL;
    int error;

    /* Find the encoder to be used by its name. */
    if (!(output_codec = avcodec_find_encoder_by_name(config->codec))) { //} avcodec_find_encoder(AV_CODEC_ID_AAC))) {
        *out_codec_ctx = NULL;
        return -1;
    }

    out_avctx = avcodec_alloc_context3(output_codec);
    if (!out_avctx) {
        *out_codec_ctx = NULL;
        return AVERROR(ENOMEM);
    }
    /* Set the basic encoder parameters.
     * The input file's sample rate is used to avoid a sample rate conversion. */
    av_channel_layout_default(&out_avctx->ch_layout, config->channels);
    out_avctx->sample_rate    = config->sample_rate; //44100;//input_codec_context->sample_rate;
    out_avctx->sample_fmt     = output_codec->sample_fmts[0];
    out_avctx->bit_rate       = config->bitrate_bps;
    out_avctx->time_base      = (AVRational){1, out_avctx->sample_rate}; // BORRAR SI NOVA
    out_avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;       // BORRAR SI NOVA

    /* Open the encoder for the audio stream to use it later. */
    if ((error = avcodec_open2(out_avctx, output_codec, NULL)) < 0) {
        *out_codec_ctx = NULL;
        avcodec_free_context(&out_avctx);
        return error;
    }

    /* Save the encoder context for easier access later. */
    *out_codec_ctx = out_avctx;
    return 0;
}

/**
 * Initialize one audio frame for reading from the input file.
 * @param[out] frame Frame to be initialized
 * @return Error code (0 if successful)
 */
static int init_input_frame(AVFrame **frame)
{
    if (!(*frame = av_frame_alloc())) {
        return AVERROR(ENOMEM);
    }
    return 0;
}

/**
 * Initialize one input frame for writing to the output file.
 * The frame will be exactly frame_size samples large.
 * @param[out] frame                Frame to be initialized
 * @param      output_codec_context Codec context of the output file
 * @param      frame_size           Size of the frame
 * @return Error code (0 if successful)
 */
static int init_output_frame(AVFrame **frame,
                             AVCodecContext *output_codec_context,
                             int frame_size)
{
    int error;

    /* Create a new frame to store the audio samples. */
    if (!(*frame = av_frame_alloc())) {
        return AVERROR(ENOMEM);
    }

    /* Set the frame's parameters, especially its size and format.
     * av_frame_get_buffer needs this to allocate memory for the
     * audio samples of the frame.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity. */
    (*frame)->nb_samples     = frame_size;
    av_channel_layout_copy(&(*frame)->ch_layout, &output_codec_context->ch_layout);
    (*frame)->format         = output_codec_context->sample_fmt;
    (*frame)->sample_rate    = output_codec_context->sample_rate;

    /* Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified. */
    if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
        av_frame_free(frame);
        return error;
    }

    return 0;
}


/**
 * Initialize a temporary storage for the specified number of audio samples.
 * The conversion requires temporary storage due to the different format.
 * The number of audio samples to be allocated is specified in frame_size.
 * @param[out] converted_input_samples Array of converted samples. The
 *                                     dimensions are reference, channel
 *                                     (for multi-channel audio), sample.
 * @param      output_codec_context    Codec context of the output file
 * @param      frame_size              Number of samples to be converted in
 *                                     each round
 * @return Error code (0 if successful)
 */
static int init_converted_samples(uint8_t ***converted_input_samples,
                                  AVCodecContext *output_codec_context,
                                  int frame_size)
{
    int error;

    /* Allocate as many pointers as there are audio channels.
     * Each pointer will point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     * Allocate memory for the samples of all channels in one consecutive
     * block for convenience. */
    if ((error = av_samples_alloc_array_and_samples(converted_input_samples, NULL,
                                  output_codec_context->ch_layout.nb_channels,
                                  frame_size,
                                  output_codec_context->sample_fmt, 0)) < 0) {
        return error;
    }
    return 0;
}

/**
 * Convert the input audio samples into the output sample format.
 * The conversion happens on a per-frame basis, the size of which is
 * specified by frame_size.
 * @param      input_data       Samples to be decoded. The dimensions are
 *                              channel (for multi-channel audio), sample.
 * @param[out] converted_data   Converted samples. The dimensions are channel
 *                              (for multi-channel audio), sample.
 * @param      frame_size       Number of samples to be converted
 * @param      resample_context Resample context for the conversion
 * @return Error code (0 if successful)
 */
static int convert_samples(const uint8_t **input_data,
                           uint8_t **converted_data, const int frame_size,
                           SwrContext *resample_context)
{


    /* Convert the samples using the resampler. */
    return swr_convert(resample_context,
                       converted_data, frame_size,
                       input_data    , frame_size);
}

/**
 * Add converted input audio samples to the FIFO buffer for later processing.
 * @param fifo                    Buffer to add the samples to
 * @param converted_input_samples Samples to be added. The dimensions are channel
 *                                (for multi-channel audio), sample.
 * @param frame_size              Number of samples to be converted
 * @return Error code (0 if successful)
 */
static int add_samples_to_fifo(AVAudioFifo *fifo,
                               uint8_t **converted_input_samples,
                               const int frame_size)
{
    int error;

    /* Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples. */
    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
        return error;
    }

    /* Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void **)converted_input_samples,
                            frame_size) < frame_size) {
        return AVERROR_EXIT;
    }
    return 0;
}

/**
 * @brief Decode an input packet into a frame.
 * @param input_packet The input packet to be decoded.
 * @param frame The output frame where the decoded data will be stored.
 * @param input_codec_context The input codec context for the decoding operation.
 * @return 0 if the decoding was successful but didn't produce a frame, 1 if a frame was produced, and a negative AVERROR code on failure.
 */
static int pkst_decode_package(AVPacket *input_packet, AVFrame *frame, AVCodecContext *input_codec_context) {
    int error;

    /* Send the input packet to the decoder */
    error = avcodec_send_packet(input_codec_context, input_packet);
    if (error < 0)
        return error; // Return on any error from avcodec_send_packet

    // Clean de packet.
    av_packet_unref(input_packet);

    /* Try to receive a frame from the decoder */
    error = avcodec_receive_frame(input_codec_context, frame);
    if (error < 0) {
        /* If the decoder requires more data to produce a frame, we return 0 */
        if (error == AVERROR(EAGAIN))
            error = 0;
    } else {
        /* If a frame was successfully produced, we return 1 */
        error = 1;
    }
    return error;
}


/**
 * This function is responsible for encoding an audio frame and placing the encoded packet into the
 * provided output container.
 *
 * @param input_frame The input frame to be encoded. If NULL, a NULL frame will be sent
 * to the encoder to signal that no more input data is expected.
 * @param output_packet[out] A packet which will hold the encoded audio data after this function returns.
 * @param output_codec_context The context of the output encoder. This must be initialized
 * and ready to encode audio frames.
 * @return 0 if the encoder did not return any packet, 1 if a packet was returned, or a negative
 * error value if an error occurred. Errors are returned as libav error codes.
 */
static int pkst_encode_audio_frame(AVFrame *input_frame, AVPacket *output_packet, int64_t *pts, AVCodecContext *output_codec_context) {
    int error;
    int64_t duration; 

    input_frame->best_effort_timestamp = av_rescale_q(*pts, output_codec_context->time_base, (AVRational){1, 44000});
    input_frame->pts = *pts;

//    printf("BEF output:%d\n", input_frame->best_effort_timestamp);
//    fflush(stdout);

    duration = av_rescale(AV_TIME_BASE, input_frame->nb_samples, input_frame->sample_rate);
    duration = av_rescale_q(duration, av_get_time_base_q(), output_codec_context->time_base);
    *pts += duration;

    /* Set a timestamp based on the sample rate for the container. */
//    if (input_frame) {
//        input_frame->pts = av_rescale_q(*pts, (AVRational){1, 48000}, output_codec_context->time_base);
//        *pts += input_frame->nb_samples;
//    }
    /* Send the audio frame stored in the temporary packet to the encoder.
     * The output audio stream encoder is used to do this. */
    error = avcodec_send_frame(output_codec_context, input_frame);
    if (error < 0)
        return error;

    /* Receive one encoded frame from the encoder. */
    error = avcodec_receive_packet(output_codec_context, output_packet);
    /* If the encoder asks for more data to be able to provide an
     * encoded frame, return indicating that no data is present. */
    if (error == AVERROR(EAGAIN) || error == AVERROR_EOF) {
        error = 0;
    } else if (error == 0) {
        error = 1;
    } 

    return error;
}

int check_fifo_size(PKSTAudioEncode *ctx) {
    if (av_audio_fifo_size(ctx->fifo) >= ctx->out_codec_ctx->frame_size)
        return 1;
    return 0;
}

/**
 * Reads samples from a FIFO audio buffer, encodes them and stores
 * the result in an output packet. 
 *
 * @param  output_packet Packet to store the encoded audio.
 * @param  ctx Context of the audio decode and encode operation.
 * @return 0 on success, negative value on error.
 */
int pkst_load_encode_get_package(AVPacket *output_packet, PKSTAudioEncode *ctx) {
    int error;
    /* Temporary storage of the output samples of the frame written to the file. */
    AVFrame *output_frame;
    /* Use the maximum number of possible samples per frame.
     * If there is less than the maximum possible frame size in the FIFO
     * buffer use this number. Otherwise, use the maximum possible frame size. */
    const int frame_size = FFMIN(av_audio_fifo_size(ctx->fifo),
                                ctx->out_codec_ctx->frame_size);

    /* Initialize temporary storage for one output frame. */
    error = init_output_frame(&output_frame, ctx->out_codec_ctx, frame_size);
    if (error < 0)
        return error;

    /* Read as many samples from the FIFO buffer as required to fill the frame.
     * The samples are stored in the frame temporarily. */
    if (av_audio_fifo_read(ctx->fifo, (void **)output_frame->data, frame_size) < frame_size) {
        av_frame_free(&output_frame);
        return -1;
    }

    error = pkst_encode_audio_frame(output_frame, output_packet, &(ctx->pts), ctx->out_codec_ctx);
    av_frame_free(&output_frame);
    return error;
}


/**
 * @brief Reads from the input, decodes, converts, and stores in the FIFO buffer
 * @param input_packet The input packet to be decoded
 * @param ctx The PKSTAudioEncode context containing the codec contexts, FIFO buffer, and resampler context
 * @return 0 on success, a negative AVERROR on failure
 */
int pkst_decode_convert_and_store(AVPacket *input_packet, PKSTAudioEncode *ctx) {
    int nb_samples;
    AVFrame *input_frame = NULL;
    uint8_t **converted_input_samples = NULL;
    int error;

    /* Initialize temporary storage for one input frame. */
    if ((error = init_input_frame(&input_frame)) < 0 )
        return error;

    /* Decode the input packet into a frame */
    error = pkst_decode_package(input_packet, input_frame, ctx->in_codec_ctx);
    if (error > 0) {
        /* Initialize a temporary storage for the converted input samples. */

//        printf("BEF :%d\n", input_frame->best_effort_timestamp);
//        fflush(stdout);

        nb_samples = (ctx->out_codec_ctx->sample_rate == ctx->in_codec_ctx->sample_rate) ?
                      input_frame->nb_samples :
                      av_rescale_rnd(swr_get_delay(ctx->resample_ctx, ctx->in_codec_ctx->sample_rate) + input_frame->nb_samples, ctx->out_codec_ctx->sample_rate, ctx->in_codec_ctx->sample_rate, AV_ROUND_UP);

        error = init_converted_samples(&converted_input_samples, ctx->out_codec_ctx, nb_samples); //input_frame->nb_samples);
        if (error < 0) goto cleanup;

        /* Convert the input samples into the desired output sample format. 
           Samples then stored in converted_input_samples. */
        nb_samples = convert_samples((const uint8_t**)input_frame->extended_data, 
                                converted_input_samples,
                                input_frame->nb_samples, 
                                ctx->resample_ctx);
        if (nb_samples < 0) goto cleanup;

        /* Add the converted input samples to the FIFO buffer for later processing. */
        error = add_samples_to_fifo(ctx->fifo, converted_input_samples, nb_samples);//input_frame->nb_samples);
        if (error < 0) goto cleanup;
    } 

cleanup:
    if (converted_input_samples) {
        av_freep(&converted_input_samples[0]);
        av_freep(&converted_input_samples);
    }
    av_frame_free(&input_frame);

    return error;
}

/**
 * Flushes the audio decoder. This function sends a NULL packet to the decoder to
 * indicate the end of the stream, and then enters a loop where it continues to pull
 * frames from the decoder. The frames are converted to the desired sample format
 * and added to the FIFO buffer for later processing.
 *
 * @param   ctx     The audio decoding and encoding context containing the decoder's codec context,
 *                  the encoder's codec context, the resampler context, and the FIFO buffer.
 *
 * @return          Error code (0 if successful).
 */

int pkst_flush_audio_decoder(PKSTAudioEncode *ctx) {
    AVFrame *frame = NULL;
    uint8_t **converted_input_samples = NULL;
    int error = 0;
    /* Initialize temporary storage for one input frame. */

    if ((error = init_input_frame(&frame)) < 0 )
        return error;


    avcodec_send_packet(ctx->in_codec_ctx, NULL);
    while (avcodec_receive_frame(ctx->in_codec_ctx, frame) == 0) {

        /* Initialize a temporary storage for the converted input samples. */
        error = init_converted_samples(&converted_input_samples, ctx->out_codec_ctx, frame->nb_samples);
        if (error < 0) goto cleanup;
            

        /* Convert the input samples into the desired output sample format. 
           Samples then stored in converted_input_samples. */
        error = convert_samples((const uint8_t**)frame->extended_data, 
                                converted_input_samples,
                                frame->nb_samples, 
                                ctx->resample_ctx);

        if (error < 0) goto cleanup;
        /* Add the converted input samples to the FIFO buffer for later processing. */
        error = add_samples_to_fifo(ctx->fifo, converted_input_samples, frame->nb_samples);
        if (error < 0) goto cleanup;

        av_frame_unref(frame);
    }
cleanup:
    av_frame_free(&frame);
    return error;
}
/**
 * Flushes the audio FIFO buffer. This function enters a loop where it continues to pull
 * frames from the FIFO buffer, as long as the size of the FIFO buffer is greater than 0.
 * Each frame is sent to the encoder for encoding. A NULL frame is sent to the encoder
 * after the loop to flush any remaining packets in the encoder.
 *
 * @param   ctx     The audio decoding and encoding context containing the decoder's codec context,
 *                  the encoder's codec context, the resampler context, and the FIFO buffer.
 *
 * @return          Error code (0 if successful).
 */

int pkst_flush_audio_fifo(PKSTAudioEncode *ctx) {
    AVFrame *frame = NULL;
    int frame_size;
    int error = 0;
    int64_t duration;
    
    // Read from FIFO and encode
    while (av_audio_fifo_size(ctx->fifo) > 0) {
        frame_size = FFMIN(av_audio_fifo_size(ctx->fifo), ctx->out_codec_ctx->frame_size);
        // Initialize temporary storage for output frame
        error = init_output_frame(&frame, ctx->out_codec_ctx, frame_size);
        if (error < 0)
            return error;
        if (av_audio_fifo_read(ctx->fifo, (void **)frame->data, frame_size) < frame_size) {
            error = AVERROR(ENOMEM);
            goto cleanup;
        }

        frame->best_effort_timestamp = av_rescale_q(ctx->pts, ctx->out_codec_ctx->time_base, (AVRational){1, 48000});
        frame->pts = ctx->pts;
        duration = av_rescale(AV_TIME_BASE, frame->nb_samples, frame->sample_rate);
        duration = av_rescale_q(duration, av_get_time_base_q(), ctx->out_codec_ctx->time_base);
        ctx->pts += duration;

        // Send the audio frame stored in the temporary packet to the encoder
        error = avcodec_send_frame(ctx->out_codec_ctx, frame);
        if (error < 0)
            goto cleanup;
        av_frame_free(&frame);
    }
    
    avcodec_send_frame(ctx->out_codec_ctx, NULL);
cleanup:
    av_frame_free(&frame);
    return error;
}


/**
 * @brief Clean up and free all resources in the PKSTAudioEncode structure.
 * 
 * @param ctx A pointer to the PKSTAudioEncode structure to be cleaned up.
 * 
 * This function will free all the non-NULL resources in the structure.
 * It assumes that the structure was correctly initialized.
 * The function does not check if the structure itself is NULL.
 * After this function is called, all the pointers in the structure will be invalid.
 */
void pkst_cleanup_decoder_encoder(PKSTAudioEncode **ctx) {
    if (ctx && *ctx) {
        if((*ctx)->fifo)
            av_audio_fifo_free((*ctx)->fifo);
        if((*ctx)->resample_ctx)
            swr_free(&((*ctx)->resample_ctx));
        if((*ctx)->out_codec_ctx)
            avcodec_free_context(&((*ctx)->out_codec_ctx));
        if((*ctx)->in_codec_ctx)
            avcodec_free_context(&((*ctx)->in_codec_ctx));
        free(*ctx);
        *ctx = NULL;  
    }
}

/**
 * @brief Open and initialize an audio encoder and decoder using an input AVStream.
 * 
 * @param in_audio_stream The input AVStream used to configure the encoder and decoder.
 * @param ed_ctx A pointer to a pointer to the PKSTAudioEncode structure to be initialized.
 * 
 * @return int A return code. 0 indicates success. Any other value indicates an error.
 * 
 * This function will allocate and initialize a PKSTAudioEncode structure, open an audio decoder and an audio encoder,
 * initialize a resampler, and initialize an audio FIFO.
 * The function returns 0 on success. On failure, it will clean up any resources that were allocated during its execution and return a non-zero error code.
 * If the function returns a non-zero error code, *ed_ctx will be NULL.
 * If the function returns 0, *ed_ctx will be a pointer to the initialized PKSTAudioEncode structure.
 * The caller is responsible for eventually freeing the PKSTAudioEncode structure using pkst_cleanup_encoder_decoder().
 */
int pkst_open_audio_decoder_encoder(const AVStream *in_audio_stream, const PKSTAudioConfig *config, PKSTAudioEncode **ed_ctx) {
    PKSTAudioEncode *ctx = calloc(1,sizeof(PKSTAudioEncode));
    int error;
    
    if (!ctx)
        return AVERROR(ENOMEM);

    *ed_ctx = NULL;

    error = pkst_open_audio_decoder(in_audio_stream, &(ctx->in_codec_ctx));
    if (error < 0) goto cleanup;
        
    error = pkst_open_audio_encoder(&(ctx->out_codec_ctx), config);
    if (error < 0) goto cleanup;


    error = pkst_init_resampler(ctx->in_codec_ctx, ctx->out_codec_ctx, &(ctx->resample_ctx));
    if (error < 0) goto cleanup;

    error = pkst_init_audio_fifo(&(ctx->fifo), ctx->out_codec_ctx);
    if (error < 0) goto cleanup;

    ctx->pts = 0;

    *ed_ctx = ctx;
    return 0;
cleanup:
    pkst_cleanup_decoder_encoder(&ctx);
    return error;
}



