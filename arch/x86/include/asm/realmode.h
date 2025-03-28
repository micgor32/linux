/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ARCH_X86_REALMODE_H
#define _ARCH_X86_REALMODE_H

/*
 * Flag bit definitions for use with the flags field of the trampoline header
 * in the CONFIG_X86_64 variant.
 */
#define TH_FLAGS_SME_ACTIVE_BIT		0
#define TH_FLAGS_SME_ACTIVE		BIT(TH_FLAGS_SME_ACTIVE_BIT)

#ifndef __ASSEMBLER__

#include <linux/types.h>
#include <asm/io.h>

/* This must match data at realmode/rm/header.S */
struct real_mode_header {
	u32	text_start;
	u32	ro_end;
	/* SMP trampoline */
	u32	trampoline_start;
	u32	trampoline_header;
	u32	smm_trampoline_start; // not sure if we even need it here, we call it from assembly code not C anyways.
	u32	smm_relocation_start;
#ifdef CONFIG_AMD_MEM_ENCRYPT
	u32	sev_es_trampoline_start;
#endif
#ifdef CONFIG_X86_64
	u32	trampoline_start64;
	u32	trampoline_pgd;
#endif
	/* ACPI S3 wakeup */
#ifdef CONFIG_ACPI_SLEEP
	u32	wakeup_start;
	u32	wakeup_header;
#endif
	/* APM/BIOS reboot */
	u32	machine_real_restart_asm;
#ifdef CONFIG_X86_64
	u32	machine_real_restart_seg;
#endif
};

// FIXME: for all SMM stub related structures and vars, bound it with config
/* This must match data at realmode/rm/stub_header.S */
struct stub_header {
	u32	text_start;
	u32	ro_end;
	/* SMP trampoline */
	u32	smm_trampoline_start;
	u32	smm_trampoline_header;
#ifdef CONFIG_AMD_MEM_ENCRYPT
	u32	sev_es_trampoline_start;
#endif
#ifdef CONFIG_X86_64
	u32	smm_trampoline_start64;
	u32	smm_trampoline_pgd;
#endif
	/* ACPI S3 wakeup */
#ifdef CONFIG_ACPI_SLEEP
	u32	wakeup_start;
	u32	wakeup_header;
#endif
	/* APM/BIOS reboot */
	u32	machine_real_restart_asm;
#ifdef CONFIG_X86_64
	u32	machine_real_restart_seg;
#endif
};

/* This must match data at realmode/rm/trampoline_{32,64}.S */
struct trampoline_header {
#ifdef CONFIG_X86_32
	u32 start;
	u16 gdt_pad;
	u16 gdt_limit;
	u32 gdt_base;
#else
	u64 start;
	u64 efer;
	u32 cr4;
	u32 flags;
	u32 lock;
	u64 smm_start;
	u32 stack_size;
	u32 stack_top;
	u16 apic_to_cpu_num[CONFIG_NR_CPUS]; // weeeell as mentioned in smm.h this is not good approach
#endif
};

/* This must match data at realmode/rm/stub.S */
struct stub_trampoline_header {
#ifdef CONFIG_X86_32
	u32 start;
	u16 gdt_pad;
	u16 gdt_limit;
	u32 gdt_base;
#else
	u64 start;
	u64 efer;
	u32 cr4;
	u32 flags;
	u32 lock;
#endif
};

extern struct stub_header *stub_header;
extern unsigned char stub_blob_end[];
extern unsigned char stub_blob[];
extern unsigned char stub_relocs[];

extern struct real_mode_header *real_mode_header;
extern unsigned char real_mode_blob_end[];

extern unsigned long initial_code;
extern unsigned long initial_stack;
extern unsigned long ending_code;
#ifdef CONFIG_AMD_MEM_ENCRYPT
extern unsigned long initial_vc_handler;
#endif

// for now lets reuse it for the stub (?)
extern u32 *trampoline_lock;

extern unsigned char real_mode_blob[];
extern unsigned char real_mode_relocs[];

extern unsigned char smm_relocation_start[];

#ifdef CONFIG_X86_32
extern unsigned char startup_32_smp[];
extern unsigned char boot_gdt[];
// add here the startup for smm fi compiled in 32bit mode
#else
extern unsigned char secondary_startup_64[];
extern unsigned char secondary_startup_64_no_verify[];
extern unsigned char smm_startup_64[];
#endif
extern struct trampoline_header *trampoline_header; // an idea: expose the header so that we can modify the params later in the driver

static inline size_t real_mode_size_needed(void)
{
	if (real_mode_header)
		return 0;	/* already allocated. */

	return ALIGN(real_mode_blob_end - real_mode_blob, PAGE_SIZE);
}

/* I could just extend the function above, but it is also used by */
/* arch/x86/platform/efi/quirks.c so I don't want to mess their logic. */
/* BTW someone should consider defining the functions in appropriate place, */
/* header file is NOT a place for that. */

static inline size_t smm_stub_size_needed(void)
{
	pr_info("we check the stub size");
	if (stub_header)
		return 0;
	pr_info("not assigned yet, the size is %zx", stub_blob_end - stub_blob);

	return stub_blob_end - stub_blob;
}

static inline void set_real_mode_mem(phys_addr_t mem)
{
	real_mode_header = (struct real_mode_header *) __va(mem);
}

static inline void set_smmstub_mem(phys_addr_t mem)
{
	pr_info("header will be allocated at 0x%llu", mem);
	stub_header = (struct stub_header *) __va(mem);
}

void reserve_real_mode(void);
void load_trampoline_pgtable(void);
void init_real_mode(void);

#endif /* __ASSEMBLER__ */

#endif /* _ARCH_X86_REALMODE_H */
