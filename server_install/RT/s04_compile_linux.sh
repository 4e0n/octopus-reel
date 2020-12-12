#!/bin/bash

LINUX_CONFIG="./kernel_config/octopus_linux_config_stmx"

KERNEL_VERSION="4.9.80"

CURDIR=$(pwd)
cd /usr/src

# Compile and Install Linux Kernel
rm /boot/*${KERNEL_VERSION}*

if [ -d "/lib/modules/${KERNEL_VERSION}" ]; then
 rm -rf /lib/modules/${KERNEL_VERSION}
fi

cd /usr/src/linux
make mrproper
cd $CURDIR
cp ${LINUX_CONFIG} /usr/src/linux/.config
cd /usr/src/linux
make
make modules
make modules_install
make install

cd $CURDIR
