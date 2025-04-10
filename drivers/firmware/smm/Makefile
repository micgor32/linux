#SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_SMM_PAYLOAD) += smram.o
obj-$(CONFIG_SMM_PAYLOAD) += smm_registers.o
obj-$(CONFIG_SPI_SMM) += spi_info.o
obj-$(CONFIG_S3_SUPPORT_SMM) += s3_comm.o
obj-$(CONFIG_SMM_PAYLOAD) += smm_loader.o
obj-$(CONFIG_MM_PAYLOAD) += mm_info.o
obj-$(CONFIG_MM_PAYLOAD) += mm_loader.o
# maybe trampoline as well
# obj-y += trampoline.o

#obj-y += smm_loader.o commented out for now, uncomment for full build
# This is for testing purposes

# obj-m += smram.o
# obj-m += smm_registers.o
# obj-m += spi_info.o
# obj-m += s3_comm.o
# obj-m += load.o
# load-y := smm_loader.o trampoline.o
#
# KDIR  := /home/micgor32/LinuxBootSMM/linux #/home/micgor32/tmp/coreboot-24.12/payloads/external/LinuxBoot/build/kernel-6_12_11 # hardcoded path, remove later
# PWD   := $(shell pwd)
#
# default:
# 	$(MAKE) -C $(KDIR) M=$(PWD) modules
# 	sh uroot.sh
#
# clean:
# 	rm -f *.mod
# 	rm -f *.mod.*
# 	rm -f *.ko
# 	rm -f *.o
# 	rm -f modules.order
# 	rm -f Module.symvers
# help:
# 	@echo "Available targets:"
# 	@echo "  default      - Build all targets"
# 	@echo "	 clean	      - Clean up the source tree"
# 	@echo "  help         - Show this help message"
