CC=cc
CFLAGS=-c -Wall `MagickWand-config --cflags` -g
LDFLAGS=`MagickWand-config --ldflags`
SOURCES=draw_me.c palette.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=draw_me

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
