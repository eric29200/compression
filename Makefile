CFLAGS  := -Wall -Wextra -O3 -g
CC      := gcc

all: compress

compress: mem.o bit_stream.o lz77.o no_compression.o huffman.o fix_huffman.o deflate.o compress.o
	$(CC) $(CFLAGS) -o $@ $^

.o: .c
	$(CC) $(CFLAGS) -c $^

clean :
	rm -f *.o */*.o compress
