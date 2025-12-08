TARGETS = nimd
CC     = gcc
CFLAGS = -g -std=c99 -Wall -Wvla -fsanitize=address,undefined

all: $(TARGETS)

%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGETS): nimd.o protocol.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGETS) *.o *.a *.dylib *.dSYM

.PHONY: all clean

nimd.o: nimd.c protocol.h
protocol.o: protocol.c protocol.h