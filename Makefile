CFLAGS  := -Wall -Wextra -O2 -g
CC      := gcc

all: test

test: utils/mem.o utils/heap.o utils/trie.o utils/bit_stream.o utils/byte_stream.o						\
	rle/rle.o 														\
	lz77/lz77.o 														\
	lzss/lzss.o 														\
	lz78/lz78.o 														\
	lzw/lzw.o 														\
	huffman/huffman_tree.o huffman/huffman_table.o huffman/huffman.o 							\
	deflate/huffman.o deflate/lz77.o deflate/fix_huffman.o deflate/dyn_huffman.o deflate/no_compression.o deflate/deflate.o	\
	test.o
	$(CC) $(CFLAGS) -o $@ $^

.o: .c
	$(CC) $(CFLAGS) -c $^

clean :
	rm -f *.o */*.o test
