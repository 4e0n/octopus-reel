#!/bin/bash

#apt-get -y install dselect net-tools

apt-get -y install patch binutils libc6-dev gcc make bc libncurses5-dev libc-dev-bin linux-libc-dev manpages-dev cpp gcc-6 gcc-multilib autoconf automake libtool flex bison cpp-6 libcc1-0 libgcc-6-dev libisl15 libmpc3 libmpfr4 libgomp1 libitm1 libatomic1 libasan3 liblsan0 libtsan0 libcilkrts5 libmpx2 libquadmath0

apt-get -y install g++ git libgsl0-dev libboost-program-options-dev

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

cd /usr/src/linux
make mrproper
cd ..
cp oct_linux_config /usr/src/linux/




# COMEDI
cd /usr/src
git clone https://github.com/Linux-Comedi/comedi.git
git clone https://github.com/Linux-Comedi/comedilib.git
git clone https://github.com/Linux-Comedi/comedi_calibrate.git
#rm -r /lib/modules/`uname -r`/kernel/drivers/staging/comedi

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
cd /usr/local/src/comedilib
./autogen.sh
./configure --prefix=/usr --sysconfdir=/etc
make -j$(getconf _NPROCESSORS_ONLN)
make install

groupadd --system iocard
echo 'KERNEL=="comedi*", MODE="0660", GROUP="iocard"' > /etc/udev/rules.d/95-comedi.rules
udevadm trigger
#adduser $USER iocard #usermod $USER -a -G iocard

modprobe comedi
udevadm trigger
ls /dev/comedi*
comedi_board_info /dev/comedi0
#comedi_test -t info -f /dev/comedi0

comedi_config /dev/comedi0 ni_mio_cs

#/etc/modprobe.d/comedi.conf:
options comedi comedi_num_legacy_minors=4

# COMEDI_CALIBRATE
cd /usr/local/src/comedi_calibrate
autoreconf -v -i
./configure --prefix=/usr --sysconfdir=/etc
make -j$(getconf _NPROCESSORS_ONLN)
make install

# RESIZE COMEDI BUFFER
comedi_config -r /dev/comedi0
comedi_config --read-buffer 640 --write-buffer 640 /dev/comedi0 ni_pcimio

comedi_calibrate --reset --calibrate -f /dev/comedi0
comedi_soft_calibrate -f /dev/comedi0

#!/bin/bash

apt-get -y install cvs

cd /usr/local/src
mkdir newlib
cd newlib
cvs -z 9 -d :pserver:anoncvs@sourceware.org:/cvs/src login
#pw: anoncvs
cvs -z 9 -d :pserver:anoncvs@sourceware.org:/cvs/src co newlib
mkdir install
cd src/newlib
./configure --prefix=/usr/local/src/newlib/install --disable-shared CFLAGS="-fno-pie -O2 -mcmodel=kernel"
make -j$(getconf _NPROCESSORS_ONLN)
make install

