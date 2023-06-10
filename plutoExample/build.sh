#! /bin/sh

arm-linux-gnueabihf-gcc -mfloat-abi=hard --sysroot=files/sysroot -std=gnu99 -g -o pluto_stream src/ad9361-iiostream.c -lpthread -liio -lm -Wall -Wextra --warn-unused-variable
