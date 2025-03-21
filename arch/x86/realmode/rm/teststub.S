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
SYM_CODE_START(trampoline_start)
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

	# flush prefetch and jump to startup_32
	ljmpl	$__KERNEL32_CS, $pa_startup_32

no_longmode:
	hlt
	jmp no_longmode
SYM_CODE_END(trampoline_start)

#ifdef CONFIG_AMD_MEM_ENCRYPT
/* SEV-ES supports non-zero IP for entry points - no alignment needed. Leave for now, remove later */
SYM_CODE_START(sev_es_trampoline_start)
	cli			# We should be safe anyway

	LJMPW_RM(1f)
1:
	mov	%cs, %ax	# Code and data in the same place
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss

	LOCK_AND_LOAD_REALMODE_ESP

	jmp	.Lswitch_to_protected
SYM_CODE_END(sev_es_trampoline_start)
#endif	/* CONFIG_AMD_MEM_ENCRYPT */

#include "../kernel/verify_cpu.S"

	.section ".text32","ax"
	.code32
	.balign 4
SYM_CODE_START(startup_32)
	movl	%edx, %ss
	addl	$pa_real_mode_base, %esp
	movl	%edx, %ds
	movl	%edx, %es
	movl	%edx, %fs
	movl	%edx, %gs

	/*
	 * Check for memory encryption support. This is a safety net in
	 * case BIOS hasn't done the necessary step of setting the bit in
	 * the MSR for this AP. If SME is active and we've gotten this far
	 * then it is safe for us to set the MSR bit and continue. If we
	 * don't we'll eventually crash trying to execute encrypted
	 * instructions.
	 */
	btl	$TH_FLAGS_SME_ACTIVE_BIT, pa_tr_flags
	jnc	.Ldone
	movl	$MSR_AMD64_SYSCFG, %ecx
	rdmsr
	bts	$MSR_AMD64_SYSCFG_MEM_ENCRYPT_BIT, %eax
	jc	.Ldone

	/*
	 * Memory encryption is enabled but the SME enable bit for this
	 * CPU has has not been set.  It is safe to set it, so do so.
	 */
	wrmsr
.Ldone:

	movl	pa_tr_cr4, %eax
	movl	%eax, %cr4		# Enable PAE mode

	# Setup trampoline 4 level pagetables
	movl	$pa_trampoline_pgd, %eax
	movl	%eax, %cr3

	# Set up EFER
	movl	$MSR_EFER, %ecx
	rdmsr
	/*
	 * Skip writing to EFER if the register already has desired
	 * value (to avoid #VE for the TDX guest).
	 */
	cmp	pa_tr_efer, %eax
	jne	.Lwrite_efer
	cmp	pa_tr_efer + 4, %edx
	je	.Ldone_efer
.Lwrite_efer:
	movl	pa_tr_efer, %eax
	movl	pa_tr_efer + 4, %edx
	wrmsr

.Ldone_efer:
	# Enable paging and in turn activate Long Mode.
	movl	$CR0_STATE, %eax
	movl	%eax, %cr0

	/*
	 * At this point we're in long mode but in 32bit compatibility mode
	 * with EFER.LME = 1, CS.L = 0, CS.D = 1 (and in turn
	 * EFER.LMA = 1). Now we want to jump in 64bit mode, to do that we use
	 * the new gdt/idt that has __KERNEL_CS with CS.L = 1.
	 */
	ljmpl	$__KERNEL_CS, $pa_startup_64
SYM_CODE_END(startup_32)

SYM_CODE_START(pa_trampoline_compat)
	/*
	 * In compatibility mode.  Prep ESP and DX for startup_32, then disable
	 * paging and complete the switch to legacy 32-bit mode.
	 */
	LOCK_AND_LOAD_REALMODE_ESP lock_pa=1
	movw	$__KERNEL_DS, %dx

	movl	$(CR0_STATE & ~X86_CR0_PG), %eax
	movl	%eax, %cr0
	ljmpl   $__KERNEL32_CS, $pa_startup_32
SYM_CODE_END(pa_trampoline_compat)





	.balign 4
SYM_DATA_START(smm_relocate_gdt)
	.short	smm_relocate_gdt_end - smm_relocate_gdt - 1	# gdt limit
	.long	pa_smm_relocate_gdt
	.short	0
	.quad	0x00cf9b000000ffff	# __KERNEL32_CS
	.quad	0x00af9b000000ffff	# __KERNEL_CS
	.quad	0x00cf93000000ffff	# __KERNEL_DS
SYM_DATA_END_LABEL(smm_relocate_gdt, SYM_L_LOCAL, smm_relocate_gdt_end)

SYM_DATA_START(smm_relocate_gdt64)
	.short	smm_relocate_gdt_end - smm_relocate_gdt - 1	# gdt limit
	.long	pa_smm_relocate_gdt
	.long	0
SYM_DATA_END(smm_relocate_gdt64)

	.balign 8
SYM_DATA_START(stub_header)
	SYM_DATA(stack_size,		.space 8)
	SYM_DATA(stack_top,		.space 8)
	SYM_DATA(c_handler,		.space 4)
SYM_DATA_END(stub_header)

