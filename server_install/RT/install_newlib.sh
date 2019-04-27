#!/bin/bash

# Lately math library support in RTAI seems to have become fragile. So, this part is most likely going to be abandoned; however still preserved for convenience if you absolutely need math functions and willing to hack into it.

mkdir /usr/src/newlib
cd /usr/src/newlib
cvs -z 9 -d :pserver:anoncvs@sourceware.org:/cvs/src login
#pw: anoncvs
cvs -z 9 -d :pserver:anoncvs@sourceware.org:/cvs/src co newlib
#mkdir install
./configure --prefix=/usr/src/newlib/install --disable-shared CFLAGS="-fno-pie -O2 -mcmodel=kernel"
#make -j$(getconf _NPROCESSORS_ONLN)
#make install

