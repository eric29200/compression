CFLAGS  := -Wall -Wextra -O3 -g
CC      := gcc

all: compression_test

compression_test: utils/mem.o utils/heap.o utils/bit_stream.o 										\
	deflate/lz77.o deflate/no_compression.o deflate/huffman.o deflate/fix_huffman.o deflate/dyn_huffman.o deflate/deflate.o 	\
	compression_test.o
	$(CC) $(CFLAGS) -o $@ $^

.o: .c
	$(CC) $(CFLAGS) -c $^

clean :
	rm -f *.o */*.o compression_test
