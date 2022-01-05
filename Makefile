CFLAGS  := -Wall -Wextra -O3
LDFLAGS	:= -lm
CC      := gcc

all: compress

compress: mem.o bit_stream.o lz77.o no_compression.o fix_huffman.o deflate.o compress.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.o: .c
	$(CC) $(CFLAGS) -c $^

clean :
	rm -f *.o */*.o compress
