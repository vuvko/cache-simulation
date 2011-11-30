CC = gcc
CFLAGS = -O2 -Wall -Werror -Wformat-security -Wignored-qualifiers -Winit-self -Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith -Wtype-limits -Wempty-body -Wlogical-op -Wstrict-prototypes -Wold-style-declaration -Wold-style-definition -Wmissing-parameter-type -Wmissing-field-initializers -Wnested-externs -Wno-pointer-sign -std=gnu99
LDFLAGS = -s
CFILES = cachesim.c trace.c common.c parsecfg.c statistics.c direct_cache.c full_cache.c memory.c random.c cache.c
HFILES = trace.h common.h parsecfg.h statistics.h direct_cache.h full_cache.h memory.h random.h cache.h
OBJECTS = $(CFILES:.c=.o)
TARGET = cachesim

all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@
include depth.make
clean:
	rm $(TARGET) *.o
depth.make: $(CFILES) $(HFILES)
	gcc -MM $(CFILES) > depth.make
