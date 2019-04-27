#!/bin/bash

CURDIR=$(pwd)

# Endure needed packages
apt -y install dselect gcc g++ cvs git autoconf automake libtool bison flex patch binutils libc6-dev make bc libncurses5-dev libc-dev-bin linux-libc-dev manpages-dev cpp gcc-6 gcc-multilib cpp-6 libcc1-0 libgcc-6-dev libisl15 libmpc3 libmpfr4 libgomp1 libitm1 libatomic1 libasan3 libcilkrts5 libmpx2 libquadmath0 g++ git libgsl-dev libgsl0-dev libboost-program-options-dev vim exuberant-ctags cvs

apt-get install libtsan0 liblsan0


cd /usr/src

# Get Linux Kernel
wget ftp://ftp.metu.edu.tr/pub/mirrors/ftp.kernel.org/pub/linux/kernel/v4.x/linux-4.9.80.tar.xz
xz -d linux-4.9.80.tar.xz
tar -xf linux-4.9.80.tar
rm linux-4.9.80.tar
ln -sf linux-4.9.80 linux
ln -sf linux-4.9.80 b

# Get RTAI
wget --no-check-certificate https://www.rtai.org/userfiles/downloads/RTAI/rtai-5.1.tar.bz2
tar --bzip2 -xf rtai-5.1.tar.bz2
rm rtai-5.1.tar.bz2
ln -sf rtai-5.1 rtai
ln -sf rtai-5.1 a
cp rtai/base/arch/x86/patches/hal-linux-4.9.80-x86-4.patch .
patch -p0 < hal-linux-4.9.80-x86-4.patch
rm hal-linux-4.9.80-x86-4.patch
rm a b

# Get COMEDI
git clone https://github.com/Linux-Comedi/comedi.git
git clone https://github.com/Linux-Comedi/comedilib.git
git clone https://github.com/Linux-Comedi/comedi_calibrate.git
rm -r /lib/modules/`uname -r`/kernel/drivers/staging/comedi


# Compile and Install Linux Kernel
rm /boot/*4.9.80*
rm -rf /lib/modules/4.9.80
cd /usr/src/linux
make mrproper
cd $CURDIR
cp octopus_linux_config /usr/src/linux/.config
cd /usr/src/linux
make
make modules
make modules_install
make install

# Compile RTAI
rm -rf /usr/realtime
cd /usr/src/rtai
make distclean
cd $CURDIR
cp octopus_rtai_config /usr/src/rtai/.rtai_config
cd /usr/src/rtai
make
make install
ln -sf /usr/realtime/modules /lib/modules/4.9.80/kernel/rtai
rm -rf /usr/realtime/modules/modules
depmod -a

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

# COMEDI Housekeeping
groupadd --system iocard
echo 'options comedi comedi_num_legacy_minors=4' > /etc/modprobe.d/comedi.conf
echo 'KERNEL=="comedi*", MODE="0660", GROUP="iocard"' > /etc/udev/rules.d/95-comedi.rules
udevadm trigger

echo 'rtai_hal' >> /etc/modules
echo 'rtai_sched' >> /etc/modules
echo 'rtai_comedi' >> /etc/modules
echo 'comedi_test' >> /etc/modules

cd $CURDIR
