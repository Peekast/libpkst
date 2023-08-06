# libpkst
**Peekast Encoder Library**

## What does it do?

libpkst is a multiple remuxer that takes an input stream and distributes it to multiple outputs. It also supports audio re-encoding and resampling. The current version supports up to 4 simultaneous outputs.

## Requirements:

- [Jansson JSON Library](https://jansson.readthedocs.io/en/latest/gettingstarted.html#compiling-and-installing-jansson)
- libavformat/libavcodec (ffmpeg v5.1.13)

### Patch and compile libavformat
`patch -p0 < tls_schannel.patch`

`patch -p0 < tls_securetransport.patch`

`./configure --enable-pic --enable-shared`
