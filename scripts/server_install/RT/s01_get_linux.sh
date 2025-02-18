#!/bin/bash

KERNEL_VERSION="4.9.176"

CURDIR=$(pwd)
cd /usr/src

# Get Linux Kernel
wget http://cdn.kernel.org/pub/linux/kernel/v4.x/linux-${KERNEL_VERSION}.tar.xz
xz -d linux-${KERNEL_VERSION}.tar.xz
tar -xf linux-${KERNEL_VERSION}.tar
rm linux-${KERNEL_VERSION}.tar
ln -sf linux-${KERNEL_VERSION} linux
ln -sf linux-${KERNEL_VERSION} b

cd $CURDIR
