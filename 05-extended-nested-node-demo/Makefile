CC = gcc
TARGET = extended_nested_node_fuse
SRC = extended_nested_node_fuse.c
CFLAGS += $(shell pkg-config fuse3 --cflags)
LDLIBS += $(shell pkg-config fuse3 --libs)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(TARGET)
