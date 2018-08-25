# Octopus-ReEL
(0ct0pu5) Octopus Realtime EEG Laboratory


Directory Hierarchy

Online Applications:
--------------------

$OCTOPUS_BASE/online/:
Main include files common to almost all online applications residing in
the project. The files are:

$OCTOPUS_BASE/online/fbcommand.h:
Commands and other constants related with ACQ/STIM frontend<->backend
communication, all kinds of low-level control.

$OCTOPUS_BASE/online/cscommand.h:
Commands and other constants related with client-server communication
mainly focused on setting stim-related parameters. The reboot and shutdown
commands are for both acquisition and stimulation servers.

$OCTOPUS_BASE/online/acq_common.h:
The constants related to acquisition servers backend and frontend application.
This file is included both from kernel space backend C and the frontend
C++ codes. So the common coding styles between C & C++ has been adhered.

$OCTOPUS_BASE/online/stim_common.h/:
Constants related to stimulation server backend and frontend..
This file is included both from kernel space backend C and the frontend
C++ codes. So the common coding styles between C & C++ has been adhered.

$OCTOPUS_BASE/online/pattdatagram.h/:
The structure for segmented transfer of the stimulus application pattern
to the stimulus server over network.


$OCTOPUS_BASE/online/channelnames.h:
These are the electrode names arhering to the Neuroscan 128-channel cap
pinout list..

$OCTOPUS_BASE/online/eventcodes.h/:
In this file are the standard event & response codes. Lots of reserved
slots exists for future paradigm definitions.

$OCTOPUS_BASE/online/eventnames.h/:
These are the names of the events defined in eventcodes.h for displaying on
screen.

==============================================================================

octopus-acq (128-channel Acquisition+event channel SYNChronized over
 parallel-port interrupt):

This is the Acquisition Server/Daemon and its associated realtime kernel-space
backend. It is supposed/tested to run over a computer with Debian/GNU Linux-4.0,
RTAI-3.5 and comedi-0.6.67/comedilib-0.6.70, with compatible A/D conversion
hardware, one parallel port and a serial (RS-232) port, and a compatible 100Mbit
Ethernet device. No console needed.

octopus-stim (Stereo auditory event stimulator+calibration/diagnostic of
 individual amplifiers):

This is Stimulation/Amplifier Calibration/Normalization server and its associated
realtime kernel-space backend. It is supposed/tested to run over a computer with
Debian/GNU Linux-4.0, RTAI-3.5 and comedi-0.6.67/comedilib-0.6.70, with compatible
D/A conversion hardware, one parallel port, and a compatible 10/100MBit Ethernet
device. No console needed.

octopus-recorder (Recording and -Online- Averaging GUI Frontend/Client):

Supposed to run over a fast (preferably multi-core/64-bit) computer with a
compatible 100Mbit ethernet device and a compatible 2D/3D/OpenGL/DRI accelerated
X11 Graphics Adapter. Supposed/tested computer has an Athlon X2-4200 CPU over an
ASUS M200 Mainboard. The on-board nVidia Graphics Adapter is compatible and fast
enough so no add-on Graphics Adapter has been used. The system has 2GBs of 800Mhz
DDR2 Memory. A 21" CRT monitor (1600x1200@85Hz) have been used to fully
utilize/view the 128-channel range. The operating system is
Debian/GNU-Linux-4.0-amd64 distribution.

octopus-analyzer (Offline GUI Data/viewer and averaging tool):

octopus-patt (Random pattern generator tool for IIDITD binaural auditory
 paradigm):
