package main

import (
	"fmt"
	pkstenc "libpkstenc"
)

func main() {
	mi, err := pkstenc.GetMediaInfoFromFile("teaser.mp4")
	fmt.Println(mi.Duration, mi.VideoCodec, err)
}
