CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -s
#CFILES = parsecfg.c common.c trace.c cachesim.c
CFILES = cachesim.c trace.c common.c
#HFILES = parsecfg.h commonn.h trace.h
HFILES = trace.h common.h
OBJECTS = $(CFILES:.c=.o)
TARGET = parse

all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@
include depth.make
clean:
	rm $(TARGET) *.o
depth.make: $(CFILES) $(HFILES)
	gcc -MM $(CFILES) > depth.make
