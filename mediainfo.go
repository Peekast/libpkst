package pkstenc

/*
#cgo CFLAGS: -I.
#cgo LDFLAGS: -lpkstenc
#include "lib/pkst_mediainfo.h"

*/
import "C"
import (
	"fmt"
)

type MediaInfo struct {
	Format           string
	Duration         float64
	VideoCodec       string
	AudioCodec       string
	Width            int
	Height           int
	VideoBitrateKbps int
	AudioBitrateKbps int
	FPS              float64
	AudioChannels    int
	SampleRate       int
}

func convertToGoMediaInfo(cInfo *C.PKSTMediaInfo) MediaInfo {
	return MediaInfo{
		Format:           C.GoString(cInfo.format),
		Duration:         float64(cInfo.duration),
		VideoCodec:       C.GoString(cInfo.video_codec),
		AudioCodec:       C.GoString(cInfo.audio_codec),
		Width:            int(cInfo.width),
		Height:           int(cInfo.height),
		VideoBitrateKbps: int(cInfo.video_bitrate_kbps),
		AudioBitrateKbps: int(cInfo.audio_bitrate_kbps),
		FPS:              float64(cInfo.fps),
		AudioChannels:    int(cInfo.audio_channels),
		SampleRate:       int(cInfo.sample_rate),
	}
}

func GetMediaInfoFromFile(filename string) (*MediaInfo, error) {
	var mi MediaInfo
	format := C.PKSTMediaInfo{}

	retval := C.pkst_extract_mediainfo_from_file(C.CString(filename), &format)
	if retval == 0 {
		defer C.pkst_free_mediainfo(&format)
		mi = convertToGoMediaInfo(&format)
		return &mi, nil
	}
	var cerr *C.char
	C.pkst_get_error(retval, &cerr)
	err := fmt.Errorf(C.GoString(cerr))
	C.pkst_free_error(&cerr)
	return nil, err
}
