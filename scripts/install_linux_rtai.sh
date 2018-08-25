#!/bin/bash

CURDIR=$(pwd)

# Kernel
cd /usr/src
wget ftp://ftp.metu.edu.tr/pub/mirrors/ftp.kernel.org/pub/linux/kernel/v4.x/linux-4.9.80.tar.xz
xz -d linux-4.9.80.tar.xz
tar -xf linux-4.9.80.tar
rm linux-4.9.80.tar
ln -sf linux-4.9.80 linux
ln -sf linux-4.9.80 b

# RTAI
wget --no-check-certificate https://www.rtai.org/userfiles/downloads/RTAI/rtai-5.1.tar.bz2
tar --bzip2 -xf rtai-5.1.tar.bz2
rm rtai-5.1.tar.bz2
ln -sf rtai-5.1 rtai
ln -sf rtai-5.1 a

cp rtai/base/arch/x86/patches/hal-linux-4.9.80-x86-4.patch .
patch -p0 < hal-linux-4.9.80-x86-4.patch
rm hal-linux-4.9.80-x86-4.patch

# COMEDI
cd /usr/src
git clone https://github.com/Linux-Comedi/comedi.git
git clone https://github.com/Linux-Comedi/comedilib.git
git clone https://github.com/Linux-Comedi/comedi_calibrate.git
rm -r /lib/modules/`uname -r`/kernel/drivers/staging/comedi

# INIT SRC
cd /usr/src/linux
make mrproper
cd /usr/src/rtai
make distclean

cd $CURDIR
cp octopus_linux_config /usr/src/linux/.config
cp octopus_rtai_config /usr/src/rtai/.rtai_config

