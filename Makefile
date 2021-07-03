.PHONY: clean all
.DEFAULT_GOAL := all

ifeq ($(OS),Windows_NT)
	MAKE=mingw32-make
else
	MAKE=make
endif
CC=g++
CFLAGS=-Wall -Wpedantic -ggdb -std=c++11

SRC = game/game.cpp
OBJ = $(BUILD_DIR)/engine.o

INCLUDE_DIR = "include"
LIB_DIR = "lib"
BUILD_DIR = "build"
PACKAGE_DIR = "package"


all: game
game: engine.o include/engine/interface.h $(SRC)
	$(CC) $(CFLAGS) -I./$(INCLUDE_DIR) game/game.cpp $(BUILD_DIR)/engine.o -o $(PACKAGE_DIR)/game.exe


engine.o: include/engine/base.h include/engine/base.cpp include/engine/interface.h
	$(MAKE) -C include/engine PREFIX=../..


clean:
ifeq ($(OS),Windows_NT)
	del /q "$(BUILD_DIR)\*"
else
	@rm -f "$(BUILD_DIR)/*"
endif