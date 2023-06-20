package pkst

/*
#cgo CFLAGS: -I./lib
#cgo LDFLAGS: -lpkst

#include "pkst_iocontext.h"
#include "pkst_audio.h"
#include "pkst_routine.h"

*/
import "C"

import (
	"fmt"
	"net"
	"sync"
	"sync/atomic"
	"unsafe"
)

const (
	AVERROR_ETIMEDOUT = -60
)

// EncoderOutput represents an output configuration for an encoder.
// It defines where and how to output the encoded data.
type EncoderOutput struct {
	Dst        string // The destination of the output (e.g., a file path, a URL).
	Type       string // The output type (e.g., format of the output file).
	KVOpts     string // Additional key-value options specific to the output.
	IgnoreFail bool   // Whether to ignore failures when outputting to this destination.
}

// AudioConfig represents a configuration for audio encoding.
// It defines how the audio data should be encoded.
type AudioConfig struct {
	Codec      string // The audio codec to be used for encoding.
	BitRate    int    // The bitrate to be used for encoding.
	SampleRate int    // The sample rate to be used for encoding.
	Channels   int    // The number of audio channels.
}

// EncoderConfig represents a complete configuration for an encoder.
// It includes configurations for inputs, outputs, and encoding parameters.
type EncoderConfig struct {
	In      string            // The input source.
	Listen  bool              // Whether the encoder should listen for connections.
	Timeout int               // The timeout period for the encoder.
	InType  string            // The type of the input (e.g., file format).
	Audio   *AudioConfig      // The configuration for audio encoding.
	Outputs [4]*EncoderOutput // The configurations for outputs. A maximum of four outputs are supported.
}

// Encoder is a Go wrapper for the PKSTEncoderConfig struct.
// It provides methods for starting, cancelling, and waiting for encoding routines.
// It also provides a channel for receiving updates about the encoding process.
type Encoder struct {
	config     *C.PKSTEncoderConfig // The underlying PKSTEncoderConfig struct from the C library.
	routine    *C.PKSTRoutine       // The encoding routine.
	server     *net.TCPListener     // The TCP server for receiving connections.
	Chan       chan interface{}     // The channel for receiving updates about the encoding process.
	startOnce  sync.Once            // Ensures the start operation only happens once.
	cancelOnce sync.Once            // Ensures the cancel operation only happens once.
	started    int32                // Atomic flag to indicate whether the encoding has started.
}

// NewEncoder creates and configures a new Encoder instance.
//
// The function first allocates the underlying PKSTEncoderConfig structure, which holds
// configuration parameters for the encoder, using values from the provided EncoderConfig
// instance.
//
// If an audio configuration is provided, the function attempts to add it to the
// encoder configuration. If any error occurs during this process, it deallocates
// the PKSTEncoderConfig structure and returns the error.
//
// For each provided output configuration, the function attempts to add it to the
// encoder configuration. If any error occurs while adding an output configuration,
// it deallocates the PKSTEncoderConfig structure and returns the error.
//
// If all configurations are added successfully, the function returns the new Encoder
// instance. If any error occurs during the process, the function returns nil and the
// error.
func NewEncoder(config *EncoderConfig) (*Encoder, error) {
	encoder, err := allocEncoderConfig(config.In, config.Listen, config.Timeout, config.InType)
	if err != nil {
		return nil, err
	}

	if config.Audio != nil {
		err = encoder.addAudioEncoderConfig(config.Audio.Codec,
			config.Audio.BitRate,
			config.Audio.Channels,
			config.Audio.SampleRate)
		if err != nil {
			C.pkst_free_encoder_config(&(encoder.config))
			return nil, err
		}
	}
	for i := 0; i < 4; i++ {
		if config.Outputs[i] != nil {
			out := config.Outputs[i]
			err = encoder.addOutputConfig(out.Dst, out.Type, out.KVOpts, out.IgnoreFail)
			if err != nil {
				C.pkst_free_encoder_config(&(encoder.config))
				return nil, err
			}
		}
	}
	return encoder, nil
}

// allocEncoderConfig allocates and initializes an Encoder configuration. It uses the
// provided input parameters to initialize a PKSTEncoderConfig structure and setup
// a TCP listener.
//
// First, the function determines whether to listen for incoming connections based on
// the listen parameter. It then obtains an available loopback IPv4 address and creates
// a TCP listener at that address.
//
// It then converts the string parameters to C strings and calls the pkst_alloc_encoder_config
// function to create the PKSTEncoderConfig structure.
//
// If successful, the function returns a pointer to the Encoder configuration. If there
// is any error during the process, such as failure to create the TCP listener or allocate
// the encoder configuration, the function returns nil and the error.
//
// The function uses C library functions for allocating and converting the string
// parameters and for creating the PKSTEncoderConfig structure.
//
// Note: It's the caller's responsibility to deallocate the C.PKSTEncoderConfig structure
// when done using it. This can be achieved by calling C.pkst_free_encoder_config on
// the pointer stored in the Encoder.config field.
func allocEncoderConfig(in string, listen bool, timeout int, inType string) (*Encoder, error) {
	var config *C.PKSTEncoderConfig
	var int_listen int

	if listen {
		int_listen = 1
	} else {
		int_listen = 0
	}

	ip, _ := LoopbackIPv4()
	server, e := net.ListenTCP("tcp4", &net.TCPAddr{IP: ip})

	if e != nil {
		return nil, e
	}

	cIn := C.CString(in)
	defer C.free(unsafe.Pointer(cIn))

	cInType := C.CString(inType)
	defer C.free(unsafe.Pointer(cInType))

	cTcpstats := C.CString("tcp://" + server.Addr().String())
	defer C.free(unsafe.Pointer(cTcpstats))

	res := C.pkst_alloc_encoder_config(&config, cIn, C.int(int_listen), C.int(timeout), cInType, cTcpstats)
	if res != 0 {
		return nil, fmt.Errorf("failed to allocate encoder config error (%d)", res)
	}

	return &Encoder{
		config: config,
		server: server,
		Chan:   make(chan interface{}),
	}, nil
}

// addAudioEncoderConfig adds an audio encoder configuration to the provided Encoder.
// It creates a PKSTAudioConfig structure using the provided parameters and calls the
// pkst_add_audio_encoder_config function to add the audio encoder configuration to the
// Encoder's PKSTEncoderConfig structure.
//
// The function converts the string codec parameter to a C string and assigns the other
// parameters directly to the PKSTAudioConfig structure's fields. After creating the
// PKSTAudioConfig structure, it calls the pkst_add_audio_encoder_config function, passing
// the Encoder's PKSTEncoderConfig structure and the PKSTAudioConfig structure.
//
// If the pkst_add_audio_encoder_config function returns a non-zero value, the function
// returns an error indicating that adding the audio encoder configuration failed. Otherwise,
// it returns nil to indicate success.
//
// Note: The function frees the memory allocated for the C string after calling the
// pkst_add_audio_encoder_config function.
func (pe *Encoder) addAudioEncoderConfig(codec string, bitrateBps, channels, sampleRate int) error {
	var audioConfig C.PKSTAudioConfig

	audioConfig.codec = C.CString(codec)
	audioConfig.bitrate_bps = C.int(bitrateBps)
	audioConfig.channels = C.int(channels)
	audioConfig.sample_rate = C.int(sampleRate)

	defer C.free(unsafe.Pointer(audioConfig.codec))

	res := C.pkst_add_audio_encoder_config(pe.config, &audioConfig)
	if res != 0 {
		return fmt.Errorf("failed to add audio encoder config error (%d)", int(res))
	}

	return nil
}

// addOutputConfig adds an output encoder configuration to the provided Encoder.
// It creates a PKSTOutputConfig structure using the provided parameters and calls the
// pkst_add_output_encoder_config function to add the output encoder configuration to the
// Encoder's PKSTEncoderConfig structure.
//
// The function checks if the provided parameters are not empty. If they are not,
// it converts the string parameters to C strings and assigns them to the PKSTOutputConfig
// structure's fields. It also converts the onFailIgnore boolean to an integer and assigns it
// to the onfail_ignore field. If any of the string parameters are empty, it assigns nil to the
// corresponding field.
//
// After creating the PKSTOutputConfig structure, it calls the pkst_add_output_encoder_config
// function, passing the Encoder's PKSTEncoderConfig structure and the PKSTOutputConfig structure.
//
// If the pkst_add_output_encoder_config function returns a non-zero value, the function
// returns an error indicating that adding the output encoder configuration failed. Otherwise,
// it returns nil to indicate success.
//
// Note: The function frees the memory allocated for the C strings after calling the
// pkst_add_output_encoder_config function.
func (pe *Encoder) addOutputConfig(dst, dstType, kvOpts string, onFailIgnore bool) error {
	var outputConfig C.PKSTOutputConfig

	if dst != "" {
		outputConfig.dst = C.CString(dst)
		defer C.free(unsafe.Pointer(outputConfig.dst))
	} else {
		outputConfig.dst = nil
	}

	if dstType != "" {
		outputConfig.dst_type = C.CString(dstType)
		defer C.free(unsafe.Pointer(outputConfig.dst_type))
	} else {
		outputConfig.dst_type = nil
	}

	if kvOpts != "" {
		outputConfig.kv_opts = C.CString(kvOpts)
		defer C.free(unsafe.Pointer(outputConfig.kv_opts))
	} else {
		outputConfig.kv_opts = nil
	}

	if onFailIgnore {
		outputConfig.onfail_ignore = C.int(1)
	} else {
		outputConfig.onfail_ignore = C.int(0)
	}

	res := C.pkst_add_output_encoder_config(pe.config, &outputConfig)
	if res != 0 {
		return fmt.Errorf("failed to add output config %d", res)
	}

	return nil
}

// Start initializes the Encoder by starting the encoder routine in a new goroutine.
// This function should only be called once per Encoder. Subsequent calls will have no effect.
//
// It begins by using the pkst_start_encoder_routine C function to initialize the encoder routine.
// If the C function returns a non-zero value, it sets an error message and exits.
//
// If the initialization is successful, it accepts a new TCP connection on the Encoder's server.
// If there is an error while accepting the connection, it sets an error message and exits.
//
// After establishing the TCP connection, it spawns a new goroutine to handle the encoding process.
// In this goroutine, it first sets the 'started' field to 1 to indicate that the encoding has started.
//
// It then enters a loop where it reads messages from the TCP socket, processes them based on their
// type, and sends the processed data to the Encoder's channel.
//
// When it encounters an error while reading a message or receives an end message, it breaks the loop.
// It then waits for the encoder routine to exit using the pkst_wait_routine C function and sends
// the last received value to the channel.
//
// After that, it closes the TCP socket, the channel, and cleans up the encoder configuration by
// calling the pkst_free_encoder_config C function. It also sets the 'started' field to 0 to indicate
// that the encoding has stopped.
//
// The function uses the sync.Once's Do method to ensure that this sequence of operations is only performed
// once. If an error occurs at any point, the Do function will still consider the operation as having
// been performed, so subsequent calls will still have no effect.
//
// The function returns any error that occurred during the initialization or nil if the initialization
// was successful.
func (pe *Encoder) Start() error {
	var err error

	doOnce := func() {
		ret := C.pkst_start_encoder_routine(pe.config, &pe.routine)
		if ret != 0 {
			err = fmt.Errorf("unable to start encoder routine, errcode: %d", int(ret))
			return
		}

		socket, e := pe.server.AcceptTCP()
		if e != nil {
			err = fmt.Errorf("on accept connection: %v", e)
			return
		}

		go func() {

			atomic.StoreInt32(&pe.started, 1)
			// Close the socket
			defer socket.Close()
			for {
				mType, mBuff, err := ReadMessage(socket)
				if err != nil {
					goto end
				}
				switch mType {
				case MESSAGE_REPORT_END:
				case MESSAGE_REPORT_STATS:
					os, e := ParseOutputStats(mBuff[:len(mBuff)-1])
					if e == nil {
						pe.Chan <- os
					}
					if mType == MESSAGE_REPORT_END {
						goto end
					}
				case MESSAGE_REPORT_START:
					mi, err := ParseMediaInfo(mBuff[:len(mBuff)-1])
					if err == nil {
						pe.Chan <- mi
					}
				}
			}
		end:
			var cint C.int
			// Wait exit routine
			C.pkst_wait_routine(&(pe.routine), unsafe.Pointer(&cint))
			// Send last value to the channel
			pe.Chan <- int(cint)
			// Close the channel
			close(pe.Chan)
			// Cleanup Config
			C.pkst_free_encoder_config(&(pe.config))
			// Set starting to 0
			atomic.StoreInt32(&pe.started, 0)
		}()
	}
	pe.startOnce.Do(doOnce)

	return err
}

// Cancel cancels the encoding routine in a thread-safe manner.
// The actual cancellation is done by the pkst_cancel_routine C function.
// This function should only be called once per Encoder. Subsequent calls will have no effect.
// The function uses the sync.Once's Do method to ensure that the cancellation is only performed once.
func (pe *Encoder) Cancel() {
	pe.cancelOnce.Do(func() {
		C.pkst_cancel_routine(pe.routine)
	})
}

// IsStarted checks if the encoding has started.
// It reads the 'started' field in a thread-safe manner using the atomic package's LoadInt32 function.
// It returns true if 'started' is 1 and false otherwise.
func (pe *Encoder) IsStarted() bool {
	return atomic.LoadInt32(&pe.started) == 1
}

// Wait blocks until the encoding routine exits and returns the last integer received from the channel
// and a boolean indicating whether an integer was received.
// It reads values from the channel in a loop and keeps track of the last integer received.
// When the channel is closed (which should happen when the encoding routine exits), it breaks the loop.
// If it received at least one integer, it sets the received flag to true.
func (pe *Encoder) Wait() (int, bool) {
	var ret int
	var received bool
	for v := range pe.Chan {
		if val, ok := v.(int); ok {
			ret = val
			received = true
		}
	}
	return ret, received
}
