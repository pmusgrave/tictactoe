CC=g++
OPT=-Og -std=c++17
BUILD_DIR=build

CPP_SOURCES= \
src/*.cpp \
#test/*.cpp

DEPS= \
inc/*.hpp \

CPP_INCLUDES= \
-Iinc \

LIBS= -lncurses
CFLAGS = $(OPT) $(CPP_INCLUDES) $(LIBS) -Wall

all: build/main
	build/main

build/main: $(CPP_SOURCES) $(DEPS)
	$(CC) $(CPP_INCLUDES) src/*.cpp -o $(BUILD_DIR)/main $(CFLAGS)

.PHONY: all test clean
clean:
	rm -f $(BUILD_DIR)/*
	rm -f test/*.o
