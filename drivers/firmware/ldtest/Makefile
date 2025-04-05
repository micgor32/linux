# SPDX-License-Identifier: GPL-2.0

obj-m += load.o
load-y := smm_loader.o 

KDIR  := /home/micgor32/LinuxBootSMM/linux #/home/micgor32/tmp/coreboot-24.12/payloads/external/LinuxBoot/build/kernel-6_12_11 # hardcoded path, remove later
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	sh uroot.sh

clean:
	rm -f *.mod
	rm -f *.mod.*
	rm -f *.ko
	rm -f *.o
	rm -f modules.order
	rm -f Module.symvers
help:
	@echo "Available targets:"
	@echo "  default      - Build all targets"
	@echo "	 clean	      - Clean up the source tree"
	@echo "  help         - Show this help message"
