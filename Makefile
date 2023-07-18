.PHONY = all clean

CC = g++ -std=c++23

# SRCS := $(wildcard *.c)
# BINS := $(SRCS:%.c=bin/%)

all: hello chess set

bin:
	mkdir -p bin

hello: bin hello.cpp
	${CC} hello.cpp -o bin/hello

chess: bin chess.cpp
	${CC} chess.cpp -o bin/chess
# 	${CC} -std=c++20 chess.cpp -o chess

set: bin set.cpp
	${CC} set.cpp -o bin/set

clean:
	rm -rfv bin
