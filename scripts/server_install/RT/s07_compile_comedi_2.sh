#!/bin/bash

# COMEDI Housekeeping
groupadd --system iocard
echo 'options comedi comedi_num_legacy_minors=4' > /etc/modprobe.d/comedi.conf
echo 'KERNEL=="comedi*", MODE="0660", GROUP="iocard"' > /etc/udev/rules.d/95-comedi.rules
udevadm trigger

echo 'rtai_hal' >> /etc/modules
echo 'rtai_sched' >> /etc/modules
echo 'rtai_comedi' >> /etc/modules
echo 'comedi_test' >> /etc/modules
