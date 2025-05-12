/* SPDX-License-Identifier: GPL-2.0 */
/*
 *
 *	Trampoline.S	Derived from Setup.S by Linus Torvalds
 *
 *	4 Jan 1997 Michael Chastain: changed to gnu as.
 *	15 Sept 2005 Eric Biederman: 64bit PIC support
 *
 *	The purpose of this code is to get us from whatever mode
 *	coreboot leaves us in when entering SMM, to the MM payload
	handler.
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

	.section ".text32","ax"
	.code32
	.balign 4
SYM_CODE_START(mm_startup_32)
	// remove later and write to the serial from C code first
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
	// till here

	// Normally the verify_cpu.S does multiple checks, we just do one simple check
	movl    $0x80000000,%eax	# See if extended cpuid is implemented
	cpuid
	cmpl    $0x80000001,%eax
	jb      no_longmode

	// ebx, esi, edi and ebp are going to be preserved (see comment in smm_stub.S if target is x86_64)
	push	%ebx
	push	%esi
	push	%edi
	push	%ebp

	// preserve cr4 and enable PAE mode	
	movl	%cr4, %eax
	push	%eax
	movl	pa_tr_cr4, %eax
	movl	%eax, %cr4
	
	// preserve cr3 and setup 4 level page tables
	movl	%cr3, %ebx
	push	%ebx
	movl	$pa_trampoline_pgd, %ebx
	movl	%ebx, %cr3

	// clear XD bit
	movl	$MSR_IA32_MISC_ENABLE, %ecx
	rdmsr
	btrl	$2, %edx		# clear MSR_IA32_MISC_ENABLE_XD_DISABLE
	wrmsr

	// efer setup
	movl	pa_tr_efer, %eax
	movl	pa_tr_efer + 4, %edx
	movl	$MSR_EFER, %ecx
	wrmsr

	movl	%cr0, %eax
	push	%eax

	// If uncommented, will just skip enabling longmode and jump to "compat" mode which will restore pre-trampoline state/
	// we have to jump there anyways, but that should be done from mm_startup_64 as the last thig in the longmode.
	//ljmpl	$0x08, $pa_mm_compat

	// So for now this hangs on the long jump.
	movl	0x80050032, %eax
	movl	%eax, %cr0
	
	mov	$0x3f8, %dx
	mov	$'W', %al
	out	%al, (%dx)

	ljmpl	$0x18, $pa_mm_startup_64

no_longmode: // Assuming that kernel was compiled in 64bit, and we get here, there is nothing left to do.
	ret
SYM_CODE_END(mm_startup_32)

SYM_CODE_START(mm_compat)
	// Restore cr4, cr3 adn cr0 to whatever state it was when exiting coreboot. Essentially a compatibility mode
	mov	$0x3f8, %dx
	mov	$'C', %al
	out	%al, (%dx)

	pop	%eax
	movl	%eax, %cr0

	pop	%ebx
	movl	%ebx, %cr3

	pop	%eax
	movl	%eax, %cr3

	// restore remaining registers
	pop %ebp
	pop %edi
	pop %esi
	pop %ebx

	ret

SYM_CODE_END(mm_compat)

	.section ".text64","ax"
	.code64
	.balign 4
SYM_CODE_START(mm_startup_64)
	mov	$0x3f8, %dx
	mov	$'L', %al
	out	%al, (%dx)

	movw $__KERNEL_DS, %dx

	//movl	$(CR0_STATE & ~X86_CR0_PG), %eax
	//movl	%eax, %cr0
	ljmpl   *tr_mm_compat(%rip)

	//jmpq *tr_smm_start(%rip)
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

SYM_CODE_END(mm_trampoline_start64)

SYM_DATA_START(tr_mm_compat)
	.long	pa_mm_compat
	.short	__KERNEL32_CS
SYM_DATA_END(tr_mm_compat)

	.balign	8
SYM_DATA_START(mm_trampoline_header)
	SYM_DATA_LOCAL(tr_smm_start,	.space 8)
SYM_DATA_END(mm_trampoline_header)
