#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "pkst_iocontext.h"
#include "pkst_mediainfo.h"
#include "pkst_audio.h"
#include "pkst_routine.h"

#include <unistd.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[]) {
    int i;
    PKSTEncoderConfig  *config;
    PKSTOutputConfig   out;
    PKSTAudioConfig    audio_config;
    PKSTRoutine *routine;
    

    audio_config.sample_rate = 44100;
    audio_config.bitrate_bps = 128000;
    audio_config.codec = "aac";
    audio_config.channels = 2;

    i = pkst_alloc_encoder_config(&config, "tcp://192.168.0.210:2000", 1,30 , NULL, NULL);
    if (i < 0) { fprintf(stderr, "pkst_open_audio_encoder_decoder (error '%d')\n", i); return -1; }


    pkst_add_audio_encoder_config(config, &audio_config);
/*
    out.dst = "flv.mp4";
    out.dst_type = "mp4"; //NULL;
    out.kv_opts = "movflags=frag_keyframe+empty_moov+default_base_moof";
    out.onfail_ignore = BOOL_TRUE;

    ret = pkst_add_output_encoder_config(config, &out);

    if (ret < 0) {
        fprintf(stderr, "pkst_add_output_encoder_config (error '%s')\n", av_err2str(ret));
        return -1;
    }
*/
    out.dst = "video_1.m3u8";
    out.dst_type = "hls";
    out.kv_opts = "hls_segment_type=mpegts&hls_playlist_type=event&hls_segment_filename=tcp://127.0.0.1:2001/video_%02d.ts";
    out.onfail_ignore = BOOL_TRUE;

    pkst_add_output_encoder_config(config, &out);


    out.dst = "rtmp://a.rtmp.youtube.com/live2/y2uc-axv6-erww-rus9-agfq";
    out.dst_type = "flv";
    out.kv_opts = "flvflags=no_duration_filesize";
    out.onfail_ignore = BOOL_FALSE;

    pkst_add_output_encoder_config(config, &out);
/*
    out.dst = "rtmp://rtmp-auto.millicast.com:1935/v2/pub/lasgexuv?token=61d5c534af5cc9c30432ae416df81cd61dc0a5c530b31146995f45111158d363";
    out.dst_type = "flv";
    out.kv_opts = "flvflags=no_duration_filesize";
    out.onfail_ignore = BOOL_TRUE;

    pkst_add_output_encoder_config(config, &out);
*/
    pkst_dump_encoder_config(config);

    pkst_start_encoder_routine(config, &routine);

    //i = pkst_wait_routine(routine);

    sleep(30);

    pkst_cancel_routine(routine);
    fprintf(stderr, "Hilo (error '%d')\n", i);

////////////////////////// A partir de este punto empiezan a aplicar las configuraciones //////////////////////////

}
