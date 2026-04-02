CC = gcc
TARGET = whoami_fuse
SRC = whoami_fuse.c
CFLAGS += $(shell pkg-config fuse3 --cflags)
LDLIBS += $(shell pkg-config fuse3 --libs)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(TARGET)
