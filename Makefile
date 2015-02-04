.PHONY: all

all: clocks latency

clocks: clocks.cc
	g++ -Wall -Wextra -O2 -o clocks -lrt clocks.cc

latency: latency.cc
	g++ -Wall -Wextra -O2 -o latency -lrt -lpthread latency.cc
