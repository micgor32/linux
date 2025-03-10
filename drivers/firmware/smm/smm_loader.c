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
#include <linux/cpu.h>

#include "smm.h"

extern struct smram_info *smram;
extern struct smm_registers_info *smm_regs;
extern struct spi_flash_info *spi_info;
extern struct s3_comm_info *s3_info;

/* Getter for CBTABLE entries exposed by dedicated parsers.
 * Should also free the memory allocated for the exposed structs.
 */
static struct smm_data get_cb_data(void)
{
	// Sanity checks for null pointers (check whether for e.g. we are indeed running CB with SMM payload).
	if (smram == NULL || smm_regs == NULL || spi_info == NULL || s3_info == NULL) {
		struct smm_data dummy; // DO NOT LEAVE THIS HERE!!
		return dummy;
	}

	int cpu_count;
	cpu_count = num_possible_cpus();

	struct smm_data ret = {
		.smram = *smram,
		.registers = *smm_regs,
		.spi_flash_info = *spi_info,
		.s3_info = *s3_info,
		.cpu_count = cpu_count,
	};
	
	// This is a placeholder for now, just dumping the values. To be cleaned up.
	printk(KERN_INFO "the smram number of regions is %x\n",
	       smram->nr_of_smm_regions);
	for (int i = 0; i < smram->nr_of_smm_regions; i++) {
		printk(KERN_INFO "smram region %d", i);
		printk(KERN_INFO "start 0x%llx\n", smram->descriptor[i].physical_start);
		printk(KERN_INFO "size 0x%zx\n",
		       (size_t)smram->descriptor[i].physical_size);
		printk(KERN_INFO "state 0x%llx\n",
		       smram->descriptor[i].region_state);
	}
	for (int i = 0; i < smm_regs->count; i++) {
		printk(KERN_INFO "regs round %d", i);
		printk(KERN_INFO "register id %d\n", smm_regs->registers[i].register_id);
		printk(KERN_INFO "address_space_id %d\n",
		       smm_regs->registers[i].address_space_id);
		printk(KERN_INFO "register_bit_width %d\n",
		       smm_regs->registers[i].register_bit_width);
		printk(KERN_INFO "register_bit_width %d\n",
		       smm_regs->registers[i].register_bit_offset);
		printk(KERN_INFO "reg value 0x%x\n", smm_regs->registers[i].value);
		printk(KERN_INFO "address 0x%x\n", smm_regs->registers[i].address);
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

	printk(KERN_INFO "s3 info\n");
	printk(KERN_INFO "start 0x%llx\n", s3_info->physical_start);
	printk(KERN_INFO "size 0x%zx\n", (size_t)s3_info->physical_size);
	printk(KERN_INFO "state 0x%llx\n", s3_info->region_state);
	printk(KERN_INFO "acpi s3 enabled %d\n", s3_info->pld_acpi_s3_enable);

	printk(KERN_INFO "cpu count: %d", cpu_count);
	// end of statements to be cleaned up

	// freeing the memory (also temp for now, see whether it makes sense here).
	kfree(smram);
	kfree(smm_regs);
	kfree(spi_info);
	kfree(s3_info);

	return ret;
}

static int setup_reloc_handler(struct smm_state *params)
{
	// place holder
	return 0;
}

static int setup_perm_handler(struct smm_state *params)
{
	// placeholder
	return 0;
}

/* Will install the relocation handler */
static int load_reloc_handler(struct smm_data *data) // left for reference from cb declaration (int nr_cpus, size_t save_state_size)
{
	struct smm_state params = {
		.cpu_count = data->cpu_count,
		.smm_save_state_size = 0,
	};

	if (setup_reloc_handler(&params)) {
		printk(KERN_ERR "reloc handler didnt worked");
		return 1;
	}

	return 0;
}

/* Will install the permantent handler */
static int load_permanent_handler(struct smm_data *data) //left for reference sform cb declaration (int nr_cpus , size_t save_state_size, uintptr_t smbase, size_t smram_size)
{
	struct smm_state params = {
		.cpu_count = data->cpu_count,
		.perm_smbase = data->smram.descriptor[0].physical_start,
		.perm_smsize = data->smram.descriptor[0].physical_size,
		.smm_save_state_size = 0,
	};

	if (setup_perm_handler(&params)) {
		printk(KERN_ERR "perm handler didnt worked");
		return 1;
	}

	return 0; 
}

static int __init smm_loader_init(void)
{
	struct smm_data cb_data = get_cb_data();
	
	// read the state bit of smram regions to confirm its not locked, a sanity check to avoid nasty errors if we try to access regions that are somehow already locked (unlikely but still).
	printk(KERN_INFO "sanity check");
	for (int i = 0; i < cb_data.smram.nr_of_smm_regions; i++) {
		// for now we will just print out the state value, idk what is the meaning of particular ones, i.e. which value means region is locked or not
		printk(KERN_INFO "state 0x%llx", cb_data.smram.descriptor[i].region_state);
	}

	// load the handlers
	if (load_permanent_handler(&cb_data)) {
		printk(KERN_ERR "loading permanent handler didnt worked");
		return 1;
	}

	if (load_reloc_handler(&cb_data)) {
		printk(KERN_ERR "loading reloc handler didnt worked");	
	}

	return 0;
}

static void __exit smm_loader_exit(void)
{
	// placeholder for now
	printk(KERN_INFO "SMM initialized");
	/*kfree(smram);*/
	/*kfree(smm_regs);*/
	/*kfree(spi_info);*/
}

module_init(smm_loader_init);
module_exit(smm_loader_exit);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("SMM Loader driver - installs permanent SMI handler");
MODULE_LICENSE("GPL v2");
