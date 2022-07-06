#!/bin/sh

build() {
    set -o xtrace
	cmake -DBUILD_SHARED_LIBS=False \
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

