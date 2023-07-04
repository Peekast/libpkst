// _CE_MEDIAINFO_H is an include guard to prevent double inclusion of this header file.
#ifndef _PKST_MEDIAINFO_H
#define _PKST_MEDIAINFO_H 1

// Includes the header file for the libavformat library.
#include <libavformat/avformat.h>
#include "keyvalue.h"
#include "pkst_strings.h"

// Defines a struct for storing media info.
typedef struct {
    char   *format;                // Format of the media.
    double duration;               // Duration of the media.
    char   *video_codec;           // Video codec used in the media.
    char   *audio_codec;           // Audio codec used in the media.
    int    video_index;
    int    audio_index;
    int    width;                  // Width of the video in the media.
    int    height;                 // Height of the video in the media.
    int    video_bitrate_kbps;     // Bitrate of the video in the media in kbps.
    int    audio_bitrate_kbps;     // Bitrate of the audio in the media in kbps.
    double fps;                    // Frame rate of the video in the media.
    int    audio_channels;         // Number of audio channels in the media.
    int    sample_rate;            // Sample rate of the audio in the media.
} PKSTMediaInfo;


// Declaration of a function that extracts media info from a file. 
extern int pkst_extract_mediainfo_from_file(const char *filename, PKSTMediaInfo **mi);

// Declaration of a function that extracts media info from a memory buffer.
extern double pkst_extract_duration_from_buffer(char *buffer, size_t buf_len);

extern double pkst_extract_duration_from_AVFormatContext(AVFormatContext *pFormatContext);

extern int pkst_extract_mediainfo_from_AVFormatContext(AVFormatContext *pFormatContext, PKSTMediaInfo **mi);

// Declaration of a function that frees a CEMediaInfo internal alloc memory.
extern void pkst_free_mediainfo(PKSTMediaInfo **mi);

extern void pkst_get_error(int err, char **error);

extern void pkst_free_error(char **error);

extern void pkst_dump_mediainfo(PKSTMediaInfo *info);


extern char *pkst_mediainfo_to_json(const PKSTMediaInfo *info);
// End of the include guard _CE_MEDIAINFO_H.
#endif


