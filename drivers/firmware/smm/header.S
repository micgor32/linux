#include <linux/linkage.h>
#include <asm/page_types.h>
#include <asm/segment.h>

#include "smm.h"

	.section ".header", "a"

	.balign 2
SYM_DATA_START(smm_tr_header)
	.long	md_header
SYM_DATA_END(smm_tr_header)
