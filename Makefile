.PHONY: clean all
.DEFAULT_GOAL := all

ifeq ($(OS),Windows_NT)
	MAKE=mingw32-make
else
	MAKE=make
endif
CXX=g++
CXXFLAGS=-Wall -Wpedantic -ggdb -std=c++11

ifneq ($(OS),Windows_NT)
	ifeq ($(shell uname), Darwin)
		override CXXFLAGS += -arch x86_64
	endif
endif

INCLUDE_DIR = "include"
LIB_DIR = "lib"
BUILD_DIR = "build"
PACKAGE_DIR = "package"
TARGET = "$(PACKAGE_DIR)/game.exe"

all: $(TARGET)
$(TARGET): OBJ=$(wildcard build/*.o)
$(TARGET): engine logger $(BUILD_DIR)/game.o
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $(OBJ) -L$(LIB_DIR) -lsfml-window -lsfml-graphics -lsfml-audio -lsfml-system -o $@

$(BUILD_DIR)/game.o: game/game.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

engine:
	$(MAKE) -C include/engine PREFIX=../..

logger:
	$(MAKE) -C include/logger PREFIX=../..

clean:
ifeq ($(OS),Windows_NT)
	del /q "$(BUILD_DIR)\*"
else
	@rm -f "$(BUILD_DIR)/*"
endif