CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I./include

SRCS = src/price_level.cpp src/orderbook.cpp src/engine.cpp
OBJS = $(SRCS:.cpp=.o)

all: main

main: main.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o main main.cpp $(OBJS)

bench: benchmarks/bench.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -O3 -o bench benchmarks/bench.cpp $(OBJS)
	./bench

test: tests/test_matching.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o test_runner tests/test_matching.cpp $(OBJS)
	./test_runner

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o main bench test_runner

.PHONY: all bench test clean