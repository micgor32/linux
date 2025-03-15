/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * The stub is a generic wrapper for bootstrapping a C-based SMM handler. Its
 * primary purpose is to put the CPU into protected mode with a stack and call
 * into the C handler.
 *
 */

.global smm_relocation_start
.global smm_relocation_end

.code16
smm_relocation_start:
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

	rsm

smm_relocation_end:
