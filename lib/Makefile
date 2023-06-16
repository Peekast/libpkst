# Definir el compilador
CC = gcc

# Definir las banderas del compilador
CFLAGS = -g -Wall -fPIC -I/opt/homebrew/Cellar/ffmpeg/5.1.2/include

# Definir la biblioteca compartida
LIBRARY = libpkst.so

# Define los archivos fuente
SOURCES = pkst_iocontext.c pkst_audio.c netutils.c keyvalue.c pkst_mediainfo.c pkst_string.c pkst_stats.c pkst_routine.c pkst_msgproto.c

LDFLAGS = -L/opt/homebrew/Cellar/ffmpeg/5.1.2/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale -ljansson
# Define los archivos de objeto
OBJECTS = $(SOURCES:.c=.o)

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(CC) -shared -o $(LIBRARY) $(OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBRARY) $(OBJECTS)
