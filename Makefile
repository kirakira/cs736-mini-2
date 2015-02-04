.PHONY: all

all: clocks latency

clocks: clocks.cc
	g++ -O2 -o clocks -lrt clocks.cc

latency: latency.cc
	g++ -O2 -o latency -lrt latency.cc
