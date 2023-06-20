package main

import (
	"fmt"
	pkst "libpkstenc"
)

func main() {

	encoder, err := pkst.AllocEncoderConfig("tcp://192.168.0.210:2000", true, 50, "")
	if err != nil {
		fmt.Println(err)
		return
	}

	err = encoder.AddAudioEncoderConfig("aac", 128000, 2, 44100)
	if err != nil {
		fmt.Println(err)
		return
	}

	err = encoder.AddOutputConfig("rtmp://a.rtmp.youtube.com/live2/y2uc-axv6-erww-rus9-agfq", "flv", "flvflags=no_duration_filesize", false)
	if err != nil {
		fmt.Println(err)
		return
	}

	encoder.AddOutputConfig("video.m3u8", "hls", "hls_segment_type=mpegts&hls_playlist_type=event&hls_segment_filename=tcp://127.0.1.1:2000/video_%02d.ts", true)
	encoder.DumpEncoderConfig()
	err = encoder.Start()
	if err == nil {
		for {
			data, ok := <-encoder.Chan
			if ok {
				switch v := data.(type) {
				case *int:
					fmt.Println("Encoder return code: ", *v)
				case *pkst.MediaInfo:
					fmt.Println("Input Data: ", v)
				case *pkst.Stats:
					fmt.Println("End: ", v)
				}
			} else {
				break
			}
		}
	}

	/*
		file, _ := os.Open("video_03.ts")
		buf, _ := io.ReadAll(file)
		duration := pkst.ExtractDurationFromBuffer(buf)
		fmt.Println(duration)
	*/
}
