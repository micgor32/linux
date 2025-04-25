/* SPDX-License-Identifier: GPL-2.0 */
/*
 *
 *	Trampoline.S	Derived from Setup.S by Linus Torvalds
 *
 *	4 Jan 1997 Michael Chastain: changed to gnu as.
 *	15 Sept 2005 Eric Biederman: 64bit PIC support
 *
 *	The purpose of this code is to get us from whatever mode
 *	coreboot leaves us in when entering SMM, to the kernel space.
 *	
 *	Derived from trampoline_$(BITS).S, shrinked to the parts that 
 *	are needed once in SMM (for e.g. we are at least in protected mode
 *	at this point). The instructions from mm_startup_32 on are placed
 *	by drivers/firmware/smm/mm_loader.c under designated SMRAM region.
 *	
 *	Also see the general comment in trampoline_64.S.
 */

#include <linux/linkage.h>
#include <asm/pgtable_types.h>
#include <asm/page_types.h>
#include <asm/msr.h>
#include <asm/segment.h>
#include <asm/processor-flags.h>
#include <asm/realmode.h>

#include "realmode.h"

	.text
	.code16

.macro LOCK_AND_LOAD_REALMODE_ESP lock_pa=0 lock_rip=0
	/*
	 * Make sure only one CPU fiddles with the realmode stack
	 */
.Llock_rm\@:
	.if \lock_pa
        lock btsl       $0, pa_tr_lock
	.elseif \lock_rip
        lock btsl       $0, tr_lock(%rip)
	.else
        lock btsl       $0, tr_lock
	.endif
        jnc             2f
        pause
        jmp             .Llock_rm\@
2:
	# Setup stack
	movl	$rm_stack_end, %esp
.endm

	.balign	PAGE_SIZE
SYM_CODE_START(mm_trampoline_start)
	// we are already in protected mode at this point (assuming cb was compiled for 32bit),
	// but we need to get to kernel address space
	mov	$0x3f8, %dx
	mov	$'H', %al
	out	%al, (%dx)
	mov	$'\n', %al
	out	%al, (%dx)
	ret

	cli			# We should be safe anyway
	wbinvd

	LJMPW_RM(1f)
1:
	mov	%cs, %ax	# Code and data in the same place
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss

	LOCK_AND_LOAD_REALMODE_ESP

	call	verify_cpu		# Verify the cpu supports long mode
	testl   %eax, %eax		# Check for return code
	jnz	no_longmode


.Lswitch_to_protected:
	/*
	 * GDT tables in non default location kernel can be beyond 16MB and
	 * lgdt will not be able to load the address as in real mode default
	 * operand size is 16bit. Use lgdtl instead to force operand size
	 * to 32 bit.
	 */

	lidtl	tr_idt	# load idt with 0, 0
	lgdtl	tr_gdt	# load gdt with whatever is appropriate

	movw	$__KERNEL_DS, %dx	# Data segment descriptor

	# Enable protected mode
	movl	$(CR0_STATE & ~X86_CR0_PG), %eax
	movl	%eax, %cr0		# into protected mode

	ret
	# flush prefetch and jump to startup_32
	ljmpl	$__KERNEL32_CS, $pa_startup_32

no_longmode:
	hlt
	jmp no_longmode
SYM_CODE_END(mm_trampoline_start)

#include "../kernel/verify_cpu.S"

	.section ".text32","ax"
	.code32
	.balign 4
SYM_CODE_START(mm_startup_32)
	mov	$0x3f8, %dx
	mov	$'H', %al
	out	%al, (%dx)
	mov	$'e', %al
	out	%al, (%dx)
	mov	$'l', %al
	out	%al, (%dx)
	mov	$'l', %al
	out	%al, (%dx)
	mov	$'0', %al
	out	%al, (%dx)
	mov	$' ', %al
	out	%al, (%dx)
	mov	$'f', %al
	out	%al, (%dx)
	mov	$'r', %al
	out	%al, (%dx)
	mov	$'o', %al
	out	%al, (%dx)
	mov	$'m', %al
	out	%al, (%dx)
	mov	$' ', %al
	out	%al, (%dx)
	mov	$'L', %al
	out	%al, (%dx)
	mov	$'i', %al
	out	%al, (%dx)
	mov	$'n', %al
	out	%al, (%dx)
	mov	$'u', %al
	out	%al, (%dx)
	mov	$'x', %al
	out	%al, (%dx)
	mov	$'\n', %al
	out	%al, (%dx)

	/* 
	 * For now, we return back to the coreboot's handler here.
	 * TODO: preserve registers, and bring the CPU to the state
	 * where the kernel address space is callable
	 */
	ret

	// 0x80050033

	lidtl	tr_idt	# load idt with 0, 0
	lgdtl	tr_smm_gdt	# load gdt with whatever is appropriate

	movw	$__KERNEL_DS, %dx	# Data segment descriptor

	# Enable protected mode
	movl	$(CR0_STATE & ~X86_CR0_PG), %eax
	movl	%eax, %cr0		# into protected mode


	movl	0x4010, %eax
	movl	%eax, %cr4

	movl	$0x3000, %eax
	movl	%eax, %cr3
	movl	$MSR_EFER, %ecx

	//movl $0x80050033, %eax	
	//movl %eax, %cr0

	/*
	 * At this point we're in long mode but in 32bit compatibility mode
	 * with EFER.LME = 1, CS.L = 0, CS.D = 1 (and in turn
	 * EFER.LMA = 1). Now we want to jump in 64bit mode, to do that we use
	 * the new gdt/idt that has __KERNEL_CS with CS.L = 1.
	 */

	ljmpl	$__KERNEL_CS, $pa_mm_startup_64
SYM_CODE_END(mm_startup_32)

SYM_CODE_START(pa_mm_trampoline_compat)
	// cleanup for coreboot
	pop	%esp
	pop	%ebp
	pop	%ebx
	pop	%esi
	pop	%edi
	pop	%eax
	
	movl	%ebx, %cr4
	movl	%eax, %cr3


	/*
	 * In compatibility mode.  Prep ESP and DX for startup_32, then disable
	 * paging and complete the switch to legacy 32-bit mode.
	 */
	//LOCK_AND_LOAD_REALMODE_ESP lock_pa=1
//	movw	$__KERNEL_DS, %dx

//	movl	$(CR0_STATE & ~X86_CR0_PG), %eax
//	movl	%eax, %cr0
//	ljmpl   $__KERNEL32_CS, $pa_mm_startup_32
SYM_CODE_END(pa_mm_trampoline_compat)

	.section ".text64","ax"
	.code64
	.balign 4
SYM_CODE_START(mm_startup_64)
	jmpq *tr_smm_start(%rip)
SYM_CODE_END(mm_startup_64)

/*
 * We start here if coreboot is compiled in 64-bit mode.
 * Similarly as in mm_startup_32, for now we just return to
 * the coreboot after acknowledging that we are here.
 */
SYM_CODE_START(mm_trampoline_start64)
	mov	$0x3f8, %dx
	mov	$'H', %al
	out	%al, (%dx)
	mov	$'e', %al
	out	%al, (%dx)
	mov	$'l', %al
	out	%al, (%dx)
	mov	$'l', %al
	out	%al, (%dx)
	mov	$'0', %al
	out	%al, (%dx)
	mov	$' ', %al
	out	%al, (%dx)
	mov	$'f', %al
	out	%al, (%dx)
	mov	$'r', %al
	out	%al, (%dx)
	mov	$'o', %al
	out	%al, (%dx)
	mov	$'m', %al
	out	%al, (%dx)
	mov	$' ', %al
	out	%al, (%dx)
	mov	$'L', %al
	out	%al, (%dx)
	mov	$'i', %al
	out	%al, (%dx)
	mov	$'n', %al
	out	%al, (%dx)
	mov	$'u', %al
	out	%al, (%dx)
	mov	$'x', %al
	out	%al, (%dx)
	mov	$'6', %al
	out	%al, (%dx)
	mov	$'4', %al
	out	%al, (%dx)
	mov	$'\n', %al
	out	%al, (%dx)

	ret
	/*
	 * APs start here on a direct transfer from 64-bit BIOS with identity
	 * mapped page tables.  Load the kernel's GDT in order to gear down to
	 * 32-bit mode (to handle 4-level vs. 5-level paging), and to (re)load
	 * segment registers.  Load the zero IDT so any fault triggers a
	 * shutdown instead of jumping back into BIOS.
	 */
	lidt	tr_idt(%rip)
	lgdt	tr_smm_gdt64(%rip)

	/* Check if paging mode has to be changed */
	movq	%cr4, %rax
	xorl	tr_smm_cr4(%rip), %eax
	testl	$X86_CR4_LA57, %eax
	jnz	.L_switch_paging

	/* Paging mode is correct proceed in 64-bit mode */

	LOCK_AND_LOAD_REALMODE_ESP lock_rip=1

	movw	$__KERNEL_DS, %dx
	movl	%edx, %ss
	addl	$pa_real_mode_base, %esp
	movl	%edx, %ds
	movl	%edx, %es
	movl	%edx, %fs
	movl	%edx, %gs

	movl	$pa_mm_trampoline_pgd, %eax
	movq	%rax, %cr3

	pushq	$__KERNEL_CS
	pushq	tr_smm_start(%rip)
	lretq
.L_switch_paging:
	/*
	 * To switch between 4- and 5-level paging modes, it is necessary
	 * to disable paging. This must be done in the compatibility mode.
	 */
	ljmpl	*tr_smm_compat(%rip)
SYM_CODE_END(mm_trampoline_start64)

	.section ".rodata","a"
	# Duplicate the global descriptor table
	# so the kernel can live anywhere
	.balign	16
SYM_DATA_START(tr_smm_gdt)
	.short	tr_smm_gdt_end - tr_smm_gdt - 1	# gdt limit
	.long	pa_tr_smm_gdt
	.short	0
	.quad	0x00cf9b000000ffff	# __KERNEL32_CS
	.quad	0x00af9b000000ffff	# __KERNEL_CS
	.quad	0x00cf93000000ffff	# __KERNEL_DS
SYM_DATA_END_LABEL(tr_smm_gdt, SYM_L_LOCAL, tr_smm_gdt_end)

SYM_DATA_START(tr_smm_gdt64)
	.short	tr_smm_gdt_end - tr_smm_gdt - 1	# gdt limit
	.long	pa_tr_smm_gdt
	.long	0
SYM_DATA_END(tr_smm_gdt64)

SYM_DATA_START(tr_smm_compat)
	.long	pa_mm_trampoline_compat
	.short	__KERNEL32_CS
SYM_DATA_END(tr_smm_compat)

	.bss
	.balign	PAGE_SIZE
SYM_DATA(mm_trampoline_pgd, .space PAGE_SIZE)

	.balign	8
SYM_DATA_START(mm_trampoline_header)
	SYM_DATA_LOCAL(tr_smm_start,	.space 8)
	SYM_DATA(tr_smm_efer,		.space 8)
	SYM_DATA(tr_smm_cr4,		.space 4)
	SYM_DATA(tr_smm_flags,		.space 4)
	SYM_DATA(tr_smm_lock,		.space 4)
	SYM_DATA(tr_smm_stack_size,		.space 4)
	SYM_DATA(tr_smm_stack_top,		.space 4)
	SYM_DATA(tr_smm_apic_to_cpu_num,	.space 2)
SYM_DATA_END(mm_trampoline_header)
	
#include "trampoline_common.S"
