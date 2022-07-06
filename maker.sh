#!/bin/sh

build() {
    set -o xtrace
	cmake -DSFML_USE_SYSTEM_DEPS=True \
          -DBUILD_SHARED_LIBS=False \
          -DCMAKE_OSX_ARCHITECTURES=x86_64 \
          -G Ninja -S . -B ./Build \
    && ninja -C ./Build
}

clean() {
    set -o xtrace
	rm -rf ./Build/*
}

help() {
	echo "Использование:"
	echo "make build -> Собрать проект с помощью CMake и ninja ./Build/"
	echo "make clean -> Удалить содержимое директории ./Build/"
}

case "$1" in
    build) build ;;
    clean) clean ;;
    *) help ;;
esac

