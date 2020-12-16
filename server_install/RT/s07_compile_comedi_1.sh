#!/bin/bash

CURDIR=$(pwd)

cd /usr/src

# Compile and Install COMEDI
cd /usr/src/comedi
./autogen.sh
./configure
make -j$(getconf _NPROCESSORS_ONLN)
make install
depmod -a
cp include/linux/comedi.h /usr/include/linux/
cp include/linux/comedilib.h /usr/include/linux/
ln -sf /usr/include/linux/comedilib.h /usr/include/linux/kcomedilib.h

# Compile and Install COMEDILIB
cd /usr/src/comedilib
./autogen.sh
./configure --prefix=/usr --sysconfdir=/etc
make -j$(getconf _NPROCESSORS_ONLN)
make install

# Compile and Install COMEDI_CALIBRATE
cd /usr/src/comedi_calibrate
autoreconf -v -i
./configure --prefix=/usr --sysconfdir=/etc
make -j$(getconf _NPROCESSORS_ONLN)
make install

cd $CURDIR
