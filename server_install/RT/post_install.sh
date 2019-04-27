

comedi_calibrate --reset --calibrate -f /dev/comedi0




#modprobe comedi
#udevadm trigger
#ls /dev/comedi*

# Postinstall - Manual
#adduser $USER iocard
#modprobe comedi
#udevadm trigger
#ls /dev/comedi*
#comedi_board_info /dev/comedi0
#comedi_test -t info -f /dev/comedi0

#comedi_config /dev/comedi0 ni_mio_cs
# RESIZE COMEDI BUFFER
#comedi_config -r /dev/comedi0
#comedi_config --read-buffer 640 --write-buffer 640 /dev/comedi0 ni_pcimio
#comedi_config /dev/comedi0 ni_mio_cs
#comedi_board_info /dev/comedi0
#comedi_test -t info -f /dev/comedi0
#comedi_soft_calibrate -f /dev/comedi0
