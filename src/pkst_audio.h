#ifndef _PKST_AUDIO_H
#define _PKST_AUDIO_H 1

#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>


typedef struct {
    AVCodecContext *in_codec_ctx;
    AVCodecContext *out_codec_ctx;
    AVAudioFifo    *fifo;
    SwrContext     *resample_ctx;
    int64_t        pts;
} PKSTAudioEncode;

typedef struct {
    char *codec;
    int bitrate_bps;
    int channels;
    int sample_rate;
} PKSTAudioConfig;


extern void pkst_cleanup_decoder_encoder(PKSTAudioEncode **ctx);

extern int pkst_open_audio_decoder_encoder(const AVStream *in_audio_stream, const PKSTAudioConfig *config, PKSTAudioEncode **ed_ctx);

extern int check_fifo_size(PKSTAudioEncode *ctx);

extern int pkst_load_encode_get_package(AVPacket *output_packet, PKSTAudioEncode *ctx);

extern int pkst_decode_convert_and_store(AVPacket *input_packet, PKSTAudioEncode *ctx);

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

extern int pkst_flush_audio_decoder(PKSTAudioEncode *ctx);



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
extern int pkst_flush_audio_fifo(PKSTAudioEncode *ctx);

#endif 