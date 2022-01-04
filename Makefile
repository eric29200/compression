CFLAGS  := -Wall -Wextra -O2 -g
LDFLAGS	:= -lm
CC      := gcc

all: compress

compress: mem.o bit_stream.o lz77.o fix_huffman.o deflate.o compress.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.o: .c
	$(CC) $(CFLAGS) -c $^

clean :
	rm -f *.o */*.o compress