#!/bin/bash
touch /etc/udev/rules.d/90-eego.rules
echo "ATTRS{idVendor}==\"2a56\",ATTRS{idProduct}==\"ee01\",SYMLINK+=\"eego3.%n\",MODE:=\"0666\",ENV{DEVTYPE}==\"usb_device\"" >> /etc/udev/rules.d/90-eego.rules
cp ./SDK/linux/64bit/libeego-SDK.so /usr/lib/
cp -R ./SDK/eemagine /usr/include/
