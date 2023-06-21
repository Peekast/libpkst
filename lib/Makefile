# Definir el compilador
CC = gcc
CFLAGS = -fPIC -Wall -g `pkg-config --cflags libavcodec libavdevice libavfilter libavformat libavutil libpostproc libswresample libswscale jansson`
LDFLAGS = `pkg-config --libs libavcodec libavdevice libavfilter libavformat libavutil libpostproc libswresample libswscale jansson`

# Definir las banderas del compilador
#CFLAGS = -g -Wall -fPIC -I/opt/homebrew/Cellar/ffmpeg/5.1.2/include

# Definir la biblioteca compartida
LIBRARY = libpkst.so

# Define los archivos fuente
SOURCES = pkst_iocontext.c pkst_audio.c netutils.c keyvalue.c pkst_mediainfo.c pkst_log.c pkst_string.c pkst_routine.c pkst_msgproto.c pkst_iobuffer.c

#LDFLAGS = -L/opt/homebrew/Cellar/ffmpeg/5.1.2/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale -ljansson
# Define los archivos de objeto
OBJECTS = $(SOURCES:.c=.o)

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(CC) -shared -o $(LIBRARY) $(OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBRARY) $(OBJECTS)

install:
	cp libpkst.so /usr/local/lib/