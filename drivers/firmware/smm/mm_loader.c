// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for installing SMI handler and locking down SMRAM
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

extern struct mm_info *mm_info;
extern struct smram_info *smram;
extern struct s3_comm_info *s3_info;

/* Getter for CBTABLE entries exposed by dedicated parsers.
 * Should also free the memory allocated for the exposed structs.
 */
 
static struct smm_data get_cb_data(void)
{
	WARN_ON(mm_info == NULL || smram == NULL || s3_info == NULL);

	int cpu_count;
	cpu_count = num_possible_cpus();

	struct smm_data ret = {
		.mm_info = *mm_info,
		.smram = *smram,
		.cpu_count = cpu_count,
		.s3_info = *s3_info
	};

	// just so that the parser devices can be unmounted
	kfree(mm_info);
	kfree(smram);
	kfree(s3_info);

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
		/*"movw $0x3f8, %%dx\n\t"*/ // debug dummy print
		/*"movb $'3', %%al\n\t"*/
		/*"outb %%al, %%dx\n\t"*/
		"movq	%%rax, %[status]\n\t"
		"jmp	.end\n\t"
		".return_not_changed:"
		/*"movw $0x3f8, %%dx\n\t"*/ // s.a.
		/*"movb $'n', %%al\n\t"*/
		/*"outb %%al, %%dx\n\t"*/
		"movq	%%rcx, %[status]\n\t"
		".end:\n\t"
		: [status] "=r" (status)
		: [cmd] "r" (cmd), [arg] "r" (arg), [retry] "r" (retry) , [apmc_port] "r" (apmc_port)
		: "%rax", "%rbx", "%rdx", "%rcx", "%r8" //, "%al" , "%dx" // only if prints are outcommented
	);

	printk(KERN_INFO "SMI returned %llx\n", status);//((status >> 8) & 0xff));
	
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
	printk(KERN_INFO, "we send to rax 0x%llx\n", cmd);
	status = trigger_smi(cmd, 0, 5); 
	return status;
}

static int lock_smram(struct smm_data *data)
{
	uint64_t cmd;
	uint8_t status;

	cmd = data->mm_info.register_mm_entry_swsmi | (PAYLOAD_MM_LOCK_SMRAM << 8);
	status = trigger_smi(cmd, 0, 5);
	return status;
}

static int register_entry_point(struct smm_data *data, uint32_t entry_point)
{
	uint64_t cmd;
	uint8_t status;

	cmd = data->mm_info.register_mm_entry_swsmi | (PAYLOAD_MM_REGISTER_ENTRY << 8);
	status = trigger_smi(cmd, entry_point, 5);
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

	int status_unlock = 1;
	status_unlock = unlock_smram(&cb_data); //register_entry_point(&cb_data, 0x7ff0000);	

	mdelay(100);
	printk(KERN_INFO "status is %d\n", status_unlock);

	/*status_unlock = unlock_smram(&cb_data);*/
	/**/
	/*mdelay(10000);*/

	// so now we can install the entry point
	
	// first see whether this is even correct
	/*if (!cb_data.mm_info.requires_long_mode_call)*/
	/*	entry_point = __pa(real_mode_header->startup_64);*/
	/*else*/
	/*	entry_point = __pa(real_mode_header->startup_32);*/
	/**/
	/*const uintptr_t location = cb_data.s3_info.physical_start;*/
	/*void __iomem *addr = ioremap((resource_size_t)location, 47);*/
	/*printk(KERN_INFO "tseg base is 0x%lx, and va 0x%lx\n", location, addr);*/
	/**/
	/**/
	/*memcpy_toio(addr, __va(real_mode_header->debug), 6);*/
	/*wbinvd();*/

	int status_reg = 1;
	status_reg = register_entry_point(&cb_data, cb_data.s3_info.physical_start);

	mdelay(100);
	printk(KERN_INFO "status is %d\n", status_reg);

	int status_lock = 1;
	status_lock = lock_smram(&cb_data);

	mdelay(100);
	printk(KERN_INFO "status is %d\n", status_lock);

	//is_for_smm = SMM_INIT_HANDLER;
	//ending_code = (unsigned long)test; // for now its just this dummy function, next step would be to place the functions needed here in smram*/
	
	return 0;
}

static void __exit mm_loader_exit(void)
{
	printk(KERN_INFO "DONE");
}

module_init(mm_loader_init);
module_exit(mm_loader_exit);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("MM loader - installs payload owned SMI handler");
MODULE_LICENSE("GPL v2");
