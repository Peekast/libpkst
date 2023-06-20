package pkst

/*
#cgo CFLAGS: -I./lib
#cgo LDFLAGS: -lpkst

#include "pkst_mediainfo.h"

*/
import "C"

import (
	"encoding/json"
	"unsafe"
)

type MediaInfo struct {
	Format           string  `json:"format"`
	Duration         float64 `json:"duration"`
	VideoCodec       string  `json:"video_codec"`
	AudioCodec       string  `json:"audio_codec"`
	VideoIndex       int     `json:"video_index"`
	AudioIndex       int     `json:"audio_index"`
	Width            int     `json:"width"`
	Height           int     `json:"height"`
	VideoBitrateKbps int     `json:"video_bitrate_kbps"`
	AudioBitrateKbps int     `json:"audio_bitrate_kbps"`
	FPS              float64 `json:"fps"`
	AudioChannels    int     `json:"audio_channels"`
	SampleRate       int     `json:"sample_rate"`
}

func ParseMediaInfo(data []byte) (*MediaInfo, error) {
	var info MediaInfo
	err := json.Unmarshal(data, &info)
	if err != nil {
		return nil, err
	}
	return &info, nil
}

func ExtractDurationFromBuffer(buffer []byte) float64 {
	// Get an unsafe pointer to the start of the byte slice.
	cBuf := (*C.char)(unsafe.Pointer(&buffer[0]))

	// Call the C function.
	cDuration := C.pkst_extract_duration_from_buffer(cBuf, C.size_t(len(buffer)))

	// Convert the C double to a Go float64 and return it.
	return float64(cDuration)
}
