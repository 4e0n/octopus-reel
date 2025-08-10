#!/bin/bash
touch /etc/udev/rules.d/90-eego.rules
touch /etc/udev/rules.d/91-octopus-hsync.rules
touch /etc/udev/rules.d/92-octopus-power.rules
echo "ATTRS{idVendor}==\"2a56\",ATTRS{idProduct}==\"ee01\",SYMLINK+=\"eego3.%n\",MODE:=\"0666\",ENV{DEVTYPE}==\"usb_device\"" >> /etc/udev/rules.d/90-eego.rules
echo "SUBSYSTEM==\"tty\", ATTRS{idVendor}==\"1b4f\", ATTRS{idProduct}==\"9206\",ENV{ID_MM_DEVICE_IGNORE}=\"1\",MODE=\"0660\", GROUP=\"octopus\", TAG+=\"uaccess\",SYMLINK+=\"octopushsync%n\"" >> /etc/udev/rules.d/91-octopus-hsync.rules
echo "ACTION==\"add|change\", SUBSYSTEM==\"usb\", ATTR{idVendor}==\"1b4f\", ATTR{idProduct}==\"9206\",TEST==\"power/control\",ATTR{power/control}=\"on\",TEST==\"power/autosuspend_delay_ms\", ATTR{power/autosuspend_delay_ms}=\"-1\"" >> /etc/udev/rules.d/92-octopus-power.rules

echo -1 | sudo tee /sys/module/usbcore/parameters/autosuspend

#cp ./SDK/linux/64bit/libeego-SDK.so /usr/lib/
#cp -R ./SDK/eemagine /usr/include/

