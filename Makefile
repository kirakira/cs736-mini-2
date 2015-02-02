.PHONY: all

all: clocks

clocks: clocks.cc
	g++ -O2 -o clocks -lrt clocks.cc
