package pkst

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io"
)

type MgsProto struct {
	ProtocolID uint8
	MsgType    uint8
	MsgSize    uint16
}

const (
	MESSAGE_REPORT_START = 0x50 // Send Mediainfo
	MESSAGE_REPORT_STATS = 0x20 // Send speed each 10 seconds
	MESSAGE_REPORT_END   = 0x40 // Out report
)

type OutProcCtx struct {
	Destination string `json:"destination"`
	Status      int    `json:"status"`
	IgnoreFail  bool   `json:"ignore_fail"`
	Error       string `json:"error"`
}

type Stats struct {
	InputPackets  int     `json:"input_packets"`
	InputSpeed    float64 `json:"input_Speed"`
	OutputPackets int     `json:"output_packets"`
	OutputSpeed   float64 `json:"output_Speed"`
	Eta           float64 `json:"eta"`
}

type OutputStats struct {
	Timestamp int          `json:"timestamp"`
	Outputs   []OutProcCtx `json:"outputs"`
	Stats     Stats        `json:"stats"`
}

func readMsgProtoHeader(r io.Reader) (*MgsProto, error) {
	var header MgsProto

	err := binary.Read(r, binary.LittleEndian, &header.ProtocolID)
	if err != nil {
		return nil, err
	}

	err = binary.Read(r, binary.LittleEndian, &header.MsgType)
	if err != nil {
		return nil, err
	}

	err = binary.Read(r, binary.LittleEndian, &header.MsgSize)
	if err != nil {
		return nil, err
	}

	return &header, nil
}

func ReadMessage(r io.Reader) (int, []byte, error) {
	hdr, err := readMsgProtoHeader(r)
	if err != nil {
		return 0, nil, err
	}
	if hdr.ProtocolID != 0xAA {
		return 0, nil, fmt.Errorf("invalid protocol header %d", hdr.ProtocolID)
	}

	buf := make([]byte, hdr.MsgSize)
	n, err := r.Read(buf)

	if n != int(hdr.MsgSize) || err != nil {
		return 0, nil, fmt.Errorf("io.Read has not read all the required data, error: %v", err)
	}
	return int(hdr.MsgType), buf, nil
}

func ParseOutputStats(data []byte) (*OutputStats, error) {
	var ctx OutputStats

	err := json.Unmarshal(data, &ctx)
	if err != nil {
		fmt.Println("Error parsing JSON: ", err)
		return &ctx, err
	}

	return &ctx, nil
}
