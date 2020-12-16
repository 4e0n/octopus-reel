#!/bin/bash

CURDIR=$(pwd)

cd /usr/src

# Compile MUSL libm.a
cd /usr/src/musl
./configure --disable-shared CFLAGS="-fno-common -fno-pic"
make
ar -dv lib/libc.a fwrite.o write.o fputs.o sprintf.o strcpy.o strlen.o memcpy.o memset.o
ar -dv lib/libc.a cpow.o cpowf.o cpowl.o
cp lib/libc.a lib/libm.a

cd $CURDIR
