#!/bin/bash

RTAI_CONFIG="./kernel_config/octopus_rtai_config_acer"

KERNEL_VERSION=4.9.80

CURDIR=$(pwd)

if [ -d "/usr/realtime" ]; then
 rm -rf /usr/realtime
fi

cd /usr/src/rtai
make distclean
cd $CURDIR
cp ${RTAI_CONFIG} /usr/src/rtai/.rtai_config
cd /usr/src/rtai
make
make install
ln -sf /usr/realtime/modules /lib/modules/${KERNEL_VERSION}/kernel/rtai
if [ -d "/usr/realtime/modules/modules" ]; then
 rm -rf /usr/realtime/modules/modules
fi
depmod -a

cd $CURDIR
