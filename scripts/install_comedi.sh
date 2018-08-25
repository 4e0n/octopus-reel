#!/bin/bash

CURDIR=$(pwd)

ln -sf /usr/realtime/modules /lib/modules/4.9.80/kernel/rtai
rm /usr/realtime/modules/modules
depmod -a

cd /usr/src/comedi
./autogen.sh
./configure
make -j$(getconf _NPROCESSORS_ONLN)
make install
depmod -a
cp include/linux/comedi.h /usr/include/linux/
cp include/linux/comedilib.h /usr/include/linux/
ln -sf /usr/include/linux/comedilib.h /usr/include/linux/kcomedilib.h

# COMEDILIB
cd /usr/src/comedilib
./autogen.sh
./configure --prefix=/usr --sysconfdir=/etc
make -j$(getconf _NPROCESSORS_ONLN)
make install

# COMEDI_CALIBRATE
cd /usr/src/comedi_calibrate
autoreconf -v -i
./configure --prefix=/usr --sysconfdir=/etc
make -j$(getconf _NPROCESSORS_ONLN)
make install

echo 'options comedi comedi_num_legacy_minors=4' > /etc/modprobe.d/comedi.conf

groupadd --system iocard
echo 'KERNEL=="comedi*", MODE="0660", GROUP="iocard"' > /etc/udev/rules.d/95-comedi.rules
udevadm trigger

echo 'rtai_hal' >> /etc/modules
echo 'rtai_sched' >> /etc/modules
echo 'comedi' >> /etc/modules
echo 'rtai_comedi' >> /etc/modules

modprobe comedi
udevadm trigger
ls /dev/comedi*

#comedi_config /dev/comedi0 ni_mio_cs
#comedi_board_info /dev/comedi0
#comedi_test -t info -f /dev/comedi0
#comedi_calibrate --reset --calibrate -f /dev/comedi0
#comedi_soft_calibrate -f /dev/comedi0

#adduser $USER iocard #usermod $USER -a -G iocard

# RESIZE COMEDI BUFFER
#comedi_config -r /dev/comedi0
#comedi_config --read-buffer 640 --write-buffer 640 /dev/comedi0 ni_pcimio

