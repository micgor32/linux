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

/* Getter for CBTABLE entries exposed by dedicated parsers.
 * Should also free the memory allocated for the exposed structs.
 */
static struct smm_data *cb_data;

static void get_cb_data(void)
{
	WARN_ON(!mm_info || !smram);
#ifdef CONFIG_S3_SUPPORT
	WARN_ON(!s3_info);
#endif
	int cpu_count;

	cpu_count = num_possible_cpus();

	cb_data = kmalloc(sizeof(*cb_data) +
			  sizeof(struct smram_descriptor) *
				  (smram->nr_of_smm_regions - 1),
		  GFP_KERNEL);

	cb_data->mm_info = *mm_info;
	cb_data->nr_of_smm_regions = smram->nr_of_smm_regions;
	cb_data->cpu_count = cpu_count;
#ifdef CONFIG_S3_SUPPORT
	cb_data->s3_info = *s3_info;
#endif

	for (int i = 0; i < cb_data->nr_of_smm_regions; i++) {
		cb_data->region[i].physical_start =
			smram->descriptor[i].physical_start;
		//printk(KERN_INFO "reg 0x%llx\n", cb_data->region[i].physical_start);
		cb_data->region[i].cpu_start = smram->descriptor[i].cpu_start;
		cb_data->region[i].physical_size =
			smram->descriptor[i].physical_size;
		cb_data->region[i].region_state = smram->descriptor[i].region_state;
	}

	// just so that the parser devices can be unmounted
	kfree(mm_info);
	kfree(smram);
#ifdef CONFIG_S3_SUPPORT
	kfree(s3_info);
#endif
}

static int trigger_smi(u64 cmd, u64 arg, u64 retry)
{
	// the values here have to be 64bit otherwise the compiler will cmplain
	u64 status;
	u16 apmc_port = 0xb2;

	asm volatile("movq	%[cmd],  %%rax\n\t"
		     "movq   %%rax,	%%rcx\n\t"
		     "movq	%[arg],  %%rbx\n\t"
		     "movq   %[retry],  %%r8\n\t"
		     ".trigger:\n\t"
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
		     : [status] "=r"(status)
		     : [cmd] "r"(cmd), [arg] "r"(arg), [retry] "r"(retry),
		       [apmc_port] "r"(apmc_port)
		     : "%rax", "%rbx", "%rdx", "%rcx", "%r8");

	// For debugging - it is useful to know what exact value was written to RAX by SMI handler.
	printk(KERN_INFO "%s: SMI returned 0x%016llx\n", __func__, status);//((status >> 8) & 0xff));

	if (status == cmd || ((status>>8) & 0xff))
		status = PAYLOAD_MM_RET_FAILURE;
	else
		status = PAYLOAD_MM_RET_SUCCESS;

	return status;
}

static int unlock_smram(struct smm_data *data)
{
	u64 cmd;
	u8 status;

	cmd = data->mm_info.register_mm_entry_swsmi |
	      (PAYLOAD_MM_UNLOCK_SMRAM << 8);
	status = trigger_smi(cmd, 0, 5);
	pr_info(DRIVER_NAME ": %s: SMI returned %x\n", __func__, status);

	return status;
}

static int lock_smram(struct smm_data *data)
{
	u64 cmd;
	u8 status;

	cmd = data->mm_info.register_mm_entry_swsmi |
	      (PAYLOAD_MM_LOCK_SMRAM << 8);
	status = trigger_smi(cmd, 0, 5);
	pr_info(DRIVER_NAME ": %s: SMI returned %x\n", __func__, status);

	return status;
}

static int register_entry_point(struct smm_data *data, uint32_t entry_point)
{
	u64 cmd;
	u8 status;

	cmd = data->mm_info.register_mm_entry_swsmi |
	      (PAYLOAD_MM_REGISTER_ENTRY << 8);
	status = trigger_smi(cmd, entry_point, 5);
	pr_info(DRIVER_NAME ": %s: SMI returned %x\n", __func__, status);

	return status;
}

/*
 * This function is intended to be called from SMM
 * once trampoline gets us to the kernel space.
 */
static void notrace test(void *unused)
{
	pr_info("I am here\n");
}

static u32 place_handler(void) {
	/*
	 * Size is hardcoded to 3000: real_mode_blob without mm_trampoline.S is 6000, and
	 * with is 9000, therefore since we want all instructions that are coming after
	 * pa_mm_startup_32, we copy the real_mode_blob - mm_trampoline.S.
	 * FIXME: we can reserve from tseg_payload_base + cb_data->region[1].physical_size.
	 *
	 * Now within SMM we have the following regions available (see cpu/x86/smm/tseg_region.c
	 * in coreboot source tree):
	 *
	 *     +-------------------------+
	 *     |          IED            | IED_REGION_SIZE
	 *     +-------------------------+
	 *     |  External Stage Cache   | SMM_RESERVED_SIZE
	 *     +-------------------------+
	 *     |      code and data      |
	 *     |         (TSEG)          |
	 *     +-------------------------+
	 *     | (optional payload area) |
	 *     |			 |
	 *     +-------------------------+
	 *     |	EFI reserved	 |
	 *     |   (fixed to 1000 bytes) |
	 *     +-------------------------+ TSEG
	 *
	 * Payload starts at the base of TSEG, this is platform dependent, and hence coreboot
	 * has to provide this information via CBTABLE. Now, the payload region is divided into
	 * two subregions: first is dedicated to EFI communications (EDK2 specific not implemented here),
	 * the second one (which we are claiming below) is supposed to hold all setup code we need to 
	 * get to the kernel space in SMM.
	 */
	u32 tseg_payload_base = cb_data->region[1].physical_start;
	void __iomem *tseg_v = ioremap(tseg_payload_base, 3000);

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
	printk(KERN_INFO "requires_long_mode_call %d\n", cb_data->mm_info.requires_long_mode_call);
	if (!cb_data->mm_info.requires_long_mode_call)
		memcpy_toio(tseg_v, __va(real_mode_header->mm_trampoline_start64),
			    3000);
	else
		memcpy_toio(tseg_v, __va(real_mode_header->mm_startup_32), 3000);//&entry32_end - &entry32_start);//

	wbinvd();

	return tseg_payload_base;
}

static int __init mm_loader_init(void)
{
	u32 entry_point;

	get_cb_data();

	if (unlock_smram(cb_data))
		return -1;

	mdelay(100);

	entry_point = place_handler();	
	if(register_entry_point(cb_data, entry_point)) {
		/*
		 * This is already bad, but it can get worse if SMRAM is not locked.
		 * It is safe to assume that at this point SMRAM is unlocked.
		 * Hence one more SMI is send to lock down SMRAM in whatever its state
		 * currently is.
		 */
		pr_warn(DRIVER_NAME ": registering entry point for MM payload failed. Locking SMRAM anyways!\n");
		if(lock_smram(cb_data))
			panic("SMRAM not locked!");

		return -1;
	}

	mdelay(100);

	if(lock_smram(cb_data))
		panic("SMRAM not locked!");

	mdelay(100);

	return 0;
}

static void __exit mm_loader_exit(void)
{
	kfree(cb_data);
	printk(KERN_DEBUG "DONE");
}

module_init(mm_loader_init);
module_exit(mm_loader_exit);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("MM loader - installs Linux-owned SMI handler");
MODULE_LICENSE("GPL v2");
