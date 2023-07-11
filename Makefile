.PHONY = all clean

CC = g++ -std=c++23

# SRCS := $(wildcard *.c)
# BINS := $(SRCS:%.c=bin/%)

all: hello chess

bin:
	mkdir -p bin

hello: bin hello.cpp
	${CC} hello.cpp -o bin/hello

chess: bin chess.cpp
	${CC} chess.cpp -o bin/chess
# 	${CC} -std=c++20 chess.cpp -o chess

clean:
	rm -rfv bin
