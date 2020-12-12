#!/bin/bash

CURDIR=$(pwd)

cd /usr/src

# Get COMEDI
git clone https://github.com/Linux-Comedi/comedi.git
git clone https://github.com/Linux-Comedi/comedilib.git
git clone https://github.com/Linux-Comedi/comedi_calibrate.git
if [ -d "/lib/modules/${KERNEL_VERSION}/kernel/drivers/staging/comedi" ]; then
 rm -r /lib/modules/${KERNEL_VERSION}/kernel/drivers/staging/comedi
fi
cd $CURDIR
