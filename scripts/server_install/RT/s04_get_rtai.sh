#!/bin/bash

RTAI_VERSION="5.1"
RTAI_VERSION2="5.2"
LINUX_VERSION="4.9.80"
PATCH_REV="4"

CURDIR=$(pwd)

cd /usr/src

# Get RTAI
wget --no-check-certificate https://www.rtai.org/userfiles/downloads/RTAI/rtai-${RTAI_VERSION}.tar.bz2
tar --bzip2 -xf rtai-${RTAI_VERSION}.tar.bz2
rm rtai-${RTAI_VERSION}.tar.bz2
ln -sf rtai-${RTAI_VERSION} rtai
ln -sf rtai-${RTAI_VERSION} a
cp rtai/base/arch/x86/patches/hal-linux-${LINUX_VERSION}-x86-${PATCH_REV}.patch .
patch -p0 < hal-linux-${LINUX_VERSION}-x86-${PATCH_REV}.patch
rm hal-linux-${LINUX_VERSION}-x86-${PATCH_REV}.patch


rm -rf /usr/src/rtai-${RTAI_VERSION}
wget --no-check-certificate https://www.rtai.org/userfiles/downloads/RTAI/rtai-${RTAI_VERSION2}.tar.bz2
tar --bzip2 -xf rtai-${RTAI_VERSION2}.tar.bz2
ln -sf rtai-${RTAI_VERSION2} rtai
rm rtai-${RTAI_VERSION2}.tar.bz2

cd $CURDIR
