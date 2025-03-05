// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for installing SMI handler and locking down SMM
 *
 * Copyright (c) 2025 9elements GmbH
 *
 * Author: Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>

#include "smm.h"

extern struct smram_info *smram;
extern struct smm_registers_info *smm_regs;
extern struct spi_flash_info *spi_info;

static int __init smm_loader_init(void)
{
	printk(KERN_INFO "the smram number of regions is %x\n",
	       smram->nr_of_smm_regions);
	for (int i = 0; i < smm_regs->count; i++) {
		printk(KERN_INFO "regs round %d", i);
		printk(KERN_INFO "0x%x\n", smm_regs->registers[i].register_id);
		printk(KERN_INFO "0x%x\n",
		       smm_regs->registers[i].address_space_id);
		printk(KERN_INFO "0x%x\n",
		       smm_regs->registers[i].register_bit_width);
		printk(KERN_INFO "0x%x\n",
		       smm_regs->registers[i].register_bit_offset);
		printk(KERN_INFO "0x%x\n", smm_regs->registers[i].value);
		printk(KERN_INFO "0x%x\n", smm_regs->registers[i].address);
	}
	printk(KERN_INFO "spi inf\n");
	printk(KERN_INFO "0x%x\n", spi_info->revision);
	printk(KERN_INFO "0x%x\n", spi_info->flags);
	printk(KERN_INFO "0x%x\n", spi_info->spi_address.register_id);
	printk(KERN_INFO "0x%x\n", spi_info->spi_address.address_space_id);
	printk(KERN_INFO "0x%x\n", spi_info->spi_address.register_bit_width);
	printk(KERN_INFO "0x%x\n", spi_info->spi_address.register_bit_offset);
	printk(KERN_INFO "0x%x\n", spi_info->spi_address.value);
	printk(KERN_INFO "0x%x\n", spi_info->spi_address.address);

	return 0;
}

static void __exit smm_loader_exit(void)
{
	// placeholder for now
	printk(KERN_INFO "SMM initialized");
	kfree(smram);
	kfree(smm_regs);
	kfree(spi_info);
}

module_init(smm_loader_init);
module_exit(smm_loader_exit);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("SMM Loader driver - installs permanent SMI handler");
MODULE_LICENSE("GPL v2");
