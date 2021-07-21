.PHONY: clean all
.DEFAULT_GOAL := all

ifeq ($(OS),Windows_NT)
	MAKE=mingw32-make
else
	MAKE=make
endif
CC=g++
CFLAGS=-Wall -Wpedantic -ggdb -std=c++11

ifneq ($(OS),Windows_NT)
	ifeq ($(shell uname), Darwin)
		override CFLAGS += -arch x86_64
	endif
endif

SRC = game/game.cpp

INCLUDE_DIR = "include"
LIB_DIR = "lib"
BUILD_DIR = "build"
PACKAGE_DIR = "package"


all: game

game: OBJ=$(wildcard build/*.o)
game: engine logger.o include/engine/interface.h $(SRC)
	$(CC) $(CFLAGS) -I./$(INCLUDE_DIR) game/game.cpp $(OBJ) -L./$(LIB_DIR) -lsfml-window -lsfml-graphics -lsfml-system -o $(PACKAGE_DIR)/game.exe

engine:
	$(MAKE) -C include/engine PREFIX=../..

logger.o: include/logger/logger.h include/logger/logger.cpp
	$(MAKE) -C include/logger PREFIX=../..


clean:
ifeq ($(OS),Windows_NT)
	del /q "$(BUILD_DIR)\*"
else
	@rm -f "$(BUILD_DIR)/*"
endif