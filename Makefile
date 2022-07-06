.PHONY: build clean

all:
	@echo "Использование:"
	@echo "make build -> Собрать проект с помощью CMake и ninja ./Build/"
	@echo "make clean -> Удалить содержимое директории ./Build/"

build:
	cmake -DSFML_USE_SYSTEM_DEPS=True -DBUILD_SHARED_LIBS=False -DCMAKE_OSX_ARCHITECTURES=x86_64 -G Ninja -S . -B ./Build && ninja -C ./Build

clean: 
	rm -rf ./Build/*
