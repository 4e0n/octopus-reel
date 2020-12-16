#!/bin/bash

./s00_install_packages.sh
./s01_get_linux.sh
./s02_get_musl.sh
./s03_get_comedi.sh
./s04_get_rtai.sh
./s05_compile_linux.sh
./s06_compile_musl.sh
./s07_compile_comedi_1.sh
./s07_compile_comedi_2.sh
./s08_compile_comedi.sh
