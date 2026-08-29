CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -Iinclude

LDFLAGS = -lm

TARGET = intellicompress

SRC = \
	src/main.c \
	src/huffman.c \
	src/rle.c \
	src/lzw.c \
	src/analyzer.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run