# Definir el compilador
CC = gcc

CFLAGS = -fPIC -Wall -g `pkg-config --cflags libavcodec libavdevice libavfilter libavformat libavutil libswresample libswscale jansson`
LDFLAGS = `pkg-config --libs libavcodec libavdevice libavfilter libavformat libavutil libswresample libswscale jansson`

# Definir la biblioteca compartida
LIBRARY_SHARED = libpkst.so
LIBRARY_STATIC = libpkst.a

# Define los archivos fuente
SOURCES = pkst_iocontext.c pkst_audio.c netutils.c keyvalue.c pkst_mediainfo.c pkst_log.c pkst_string.c pkst_routine.c pkst_msgproto.c pkst_iobuffer.c

OBJECTS = $(SOURCES:.c=.o)

all: $(LIBRARY_SHARED) $(LIBRARY_STATIC)

$(LIBRARY_SHARED): $(OBJECTS)
	$(CC) -shared -o $(LIBRARY_SHARED) $(OBJECTS) $(LDFLAGS)

$(LIBRARY_STATIC): $(OBJECTS)
	ar rcs $(LIBRARY_STATIC) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBRARY_SHARED) $(LIBRARY_STATIC) $(OBJECTS)

install:
	cp libpkst.so /usr/local/lib/
	cp libpkst.a /usr/local/lib/
	mkdir -p /usr/local/include/libpkst && cp *.h /usr/local/include/libpkst/
