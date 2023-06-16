CC = g++
CFLAGS = -std=c++11 -Wall -pedantic -g -O2
all: musicparse
musicparse: music.cpp
	$(CC) $(CFLAGS) music.cpp -o musicparse

clean:
	rm -f musicparse
