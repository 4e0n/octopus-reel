#!/bin/bash

apt-get -y install dselect patch binutils libc6-dev gcc make bc libncurses5-dev libc-dev-bin linux-libc-dev manpages-dev cpp gcc-6 gcc-multilib autoconf automake libtool flex bison cpp-6 libcc1-0 libgcc-6-dev libisl15 libmpc3 libmpfr4 libgomp1 libitm1 libatomic1 libasan3 libcilkrts5 libmpx2 libquadmath0 g++ git libgsl-dev libboost-program-options-dev vim exuberant-ctags cvs

#if 64-bit debian
apt-get -y install libtsan0 liblsan0

