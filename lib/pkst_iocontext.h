#ifndef _PKST_IOCONTEXT_H
#define _PKST_IOCONTEXT_H 1

#define HANDLER_NAME "Media file produced by Peekast Media LLC (2023)."

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include "pkst_mediainfo.h"
#include "pkst_defines.h"
#include "pkst_audio.h"

#define BOOL_TRUE  1
#define BOOL_FALSE 0

#define MAX_OUTPUTS 4

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
    char *tcpstats;           // Socket to send the destination report. This value could be NULL (e.g. tcp://<ip>:<port>).
    int  outs_len;            // Number of outputs configured.
    char *kv_audio_req;          // Audio Requirements
    char *kv_video_req;          // Video Requirements
    PKSTAudioConfig  *audio_config;
    PKSTVideoConfig  *video_config;
    PKSTOutputConfig *outs[MAX_OUTPUTS];   // An array of output configurations.
} PKSTEncoderConfig;

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


typedef struct {
    int ctx_len;
    PKSTOutProcCtx *ctx[MAX_OUTPUTS];
} PKSTMultiOutCtx;

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
 * This function adds a new output configuration to an existing PKSTEncoderConfig. 
 * It takes as arguments a pointer to a PKSTEncoderConfig and a pointer to a PKSTOutputConfig.
 *
 * The function returns 0 if it succeeds and -1 if an error occurs.
 */
extern int pkst_add_output_encoder_config(PKSTEncoderConfig *enc, PKSTOutputConfig *out);


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
extern int pkst_add_audio_encoder_config(PKSTEncoderConfig *enc, PKSTAudioConfig *config);


/* 
 * This function prints the details of a PKSTEncoderConfig structure. 
 * It receives a pointer to a PKSTEncoderConfig and prints each of its elements to stdout.
 */
extern void pkst_dump_encoder_config(PKSTEncoderConfig *enc);



/* 
 * This function deallocates a PKSTOutputConfig structure. 
 * It receives a pointer to a PKSTOutputConfig and frees all its members and itself.
 * Note that after calling this function, the pointer is no longer valid and should not be used.
 */
extern void pkst_free_output_config(PKSTOutputConfig *out);


/* 
 * This function deallocates a PKSTEncoderConfig structure. 
 * It receives a pointer to a PKSTEncoderConfig and frees all its members and itself.
 * This includes freeing each output configuration within the encoder config.
 * Note that after calling this function, the pointer is no longer valid and should not be used.
 */
extern void pkst_free_encoder_config(PKSTEncoderConfig **enc);


/* 
 * This function closes an input context and deallocates its memory. 
 * It takes a double pointer to a PKSTInputCtx. 
 * If the pointer or the context itself is NULL, the function returns without doing anything. 
 * Otherwise, it closes the AVFormatContext contained in the PKSTInputCtx (if it's not NULL) using the avformat_close_input function. 
 * It then frees the PKSTInputCtx and sets the pointer to NULL. 
 * After this function is called, the pointer that was passed to it should not be used.
 */
extern void pkst_close_input_context(PKSTInputCtx **ctx);

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
 */
extern AVFormatContext *pkst_open_output_context_for_remux(PKSTOutputConfig *config, AVStream *in_avstream[2]);

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
extern AVFormatContext *pkst_open_output_context(PKSTOutputConfig *config, AVStream *remux[2], AVCodecContext *enc[2]);

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
extern int pkst_send_AVPacket_multiple_contexts(AVPacket *packet, AVRational in_timebase, int stream_type, PKSTOutProcCtx *outputs[], size_t out_len);

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
 */
extern int pkst_audiovideo_remux(PKSTInputCtx *in, PKSTMultiOutCtx *out);

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


extern char *pkst_out_report_to_json(const PKSTMultiOutCtx *report, int status);
#endif