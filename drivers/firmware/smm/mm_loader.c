// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for installing Linux-owned SMI handler
 *
 * Copyright (c) 2025 9elements GmbH
 *
 * Author: Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <asm/realmode.h>

#include "smm.h"

#define DRIVER_NAME "mm_loader"

extern struct mm_info *mm_info;
extern struct smram_info *smram;
#ifdef CONFIG_S3_SUPPORT
extern struct s3_comm_info *s3_info;
#endif

/* Getter for CBTABLE entries exposed by dedicated parsers.
 * Should also free the memory allocated for the exposed structs.
 */
 
static struct smm_data get_cb_data(void)
{
	WARN_ON(mm_info == NULL || smram == NULL); 
#ifdef CONFIG_S3_SUPPORT
	WARN_ON(s3_info == NULL);
#endif
	int cpu_count;
	cpu_count = num_possible_cpus();

	struct smm_data ret = {
		.mm_info = *mm_info,
		.nr_of_smm_regions = smram->nr_of_smm_regions,
		.cpu_count = cpu_count,
#ifdef CONFIG_S3_SUPPORT
		.s3_info = *s3_info
#endif
	};

	for (int i = 0; i < ret.nr_of_smm_regions; i++) {
		ret.region[i].physical_start = smram->descriptor[i].physical_start;
		ret.region[i].cpu_start = smram->descriptor[i].cpu_start;
		ret.region[i].physical_size = smram->descriptor[i].physical_size;
		ret.region[i].region_state = smram->descriptor[i].region_state;

	}

	// just so that the parser devices can be unmounted
	kfree(mm_info);
	kfree(smram);
#ifdef CONFIG_S3_SUPPORT
	kfree(s3_info);
#endif

	return ret;
}

static int trigger_smi(uint64_t cmd, uint64_t arg, uint64_t retry){
	// the values here have to be 64bit otherwise the compiler will cmplain
	uint64_t status;
	uint16_t apmc_port = 0xb2;
	asm volatile (
		"movq	%[cmd],  %%rax\n\t"
		"movq   %%rax,	%%rcx\n\t"
		"movq	%[arg],  %%rbx\n\t"
		"movq   %[retry],  %%r8\n\t"
		".trigger:	  \n\t"
		"mov	%[apmc_port], %%dx\n\t"
		"outb	%%al, %%dx\n\t"
		"cmpq	%%rcx, %%rax\n\t"
		"jne .return_changed\n\t"
		"pushq  %%rcx\n\t"
		"movq   $10000, %%rcx\n\t"
		"rep    nop\n\t"
		"popq   %%rcx\n\t"
		"cmpq   $0, %%r8\n\t"
		"je     .return_not_changed\n\t"
		"decq   %%r8\n\t"
		"jmp    .trigger\n\t"
		".return_changed:\n\t"
// the dummy prints to the console should not be compiled for normal run - writing directly
// to serial can (and usually will) cause some issues. Useful for debugging tho.
#ifdef CONFIG_DEBUG_KERNEL
		"movw $0x3f8, %%dx\n\t"
		"movb $'c', %%al\n\t"
		"outb %%al, %%dx\n\t"
		"movb $'\n', %%al\n\t"
		"outb %%al, %%dx\n\t"
#endif
		"movq	%%rax, %[status]\n\t"
		"jmp	.end\n\t"
		".return_not_changed:"
#ifdef CONFIG_DEBUG_KERNEL
		"movw $0x3f8, %%dx\n\t"
		"movb $'n', %%al\n\t"
		"outb %%al, %%dx\n\t"
		"movb $'\n', %%al\n\t"
		"outb %%al, %%dx\n\t"
#endif
		"movq	%%rcx, %[status]\n\t"
		".end:\n\t"
		: [status] "=r" (status)
		: [cmd] "r" (cmd), [arg] "r" (arg), [retry] "r" (retry) , [apmc_port] "r" (apmc_port)
		: "%rax", "%rbx", "%rdx", "%rcx", "%r8"
	);

	// For debugging - it is useful to know what exact value was written to RAX by SMI handler.
	printk(KERN_DEBUG "%s: SMI returned 0x%llx\n", __func__, ((status >> 8) & 0xff));
	
	if(status == cmd) {
		status = PAYLOAD_MM_RET_FAILURE;
	} else
		status = PAYLOAD_MM_RET_SUCCESS;
	
	return status;
}

static int unlock_smram(struct smm_data *data)
{
	uint64_t cmd;
	uint8_t status;

	cmd = data->mm_info.register_mm_entry_swsmi | (PAYLOAD_MM_UNLOCK_SMRAM << 8);
	status = trigger_smi(cmd, 0, 5); 
	pr_info(DRIVER_NAME ": %s: SMI returned %llx\n", __func__);

	return status;
}

static int lock_smram(struct smm_data *data)
{
	uint64_t cmd;
	uint8_t status;

	cmd = data->mm_info.register_mm_entry_swsmi | (PAYLOAD_MM_LOCK_SMRAM << 8);
	status = trigger_smi(cmd, 0, 5);
	pr_info(DRIVER_NAME ": %s: SMI returned %llx\n", __func__);

	return status;
}

static int register_entry_point(struct smm_data *data, uint32_t entry_point)
{
	uint64_t cmd;
	uint8_t status;

	cmd = data->mm_info.register_mm_entry_swsmi | (PAYLOAD_MM_REGISTER_ENTRY << 8);
	status = trigger_smi(cmd, entry_point, 5);
	pr_info(DRIVER_NAME ": %s: SMI returned %llx\n", __func__);

	return status;
}

static void notrace test(void *unused)
{
	pr_info("I am here\n");
}

static int __init mm_loader_init(void)
{
	struct smm_data cb_data = get_cb_data();
	uint32_t entry_point;

	is_for_smm = SMM_INIT_HANDLER;
	ending_code = (unsigned long)test;

	int status_unlock = 1;
	status_unlock = unlock_smram(&cb_data);

	mdelay(100);
	printk(KERN_DEBUG "status is %d\n", status_unlock);

	/*
	 * For now let the location be default SMBASE - this is not desired place,
	 * but it is safe for now (till we have the address of the region translated properly),
	 * we overwrite the stub code that is used for relocation only anyways.
	 */
	const uintptr_t location = 0x38000;

	/*
	 * Size is hardcoded to 3000: real_mode_blob without mm_trampoline.S is 6000, and
	 * with is 9000, therefore since we want all instructions that are coming after 
	 * pa_mm_startup_32, we copy the real_mode_blob - mm_trampoline.S.
	 */
	void __iomem *addr = ioremap((resource_size_t)location, 3000); 

	/* 
	 * There are two things we have to do before telling coreboot where the handler is:
	 * - modify is_for_smm so that head_$(BITS) will point to the handler code instead
	 *   of continuing with the boot procedure (we do NOT want that to be done). I.e.
	 *   it will call ending_code.
	 * - let ending_code point to the handler code (for now to the dummy func).
	 */
	is_for_smm = SMM_INIT_HANDLER;
	ending_code = (unsigned long)test;
	
	/*
	 * Depending on bitness of coreboot, we copy different entry code to SMRAM
	 */
	if (!cb_data.mm_info.requires_long_mode_call)
		memcpy_toio(addr, __va(real_mode_header->mm_trampoline_start64), 3000);
	else
		memcpy_toio(addr, __va(real_mode_header->mm_startup_32), 3000);

	wbinvd();
	

	int status_reg = 1;
	status_reg = register_entry_point(&cb_data, 0x38000);

	mdelay(100);
	printk(KERN_DEBUG "status is %d\n", status_reg);

	int status_lock = 1;
	status_lock = lock_smram(&cb_data);

	mdelay(100);
	printk(KERN_DEBUG "status is %d\n", status_lock);

	return 0;
}

static void __exit mm_loader_exit(void)
{
	// Nothing to be cleaned here.
	printk(KERN_DEBUG "DONE");
}

module_init(mm_loader_init);
module_exit(mm_loader_exit);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("MM loader - installs Linux-owned SMI handler");
MODULE_LICENSE("GPL v2");
