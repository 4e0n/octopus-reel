# *      Octopus - Bioelectromagnetic Source Localization System 0.9.9      *
# *                   (>) GPL 2007-2009,2018- Barkin Ilhan                  *
# *            barkin@unrlabs.org, http://www.unrlabs.org/barkin            *

obj-m    := octopus-acq-backend.o

CURRENT = $(shell uname -r)
KDIR    := /lib/modules/$(shell uname -r)/build
PWD    := $(shell pwd)
EXTRA_CFLAGS += -O2 -I/usr/realtime/include -I/usr/include -Wall -ffast-math -mhard-float -msse3 -mpreferred-stack-boundary=4

default:
	@echo --------------------------------------------
	@echo PWD = $(PWD)
	@echo CURRENT = $(CURRENT)
	@echo KDIR = $(KDIR)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules 

clean:
	-rm -f -r *.o *.ko *.o.d *.symvers .*.cmd .*.flags *.mod.c modules.order .tmp_versions
