/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Adapted from rmpiggy.S 
 *
 * Wrapper script for the realmode binary as a transport object
 * before copying to low memory.
 */
#include <linux/linkage.h>
#include <asm/page_types.h>


	.section ".init.data","aw"

	.balign PAGE_SIZE

SYM_DATA_START(stub_blob)
	.incbin	"trampoline.bin"
SYM_DATA_END_LABEL(stub_blob, SYM_L_GLOBAL, stub_blob_end)
