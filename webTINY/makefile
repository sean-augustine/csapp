CC = gcc
CFLAGS = -g
OBJECTS = tiny makesum

all:$(OBJECTS)

tiny: tiny.o webtiny.o
	$(CC) $(CFLAGS) $^ -o $@
	
makesum: makesum.cc
	g++ -g -std=c++11 $^ -o $@

tiny.o:webtiny.h
webtiny.o:webtiny.h

clean:
	rm $(OBJECTS) *.o