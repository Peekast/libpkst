#ifndef _PKST_IOCONTEXT_H
#define _PKST_IOCONTEXT_H 1

#define HANDLER_NAME "Media file produced by Peekast Media LLC (2023)."

#include <unistd.h>
#include <time.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include "pkst_mediainfo.h"
#include "pkst_defines.h"
#include "pkst_audio.h"

#define BOOL_TRUE  1
#define BOOL_FALSE 0

#define MAX_OUTPUTS 10

#define PKST_VIDEO_STREAM_OUTPUT 0
#define PKST_AUDIO_STREAM_OUTPUT 1

typedef void PKSTVideoEncode;
typedef void PKSTVideoConfig;

/* 
 * Struct representing the configuration of the output. This includes
 * information about the destination, type of destination, key-value options, 
 * and a flag indicating whether to ignore failures or not.
 */
typedef struct {
    char *dst;                // A string indicating the destination of the data.
    char *dst_type;           // Type of the output (for instance, format or protocol type).
    char *kv_opts;            // Key-Value options to apply on output.
    int  onfail_ignore;       // Boolean flag indicating whether to ignore failures (1 = ignore, 0 = don't ignore).
} PKSTOutputConfig;
//PKOutputConfig

/* 
 * Struct representing the configuration of the encoder. This includes
 * information about the source of the data, type of source, destination
 * configuration, and the number of outputs.
 */
typedef struct {
    char *in;                 // A string indicating the source of the data to be remultiplexed.
    char *in_type;            // Type of the input (for instance, format or protocol type).
    int  listen;              // If in the source need to listen on a socket or protocol.
    int  timeout;             // Network timeout.
    int  force_audio_encoding;
    int  force_video_encoding;
    char *tcpstats;           // Socket to send the destination report. This value could be NULL (e.g. tcp://<ip>:<port>).
    int  outs_len;            // Number of outputs configured.
    PKSTAudioConfig  *audio_config;
    PKSTVideoConfig  *video_config;
    PKSTOutputConfig *outs[MAX_OUTPUTS];   // An array of output configurations.
} PKSTEncoderConfig;
//PKstEncoderConfig
/* 
 * Struct representing the input context used in processing. 
 * It contains information about the format context of the input 
 * and an array for stream indices.
 */
typedef struct {
    AVFormatContext *in_ctx; // Format context for the input media. This structure contains information about the media.
    PKSTAudioEncode *a_enc_ctx;
    PKSTVideoEncode *v_enc_ctx;
    int streams[2];          // Array of stream indices. Typically, one for audio and one for video.
} PKSTInputCtx;
//PKstInputContext
typedef struct {
    int input_pkts;
    int output_pkts;
    time_t start_time;
    time_t current_time;
} PKSTStats;

typedef struct {
    int64_t start_pts;
    int64_t start_dts;
    int first_packet;
} PKSTts;
//PKstStartTimecode
/* 
 * Struct representing the output context used in processing. 
 * It contains information about the format context of the output, 
 * the status of the output process and a flag indicating whether to ignore 
 * failures during the output process or not.
 */
typedef struct {
    AVFormatContext *out_ctx; // Format context for the output media. This structure contains information about the output format.
    char *dst;                // Destination name
    int status;               // Status of the output process. The specific values it can take might depend on your program.  
    int onfail_ignore;        // Boolean flag indicating whether to ignore failures (1 = ignore, 0 = don't ignore).
} PKSTOutProcCtx;
//PKstOutputContext

typedef struct {
    int ctx_len;
    PKSTStats *stats;
    PKSTOutProcCtx *ctx[MAX_OUTPUTS];
} PKSTMultiOutCtx;
//PKstMultipleOutputContext

/* 
 * Function to allocate and initialize a new PKSTEncoderConfig structure. 
 * The function takes as input the address of a pointer to a PKSTEncoderConfig 
 * structure, along with a set of character strings representing the input,
 * the input type, and tcp stats. 
 * 
 * The function returns 0 if it succeeds and -1 if an error occurs.
 */
extern int pkst_alloc_encoder_config(PKSTEncoderConfig **enc, char *in, int listen, int timeout, char *in_type, char *tcpstats);


/* 
 * This function deallocates a PKSTEncoderConfig structure. 
 * It receives a pointer to a PKSTEncoderConfig and frees all its members and itself.
 * This includes freeing each output configuration within the encoder config.
 * Note that after calling this function, the pointer is no longer valid and should not be used.
 */
extern void pkst_free_encoder_config(PKSTEncoderConfig **enc);

/* 
 * This function adds a new output configuration to an existing PKSTEncoderConfig. 
 * It takes as arguments a pointer to a PKSTEncoderConfig and a pointer to a PKSTOutputConfig.
 *
 * The function returns 0 if it succeeds and -1 if an error occurs.
 */
extern int pkst_add_output_encoder_config(PKSTEncoderConfig *enc, PKSTOutputConfig *out);


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
extern int pkst_add_audio_encoder_config(PKSTEncoderConfig *enc, PKSTAudioConfig *config, int force_encoding);


/* 
 * This function prints the details of a PKSTEncoderConfig structure. 
 * It receives a pointer to a PKSTEncoderConfig and prints each of its elements to stdout.
 */
extern void pkst_dump_encoder_config(PKSTEncoderConfig *enc);


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

extern int pkst_open_input_context(PKSTEncoderConfig *config, PKSTInputCtx **ctx);

/* 
 * This function closes an input context and deallocates its memory. 
 * It takes a double pointer to a PKSTInputCtx. 
 * If the pointer or the context itself is NULL, the function returns without doing anything. 
 * Otherwise, it closes the AVFormatContext contained in the PKSTInputCtx (if it's not NULL) using the avformat_close_input function. 
 * It then frees the PKSTInputCtx and sets the pointer to NULL. 
 * After this function is called, the pointer that was passed to it should not be used.
 */
extern void pkst_close_input_context(PKSTInputCtx **ctx);


/**
 * @brief Open multiple output contexts for remuxing based on the provided configuration and input context.
 * @param config The encoder configuration, which contains information about the outputs.
 * @param pkst_in_ctx The input context, which contains information about the input streams.
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
extern int pkst_open_multiple_ouputs_context(PKSTEncoderConfig *config, PKSTInputCtx *in_ctx, PKSTMultiOutCtx **out_ctx);

/**
 * @brief Clean up and close a PKSTOutProcCtx.
 * @param out_ctx A pointer to the output context to be cleaned up and closed.
 * 
 * This function frees up the resources associated with a PKSTOutProcCtx. It closes 
 * the associated AVFormatContext's pb if necessary, frees the AVFormatContext, and 
 * finally frees the PKSTOutProcCtx itself. It also sets the pointer to the PKSTOutProcCtx to NULL.
 */
extern void pkst_close_outputs_contexts(PKSTMultiOutCtx **out_ctx);


/**
 * @brief Write trailers to multiple output contexts.
 * 
 * This function writes the trailer to each output context that has a status of 0 (indicating success).
 * If an error occurs while writing the trailer, the status of the context is updated to the error code.
 * 
 * @param outputs The array of output contexts.
 * @param out_len The length of the array of output contexts.
 */
extern void pkst_write_trailers_multiple_contexts(PKSTOutProcCtx *outputs[], size_t out_len);

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
extern int pkst_audiovideo_process(PKSTInputCtx *in, PKSTMultiOutCtx *out, int socket);
extern AVStream *pkst_get_audio_stream(PKSTInputCtx *ctx);

extern int pkst_process_av_packet(PKSTts *ts, AVPacket *pkt, PKSTInputCtx *in, PKSTMultiOutCtx *out, int *output_fail);
extern void dump_multi_out_ctx(const PKSTMultiOutCtx *multi_out_ctx);
extern char *dump_multi_out_ctx_json(const PKSTMultiOutCtx *multi_out_ctx, int error);

extern int pkst_flush_audio_encoder_queue(PKSTInputCtx *in, PKSTMultiOutCtx *out);
#endif