/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * The stub is a generic wrapper for bootstrapping a C-based SMM handler. Its
 * primary purpose is to put the CPU into protected mode with a stack and call
 * into the C handler.
 *
 */

#include <linux/linkage.h>
#include <asm/pgtable_types.h>
#include <asm/page_types.h>
#include <asm/msr.h>
#include <asm/segment.h>
#include <asm/processor-flags.h>
#include <asm/realmode.h>

// This is bad solution to import it all (since we do not need most of it) but lets not care about that for now 
#include "../../../arch/x86/realmode/rm/trampoline_64.S"
#include "../../../arch/x86/realmode/rm/header.S"
#include "../../../arch/x86/realmode/rm/stack.S"
#include "../../../arch/x86/realmode/rm/reboot.S"
#include "../../../arch/x86/realmode/rm/pasyms.h"

.code16
SYM_CODE_START(smm_relocation_start)
	mov	$0x3f8, %dx
	mov	$'C', %al
	out	%al, (%dx)
	mov	$'r', %al
	out	%al, (%dx)
	mov	$'a', %al
	out	%al, (%dx)
	mov	$'s', %al
	out	%al, (%dx)
	mov	$'h', %al
	out	%al, (%dx)
	jmp	smm_trampoline_start

	rsm

SYM_CODE_END(smm_relocation_start)
