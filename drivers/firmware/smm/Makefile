# SPDX-License-Identifier: GPL-2.0

# obj-y += smm_loader.o commented out for now, uncomment for full build
# This is for testing purposes

obj-m := smm_loader.o
#obj-m := smm_init_reloc.o

KDIR  := /home/micgor32/tmp/coreboot-24.12/payloads/external/LinuxBoot/build/kernel-6_12_11 # hardcoded path, remove later
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# all:
# 	$(MAKE) -C $(KDIR) M=$(PWD) modules
#
# loader: smm_loader.o
# 	$(MAKE) -C $(KDIR) M=$(PWD) modules
#
# relocation: smm_init_reloc.o
# 	$(MAKE) -C $(KDIR) M=$(PWD) modules
#

clean:
	rm -f smm_*.mod
	rm -f smm_*.mod.*
	rm -f smm_*.ko
	rm -f smm_*.o
	rm -f module.order
	rm -f Module.symver
help:
	@echo "Available targets:"
	@echo "  all          - Build all modules" 
	@echo "  loader       - Build only SMM loader"
	@echo "  relocation   - Build only SMM relocation handler"
	@echo "  help         - Show this help message"
