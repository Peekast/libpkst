package main

import (
	"fmt"
	//	pkst "libpkst"
	pkst "libpkstenc"
)

func main() {

	encoderConfig := pkst.EncoderConfig{
		In:      "input_source", // reemplaza "input_source" por la fuente de entrada real
		Listen:  true,           // asumiremos que el codificador debe escuchar las conexiones
		Timeout: 30,             // establece un período de tiempo de espera, por ejemplo, 30
		InType:  "input_type",   // reemplaza "input_type" por el tipo de entrada real
		Audio: &pkst.AudioConfig{
			Codec:      "audio_codec", // reemplaza "audio_codec" por el codec de audio real
			BitRate:    128,           // por ejemplo, 128
			SampleRate: 44100,         // por ejemplo, 44100
			Channels:   2,             // por ejemplo, 2
		},
		Outputs: [4]*pkst.EncoderOutput{
			{
				Dst:        "output1_dst",  // reemplaza "output1_dst" por el destino de la salida real
				Type:       "output1_type", // reemplaza "output1_type" por el tipo de salida real
				KVOpts:     "output1_opts", // reemplaza "output1_opts" por las opciones adicionales
				IgnoreFail: true,           // asumiremos que se deben ignorar los fallos
			},
			// Hacer lo mismo para las salidas adicionales si son necesarias
			nil,
			nil,
			nil,
		},
	}

	encoder, err := pkst.NewEncoder(&encoderConfig)
	fmt.Println(encoder, err)
}
