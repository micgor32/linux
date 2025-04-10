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
#include <asm/realmode.h>

#include <../../../arch/x86/kernel/apic/local.h>
#include "smm.h"

extern struct smram_info *smram;
extern struct smm_registers_info *smm_regs;
#ifdef SPI_SMM
extern struct spi_flash_info *spi_info;
#endif
#ifdef S3_SUPPORT_SMM
extern struct s3_comm_info *s3_info;
#endif
extern struct mm_info *mm_info;

/* Getter for CBTABLE entries exposed by dedicated parsers.
 * Should also free the memory allocated for the exposed structs.
 */
 
static struct smm_data get_cb_data(void)
{
	// Sanity checks for null pointers (check whether for e.g. we are indeed running CB with SMM payload).
	/*if (smram == NULL || smm_regs == NULL || spi_info == NULL || s3_info == NULL || mm_info = NULL) {*/
	/*	struct smm_data dummy; // DO NOT LEAVE THIS HERE!!*/
	/*	return dummy;*/
	/*}*/

	int cpu_count;
	cpu_count = num_possible_cpus();

	struct smm_data ret = {
		.smram = *smram,
		//.registers = *smm_regs,
#ifdef SPI_SMM
		.spi_flash_info = *spi_info,
#endif
#ifdef S3_SUPPORT_SMM
		.s3_info = *s3_info,
#endif
		.mm_info = *mm_info,
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
	printk(KERN_INFO "mm entry 0x%x\n", mm_info->register_mm_entry_swsmi);
	// end of statements to be cleaned up

	// freeing the memory (also temp for now, see whether it makes sense here).
	kfree(smram);
	//kfree(smm_regs);
#ifdef SPI_SMM
	kfree(spi_info);
#endif
#ifdef S3_SUPPORT_SMM
	kfree(s3_info);
#endif
	kfree(mm_info);

	return ret;
}

static uintptr_t stack_top;
static size_t global_stack_size;

static int setup_stack(struct smm_data *data)
{
	size_t stack_size = (size_t)data->smram.stack_size;
	if (stack_size <= SMM_MINIMUM_STACK_SIZE || (stack_size & 3) != 0) {
		printk(KERN_ERR "too small stack size\n");
		return -1;
	}

	const size_t total_stack_size = data->cpu_count * stack_size;
	printk(KERN_INFO "total_stack_size is 0x%zx", total_stack_size);
	if (total_stack_size >= data->smram.perm_smsize) {
		printk(KERN_ERR "%s: Stack won't fit smram\n", __func__);
		return -1;
	}
	
	stack_top = data->smram.descriptor[0].physical_start + total_stack_size;
	global_stack_size = stack_size;
	printk(KERN_INFO "stack top 0x%x", stack_top);
	printk(KERN_INFO "stack size 0x%zx", global_stack_size);
	return 0;

}

static void notrace do_relocation(void *unused) 
{

	/*asm volatile (*/
	/*       "movw $0x3f8, %%dx\n\t"*/
	/*       "movb $'i', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'m', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $' ', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'i', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'n', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $' ', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'s', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'m', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'m', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'\n', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "rsm\n\t"*/
	/*       :*/
	/*       :*/
	/*       : "al", "dx"*/
	/*);*/
	pr_info("if everything went well, we are in smm, on cpu %d\n", smp_processor_id());
	/*asm volatile (*/
	/*       "movw $0x3f8, %%dx\n\t"*/
	/*       "movb $'i', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'m', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $' ', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'i', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'n', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $' ', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'s', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'m', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'m', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "movb $'\n', %%al\n\t"*/
	/*       "outb %%al, %%dx\n\t"*/
	/*       "rsm\n\t"*/
	/*       :*/
	/*       :*/
	/*       : "al", "dx"*/
	/*);*/
}


/*static int load_initial_stub(uintptr_t loc, void *start)*/
/*{*/
/*	// again, placeholder*/
/*	printk(KERN_INFO "address 0x%02hh\n", start);*/
/*	// once we have */
/*	return 0;*/
/*}*/

//extern struct stub_data stub_entry_params;
extern u32 smm_relocation;
//extern uint8_t smm_relocation_end;

static int load_trampoline(uintptr_t location)
{
	printk(KERN_INFO "location where stub will be placed 0x%x", location);
	void __iomem *addr = ioremap((resource_size_t)location, 47);
	if (!addr) {
		printk(KERN_ERR "Failed to ioremap the target address\n");
		return -ENOMEM;
	}

	// debug prints remove
	printk(KERN_INFO "virt 0x%lx\n", addr);
	printk(KERN_INFO "src 0x%lx\n", &smm_relocation);
	// ...till here	

	memcpy_toio(addr, &smm_relocation, 47);

	// memcpy the address from
	wbinvd();
	
	return 0;
}

static int setup_stub_params(const uintptr_t smbase, const size_t smm_size, struct smm_state *params)
{
	const uintptr_t stub_location = smbase + SMM_ENTRY_OFFSET;
	/*if(load_initial_stub(stub_location, &params->smmstub_start)) {*/
	/*	printk(KERN_ERR "fialed to load initial stub\n");*/
	/*	return -1;*/
	/*}*/

	//stub_params = kmalloc(sizeof(*stub_params), GFP_KERNEL);
	
	/*stub_entry_params.stack_top = stack_top;*/
	/*stub_entry_params.stack_size = global_stack_size;*/
	/*stub_entry_params.c_handler = (uintptr_t)do_reloc; // pass that info together with state for later, will make my life easier when dealing with permanent handler*/
	/*stub_entry_params.cr3 = params->cr3;*/

	/*int i;*/
	/**/
	/*for_each_online_cpu(i) {*/
	/*	stub_entry_params.apic_to_cpu_num[i] = per_cpu(x86_cpu_to_apicid, i);*/
	/*	printk(KERN_INFO "cpu %d, apic id %d", i, stub_entry_params.apic_to_cpu_num[i]);*/
	/*}*/
	/**/
	/*if (i != params->cpu_count) {*/
	/*	printk(KERN_ERR "Failed to set up APIC map\n");*/
	/*	return -1;*/
	/*}*/

	if(load_trampoline(stub_location))
		return 1;
	
	return 0;
}

//static DEFINE_SPINLOCK(reloc_lock);

static void initiate_relocation(void)
{
	int i = 1000;
	const bool x2apic = boot_cpu_has(X86_FEATURE_X2APIC);

	apic->send_IPI(smp_processor_id(), LAPIC_INT_ASSERT | LAPIC_DM_SMI);

	while (x2apic && i--) {
		cpu_relax();
	}
}

static int setup_reloc_handler(struct smm_state *params)
{
	uintptr_t smbase = SMM_DEFAULT_SMBASE;
	if (params->nr_cnn_save_states > 1)
		return -1;

	/*if (params->handler == NULL)*/
	/*	return -1;*/

	return setup_stub_params(smbase, SMM_DEFAULT_SIZE, params);
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
	
	/*if (load_reloc_handler(&cb_data)) {*/
	/*	printk(KERN_ERR "loading reloc handler didnt worked");	*/
	/*	return -1;*/
	/*}*/
	/**/
	/**/
	/*// load the handlers*/
	/*if (load_permanent_handler(&cb_data)) {*/
	/*	printk(KERN_ERR "loading permanent handler didnt worked");*/
	/*	return -1;*/
	/*}*/
	const uintptr_t stub_location = SMM_DEFAULT_SMBASE + SMM_ENTRY_OFFSET;

	//load_trampoline(stub_location);
	//apic->send_IPI_allbutself(LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	//outb(0x00, 0xb2);
	//
	//int y;
	//y = setup_stack(&cb_data);
	
	printk(KERN_INFO "bsp %d", get_boot_cpu_id());
	
	/*int i;*/
	/**/
	/*for_each_online_cpu(i) {*/
	/*	trampoline_header->apic_to_cpu_num[i] = per_cpu(x86_cpu_to_apicid, i);*/
	/*	printk(KERN_INFO "cpu %d, apic id %d", i, trampoline_header->apic_to_cpu_num[i]);*/
	/*}*/

	printk(KERN_INFO "the flag is %d", is_for_smm);
	is_for_smm = SMM_INIT_HANDLER;
	printk(KERN_INFO "and now %d", is_for_smm);
	ending_code = (unsigned long)do_relocation;

	//default_send_IPI_single_phys(0, LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	//default_send_IPI_mask_sequence_phys(cpu_online_mask, LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	

	printk("ttttttttttttttttt\n");

	//__apic_send_IPI(smp_processor_id(), LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	//outb(0x00, 0xb2);

	//initiate_relocation();

	/*for_each_online_cpu(i){*/
	/*	if (!is_for_smm) */
	/*		pr_info("not good");*/
	/*}*/
	

	//initiate_relocation();
	// for testing, lets see what happens
	/*unsigned long flags;*/
	/**/
	/*printk(KERN_INFO "obtaining lock before sending SMI");*/
	/*spin_lock_irqsave(&reloc_lock, flags);*/
	/*initiate_relocation();*/
	/*spin_unlock_irqrestore(&reloc_lock, flags);*/
	/*printk(KERN_INFO "lock released");*/

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
