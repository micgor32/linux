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
#include <asm/apic.h>
#include <linux/spinlock.h>
#include <linux/smp.h>

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

static uintptr_t stack_top;
static size_t global_stack_size;

static int setup_stack(struct smm_data *data)
{
	size_t stack_size = (size_t)data->smram.stack_size;
	if (stack_size <= SMM_MINIMUM_STACK_SIZE || (stack_size & 3) != 0) {
		printk(KERN_ERR "too small stack size");
		return -1;
	}

	const size_t total_stack_size = data->cpu_count * stack_size;
	printk(KERN_INFO "total_stack_size is 0x%zx", total_stack_size);
	if (total_stack_size >= data->smram.perm_smsize) {
		printk(KERN_ERR, "%s: Stack won't fit smram\n", __func__);
		return -1;
	}
	
	stack_top = data->smram.descriptor[0].physical_start + total_stack_size;
	global_stack_size = stack_size;
	return 0;

}

static asmlinkage void do_reloc(void *arg)
{
	// nothing for now
}

struct stub_data *stub_params;
static int setup_stub(const uintptr_t smbase, const size_t smm_size, struct smm_state *params)
{
	// defince stub and its size 
	
	const uintptr_t stub_location = smbase + SMM_ENTRY_OFFSET;

	stub_params = kmalloc(sizeof(*stub_params), GFP_KERNEL);
	stub_params->stack_top = stack_top;
	stub_params->stack_size = global_stack_size;
	stub_params->c_handler = (uintptr_t)do_reloc; // pass that info together with state for later, will make my life easier when dealing with permanent handler
	stub_params->cr3 = params->cr3;

	int i;

	for_each_online_cpu(i) {
		stub_params->apic[i] = per_cpu(x86_cpu_to_apicid, i);
		//printk(KERN_INFO "cpu %d, apic id %d", i, stub_params->apic[i]);
	}

	if (i != params->cpu_count) {
		printk(KERN_ERR "Failed to set up APIC map\n");
		return -1;
	}
	
	return 0;
}

static DEFINE_SPINLOCK(reloc_lock);

static void initiate_relocation(void)
{
	unsigned long flags;

	printk(KERN_INFO "obtaining lock before sending SMI");
	spin_lock_irqsave(&reloc_lock, flags);
	__apic_send_IPI_self(LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	spin_unlock_irqrestore(&reloc_lock, flags);
	printk(KERN_INFO "lock released");
	kfree(stub_params);
}

static int setup_reloc_handler(struct smm_state *params)
{
	uintptr_t smbase = SMM_DEFAULT_SMBASE;
	if (params->nr_cnn_save_states > 1)
		return -1;

	/*if (params->handler == NULL)*/
	/*	return -1;*/

	return setup_stub(smbase, SMM_DEFAULT_SIZE, params);
}

static int setup_perm_handler(struct smm_state *params)
{
	// placeholder
	return 0;
}

static int load_reloc_handler(struct smm_data *data) // left for reference from cb declaration (int nr_cpus, size_t save_state_size)
{
	struct smm_state params = {
		.cpu_count = data->cpu_count,
		.perm_smbase = data->smram.perm_smbase,
		.perm_smsize = data->smram.perm_smsize,
		.smm_save_state_size = data->smram.smm_save_state_size,
		.nr_cnn_save_states = 1,
		.cr3 = data->smram.cr3,
	};

	if (setup_reloc_handler(&params)) {
		printk(KERN_ERR "reloc handler didnt worked");
		return 1;
	}

	return 0;
}

static int load_permanent_handler(struct smm_data *data) //left for reference sform cb declaration (int nr_cpus , size_t save_state_size, uintptr_t smbase, size_t smram_size)
{
	struct smm_state params = {
		.cpu_count = data->cpu_count,
		.perm_smbase = data->smram.perm_smbase,
		.perm_smsize = data->smram.perm_smsize,
		.smm_save_state_size = data->smram.smm_save_state_size,
		.nr_cnn_save_states = (size_t)data->cpu_count,
	};

	if (setup_perm_handler(&params)) {
		printk(KERN_ERR "perm handler didnt worked");
		return 1;
	}

	return 0; 
}



static int reloc_map(const uintptr_t smbase, const uint nr_cpus, const struct smm_data *params)
{
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

	if (setup_stack(&cb_data)) {
		printk(KERN_ERR "setting up stack failed");
		return -1;
	}

	if (load_reloc_handler(&cb_data)) {
		printk(KERN_ERR "loading reloc handler didnt worked");	
		return -1;
	}


	// load the handlers
	if (load_permanent_handler(&cb_data)) {
		printk(KERN_ERR "loading permanent handler didnt worked");
		return -1;
	}

	// for testing, lets see what happens
	initiate_relocation();

	return 0;
}

static void __exit smm_loader_exit(void)
{
	// placeholder for now
	printk(KERN_INFO "SMM initialized");
	// no need for a "big" cleanup, the critical parts of memory was already freed in get_cb_data(), we have no deadlock here - proof: try executing as out-of-tree, rmmod should go without issues
	// meaning the remaining "parser" modules may be removed and do not create cross-dependenceis with this loader.
}

module_init(smm_loader_init);
module_exit(smm_loader_exit);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("SMM Loader driver - installs permanent SMI handler");
MODULE_LICENSE("GPL v2");
