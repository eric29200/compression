CFLAGS  := -Wall -Wextra -O2 -g
LDFLAGS	:= -lm
CC      := gcc

all: deflate

deflate: mem.o bit_stream.o lz77.o deflate.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.o: .c
	$(CC) $(CFLAGS) -c $^

clean :
	rm -f *.o */*.o deflate
