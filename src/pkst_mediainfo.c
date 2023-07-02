#include <stdio.h>
#include <jansson.h>

#include "pkst_mediainfo.h"
#include "pkst_defines.h"
#include "pkst_iobuffer.h"
#include "pkst_log.h"
#include "keyvalue.h"

double pkst_estimate_duration_from_AVFormatContext(AVFormatContext *pFormatContext) {
    int64_t start_pts = AV_NOPTS_VALUE;
    int64_t end_pts = AV_NOPTS_VALUE;
    int64_t end_pts_duration = AV_NOPTS_VALUE;
    AVPacket pkt;
    // Busca el primer stream de video.
    int video_stream_index = -1;

    for (int i = 0; i < pFormatContext->nb_streams; ++i) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        // No se encontró un stream de video.
        return AV_NOPTS_VALUE;
    }
    
    AVStream* video_stream = pFormatContext->streams[video_stream_index];
    while (av_read_frame(pFormatContext, &pkt) >= 0) {
        if (pkt.stream_index == video_stream_index) {
            if (start_pts == AV_NOPTS_VALUE) {
                start_pts = pkt.pts;
            }
            end_pts = pkt.pts;
            end_pts_duration = pkt.duration;
        }
        av_packet_unref(&pkt);
    }

    if (start_pts == AV_NOPTS_VALUE || end_pts == AV_NOPTS_VALUE) {
        // No se pudo estimar la duración
        return AV_NOPTS_VALUE;
    }

    // Calcula la duración en la base de tiempo del stream de video.
    int64_t duration_pts = end_pts - start_pts + end_pts_duration;

    // Convierte la duración a segundos.
    double duration_seconds = duration_pts * av_q2d(video_stream->time_base);

    return duration_seconds;
}


double pkst_extract_duration_from_AVFormatContext(AVFormatContext *pFormatContext) {
    return pFormatContext->duration != AV_NOPTS_VALUE ? 
           pFormatContext->duration / AV_TIME_BASE : 
           pkst_estimate_duration_from_AVFormatContext(pFormatContext);
}



int pkst_extract_mediainfo_from_AVFormatContext(AVFormatContext *pFormatContext, PKSTMediaInfo **mi) {
    AVStream *stream;
    int video_stream_index = -1;
    int audio_stream_index = -1;

    int i;

    if (!mi || !(*mi = pkst_alloc(sizeof(PKSTMediaInfo))))  {
        return -1;
    }

    (*mi)->format = pkst_alloc(strlen(pFormatContext->iformat->name)+1);
    if ((*mi)->format == NULL) {
        return -1;
    }

    strcpy((*mi)->format, pFormatContext->iformat->name);

    if (pFormatContext->duration != AV_NOPTS_VALUE)
        (*mi)->duration = pFormatContext->duration / AV_TIME_BASE;
    else 
        (*mi)->duration = -1;

    for (i = 0; i < pFormatContext->nb_streams; i++) {
        stream = pFormatContext->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            video_stream_index = i;
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            audio_stream_index = i;
        
        if (video_stream_index != -1 && audio_stream_index != -1)
            break;
    }

    (*mi)->audio_index = audio_stream_index;
    (*mi)->video_index = video_stream_index;

    if (video_stream_index != -1) {
        stream = pFormatContext->streams[video_stream_index];
        (*mi)->width = stream->codecpar->width;

        (*mi)->height = stream->codecpar->height;

        (*mi)->fps = av_q2d(stream->avg_frame_rate);
        (*mi)->video_bitrate_kbps = stream->codecpar->bit_rate / 1000;

        (*mi)->video_codec = pkst_alloc(strlen(avcodec_get_name(stream->codecpar->codec_id)));
        if ((*mi)->video_codec != NULL) {
            strcpy((*mi)->video_codec, avcodec_get_name(stream->codecpar->codec_id));
        }
    }

    if (audio_stream_index != -1) {
        stream = pFormatContext->streams[audio_stream_index];
        (*mi)->audio_bitrate_kbps = stream->codecpar->bit_rate / 1000;
        (*mi)->sample_rate = stream->codecpar->sample_rate;
        (*mi)->audio_channels = stream->codecpar->ch_layout.nb_channels;
        (*mi)->audio_codec = pkst_alloc(strlen(avcodec_get_name(stream->codecpar->codec_id)));
        if ((*mi)->audio_codec != NULL) {
            strcpy((*mi)->audio_codec, avcodec_get_name(stream->codecpar->codec_id));
        }
    }
    return 0;
}


int pkst_extract_mediainfo_from_file(const char *filename, PKSTMediaInfo **mi) {
    AVFormatContext *pFormatContext;
    int ret;

    pFormatContext = avformat_alloc_context();
    if (pFormatContext == NULL) {
        return -1;
    }

    if ((ret = avformat_open_input(&pFormatContext, filename, NULL, NULL)) != 0) {
        return ret; 
    }

    if ((ret = avformat_find_stream_info(pFormatContext, NULL)) < 0) {
        return ret;
    }

    ret = pkst_extract_mediainfo_from_AVFormatContext(pFormatContext,mi);

    avformat_close_input(&pFormatContext);
    avformat_free_context(pFormatContext);
    return ret;
}


double pkst_extract_duration_from_buffer(char *buffer, size_t buf_len) {
    PKSTIOBuffer iobuf;

    AVFormatContext *pFormatContext = NULL;
    AVIOContext *avioContext = NULL;
    unsigned char *avioBuffer = NULL;
    double ret = 0;

    iobuf.buffer = (uint8_t *)buffer;
    iobuf.buf_len = buf_len;
    iobuf.pos = 0;

    // Create an AVIOContext that will read from memory
    avioBuffer = av_calloc(1,buf_len);
    if (!avioBuffer) {
        ret = -1;
        goto cleanup;
    }
    
    avioContext = avio_alloc_context(avioBuffer, buf_len, 0, &iobuf, ioread_buffer, NULL, NULL);
    if (!avioContext) {
        ret = -1;
        goto cleanup;
    }

    // Create an AVFormatContext
    pFormatContext = avformat_alloc_context();
    if (!pFormatContext) {
        ret = -1;
        goto cleanup;
    }
    pFormatContext->pb = avioContext;

    // Open the input stream using the AVIOContext
    if (avformat_open_input(&pFormatContext, NULL, NULL, NULL) != 0) {
                       
        ret = -1;
        goto cleanup;
    }

    // Find the stream info
    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        ret = -1;
        goto cleanup;
    }

    // Extract media info
    ret = pkst_extract_duration_from_AVFormatContext(pFormatContext);

cleanup:
    // Close the input stream and free the contexts
    if (pFormatContext != NULL) {
        avformat_close_input(&pFormatContext);
        avformat_free_context(pFormatContext);
    }
    return ret;
}

void pkst_free_mediainfo(PKSTMediaInfo **mi) {
    if (mi && *mi) {
        if ((*mi)->format != NULL) 
            free((*mi)->format);

        if ((*mi)->video_codec != NULL) 
            free((*mi)->video_codec);

        if ((*mi)->audio_codec != NULL)
            free((*mi)->audio_codec);
        
        free(*mi);
        *mi = NULL;
    }
}


void pkst_get_error(int err, char **error) {
    *error = pkst_alloc(AV_ERROR_MAX_STRING_SIZE);
    if (*error) {
        av_strerror(err, *error, AV_ERROR_MAX_STRING_SIZE);
    }
}

void pkst_free_error(char **error) {
    if (*error != NULL) 
        free(*error);
}


void pkst_dump_mediainfo(PKSTMediaInfo *info) {
    if (info == NULL) {
        pkst_log(NULL,0,"Invalid PKSTMediaInfo pointer\n");
        return;
    }

    pkst_log(NULL,0,"Media Info:\n");
    pkst_log(NULL,0,"Format: %s, Duration: %.2f\n", info->format, info->duration);
    pkst_log(NULL,0,"Video Codec: %s, Index: %d, Width: %d, Height: %d, Bitrate: %d kbps, FPS: %.2f\n",
            info->video_codec, info->video_index, info->width, info->height, info->video_bitrate_kbps, info->fps);
    pkst_log(NULL,0,"Audio Codec: %s, Index: %d, Bitrate: %d kbps, Channels: %d, Sample Rate: %d\n",
            info->audio_codec, info->audio_index, info->audio_bitrate_kbps, info->audio_channels, info->sample_rate);
}


char *pkst_mediainfo_to_json(const PKSTMediaInfo *info) {
    json_t *jobj = json_object();

    // Asegúrate de comprobar que info no es NULL antes de acceder a sus miembros
    if (info) {
        json_object_set_new(jobj, "format", json_string(info->format));
        json_object_set_new(jobj, "duration", json_real(info->duration));
        json_object_set_new(jobj, "video_codec", json_string(info->video_codec));
        json_object_set_new(jobj, "audio_codec", json_string(info->audio_codec));
        json_object_set_new(jobj, "video_index", json_integer(info->video_index));
        json_object_set_new(jobj, "audio_index", json_integer(info->audio_index));
        json_object_set_new(jobj, "width", json_integer(info->width));
        json_object_set_new(jobj, "height", json_integer(info->height));
        json_object_set_new(jobj, "video_bitrate_kbps", json_integer(info->video_bitrate_kbps));
        json_object_set_new(jobj, "audio_bitrate_kbps", json_integer(info->audio_bitrate_kbps));
        json_object_set_new(jobj, "fps", json_real(info->fps));
        json_object_set_new(jobj, "audio_channels", json_integer(info->audio_channels));
        json_object_set_new(jobj, "sample_rate", json_integer(info->sample_rate));
    }
    
    char *result = json_dumps(jobj, JSON_ENCODE_ANY);

    // Libera la memoria del objeto JSON
    json_decref(jobj);

    return result;
}