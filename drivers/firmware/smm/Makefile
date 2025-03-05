# SPDX-License-Identifier: GPL-2.0
obj-y += smram.o
obj-y += smm_registers.o
obj-y += spi_info.o
obj-y += s3_comm.o
obj-y += smm_loader.o

#obj-y += smm_loader.o commented out for now, uncomment for full build
# This is for testing purposes

#obj-m := smm_loader.o
#obj-m := smm_init_reloc.o
# obj-m += smram.o
# obj-m += smm_registers.o
# obj-m += spi_info.o
# obj-m += s3_comm.o
#
# KDIR  := /home/micgor32/tmp/coreboot-24.12/payloads/external/LinuxBoot/build/kernel-6_12_11 # hardcoded path, remove later
# PWD   := $(shell pwd)
#
# default:
# 	$(MAKE) -C $(KDIR) M=$(PWD) modules
# 	#sh uroot.sh
#
clean:
	rm -f *.mod
	rm -f *.mod.*
	rm -f *.ko
	rm -f *.o
	rm -r .*
	rm -f modules.order
	rm -f Module.symvers
# help:
# 	@echo "Available targets:"
# 	@echo "  default      - Build all targets"
# 	@echo "	 clean	      - Clean up the source tree"
# 	@echo "  help         - Show this help message"
