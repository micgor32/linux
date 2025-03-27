// SPDX-License-Identifier: GPL-2.0
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/memblock.h>
#include <linux/cc_platform.h>
#include <linux/pgtable.h>

// for testing only
#include <linux/kernel.h>
// end

#include <asm/set_memory.h>
#include <asm/realmode.h>
#include <asm/tlbflush.h>
#include <asm/crash.h>
#include <asm/sev.h>

struct real_mode_header *real_mode_header;
struct stub_header *stub_header;
u32 *trampoline_cr4_features;
u32 *smm_trampoline_cr4_features; 

/* Hold the pgd entry used on booting additional CPUs */
pgd_t trampoline_pgd_entry;
// useless, but for now we just replicated the trampoline 
pgd_t smm_pgd_entry;
//int smm(size_t); was for testing only

void load_trampoline_pgtable(void)
{
#ifdef CONFIG_X86_32
	load_cr3(initial_page_table);
#else
	/*
	 * This function is called before exiting to real-mode and that will
	 * fail with CR4.PCIDE still set.
	 */
	if (boot_cpu_has(X86_FEATURE_PCID))
		cr4_clear_bits(X86_CR4_PCIDE);

	write_cr3(real_mode_header->trampoline_pgd);
#endif

	/*
	 * The CR3 write above will not flush global TLB entries.
	 * Stale, global entries from previous page tables may still be
	 * present.  Flush those stale entries.
	 *
	 * This ensures that memory accessed while running with
	 * trampoline_pgd is *actually* mapped into trampoline_pgd.
	 */
	__flush_tlb_all();
}

void __init reserve_real_mode(void)
{
	phys_addr_t mem, smmem;
	size_t size = real_mode_size_needed();
	size_t smsize = smm_stub_size_needed();

	if (!size) {
		pr_info("no size for rm trampoline\n");
		return;
	}

	pr_info("size needed for rm trampoline and stuff %zx\n", size);

	if (!smsize)
		pr_info("no size for smmstub\n");

	WARN_ON(slab_is_available());

	/* Has to be under 1M so we can execute real-mode AP code. */
	mem = memblock_phys_alloc_range(size, PAGE_SIZE, 0, 1<<20);
	pr_info("mem for real mode 0x%llu", mem);
	if (!mem)
		pr_info("No sub-1M memory is available for the trampoline\n");
	else
		set_real_mode_mem(mem);

	// lets see whether this works
	//set_smmstub_mem(0x38000);
	
	/*smmem = memblock_find_in_range(0x30000, 0x30000+65536, 65536, 65536);*/
	/*memblock_reserve(smmem, 65536);*/
	/*set_smmstub_mem(smmem);*/

	// lets see if we can reserve the space in default SMBASE
	/*phys_addr_t smbase = 0x38000;*/
	/*phys_addr_t end = 0x3FFFF;*/
	/*smmem = memblock_phys_alloc_range(smsize, PAGE_SIZE, 1<<20, 1<<21);*/
	/*pr_info("mem for stub header 0x%llu", smmem);*/
	/*if (!smmem)*/
	/*	pr_info("No sub-1M memory is available for the SMM trampoline\n");*/
	/*else*/
	/*	set_smmstub_mem(smmem);*/

	/*
	 * Unconditionally reserve the entire first 1M, see comment in
	 * setup_arch().
	 */
	memblock_reserve(0, SZ_1M);
}

static void __init sme_sev_setup_real_mode(struct trampoline_header *th)
{
#ifdef CONFIG_AMD_MEM_ENCRYPT
	if (cc_platform_has(CC_ATTR_HOST_MEM_ENCRYPT))
		th->flags |= TH_FLAGS_SME_ACTIVE;

	if (cc_platform_has(CC_ATTR_GUEST_STATE_ENCRYPT)) {
		/*
		 * Skip the call to verify_cpu() in secondary_startup_64 as it
		 * will cause #VC exceptions when the AP can't handle them yet.
		 */
		th->start = (u64) secondary_startup_64_no_verify;

		if (sev_es_setup_ap_jump_table(real_mode_header))
			panic("Failed to get/update SEV-ES AP Jump Table");
	}
#endif
}

void smm_test(void)
{
	pr_info("test\n");
	early_printk("wont work anyways\n");
}

void notrace test_p(void)
{
	asm volatile (
        "movw $0x3f8, %%dx\n\t"
        "movb $'h', %%al\n\t"
        "outb %%al, %%dx\n\t"
        "movb $'e', %%al\n\t"
        "outb %%al, %%dx\n\t"
        "movb $'l', %%al\n\t"
        "outb %%al, %%dx\n\t"
        "movb $'l', %%al\n\t"
        "outb %%al, %%dx\n\t"
        "movb $'o', %%al\n\t"
        "outb %%al, %%dx\n\t"
        "movb $'\n', %%al\n\t"
        "outb %%al, %%dx\n\t"
        :
        :
        : "al", "dx"
	);
}

static void __init setup_real_mode(void)
{
	u16 real_mode_seg;
	const u32 *rel;
	u32 count;
	unsigned char *base;
	unsigned long phys_base;
	struct trampoline_header *trampoline_header;
	size_t size = PAGE_ALIGN(real_mode_blob_end - real_mode_blob);
#ifdef CONFIG_X86_64
	u64 *trampoline_pgd;
	u64 efer;
	int i;
#endif

	base = (unsigned char *)real_mode_header;

	/*
	 * If SME is active, the trampoline area will need to be in
	 * decrypted memory in order to bring up other processors
	 * successfully. This is not needed for SEV.
	 */
	if (cc_platform_has(CC_ATTR_HOST_MEM_ENCRYPT))
		set_memory_decrypted((unsigned long)base, size >> PAGE_SHIFT);

	memcpy(base, real_mode_blob, size);

	phys_base = __pa(base);
	real_mode_seg = phys_base >> 4;

	rel = (u32 *) real_mode_relocs;

	/* 16-bit segment relocations. */
	count = *rel++;
	while (count--) {
		u16 *seg = (u16 *) (base + *rel++);
		*seg = real_mode_seg;
	}

	/* 32-bit linear relocations. */
	count = *rel++;
	while (count--) {
		u32 *ptr = (u32 *) (base + *rel++);
		*ptr += phys_base;
	}

	/* Must be performed *after* relocation. */
	trampoline_header = (struct trampoline_header *)
		__va(real_mode_header->trampoline_header);
	// copying reloc code to smbase
	const uintptr_t location = 0x38000;
	void *v;
	v = phys_to_virt(location);
	memcpy(v, __va(real_mode_header->smm_trampoline_start), 175);

	/*u32 blob;*/
	/*blob = real_mode_header->smm_relocation_start;*/
	/**/
	//printk("address of the relocation start is 0x%\n", (unsigned int)&smm_relocation_start);

	/*void __iomem *addr = ioremap((resource_size_t)location, 47); // hardcode size, beautiful*/
	/*memcpy_toio(addr, &blob, 47);*/
	/*wbinvd();*/
	test_p();
	printk("reloc code should be under 0x38000 (I think), the address is 0x%lx\n", (unsigned long)__va(real_mode_header->smm_relocation_start));

#ifdef CONFIG_X86_32
	trampoline_header->start = __pa_symbol(startup_32_smp);
	trampoline_header->gdt_limit = __BOOT_DS + 7;
	trampoline_header->gdt_base = __pa_symbol(boot_gdt);
#else
	/*
	 * Some AMD processors will #GP(0) if EFER.LMA is set in WRMSR
	 * so we need to mask it out.
	 */
	rdmsrl(MSR_EFER, efer);
	trampoline_header->efer = efer & ~EFER_LMA;

	trampoline_header->start = (u64) secondary_startup_64;
	trampoline_header->smm_start = (u64) test_p; //smm_startup_64;
	trampoline_cr4_features = &trampoline_header->cr4;
	*trampoline_cr4_features = mmu_cr4_features;

	trampoline_header->flags = 0;

	trampoline_lock = &trampoline_header->lock;
	*trampoline_lock = 0;

	trampoline_pgd = (u64 *) __va(real_mode_header->trampoline_pgd);

	/* Map the real mode stub as virtual == physical */
	trampoline_pgd[0] = trampoline_pgd_entry.pgd;

	/*
	 * Include the entirety of the kernel mapping into the trampoline
	 * PGD.  This way, all mappings present in the normal kernel page
	 * tables are usable while running on trampoline_pgd.
	 */
	for (i = pgd_index(__PAGE_OFFSET); i < PTRS_PER_PGD; i++)
		trampoline_pgd[i] = init_top_pgt[i].pgd;
#endif

	sme_sev_setup_real_mode(trampoline_header);
	// FIXME: bound EVERYHTING related to SMM with a corresponding config.
	// While if we run on different firmware (regardless if the kernel is supposed to be LB payload or not) this code just won"t work,
	// I have no clue what potential errors would this trigger. Safest assumption would be that the memcopy ops would obviously fail (since SMRAM is locked),
	// and the rest of the code would just take no effect.
	//#ifdef CONFIG_SMM
	// Not sure whether we need to do segment relocs, lets see
/*	u16 stub_seg;*/
/*	const u32 *rel_stub;*/
/*	u32 count_stub;*/
/*	//unsigned char *base;*/
/*	//unsigned long phys_base;*/
/*	struct stub_trampoline_header *stub_trampoline_header;*/
/*	// see whether PAGE_ALIGN is needed, I don"t think so though, 16 here is */
/*	// mysterious for me, see what for we add it*/
/*	size_t stub_size = stub_blob_end - stub_blob;*/
/*#ifdef CONFIG_X86_64*/
/*	u64 *smm_trampoline_pgd;*/
/*	u64 smm_efer;*/
/*	int y;*/
/*#endif*/
	/*const uintptr_t location = 0x38000;*/
	/*// now we put the stub in smbase, hardoded for now, see whether we can pass params that early */
	/*void __iomem *addr = ioremap((resource_size_t)location, stub_size);*/
	/**/
	/*memcpy_toio(addr, stub_blob, stub_size);*/
	/*wbinvd(); // again check if needed*/
	/*pr_info("we've got through copying blob to smbase\n");*/
	/**/
	/**/
	/*// we can hardcode this for now (i think)*/
	/*stub_seg = location >> 4;*/
	/*rel_stub = (u32 *) stub_relocs;*/
	/**/
	/*/* 16-bit segment relocations. */
	/*count_stub = *rel_stub++;*/
	/*while (count_stub--) {*/
	/*	u16 *seg = (u16 *) (addr + *rel_stub++);*/
	/*	*seg = stub_seg;*/
	/*}*/
	/**/
	/*/* 32-bit linear relocations. */
	/*count_stub = *rel_stub++;*/
	/*while (count_stub--) {*/
	/*	u32 *ptr = (u32 *) (addr + *rel_stub++);*/
	/*	*ptr += location;*/
	/*}*/
	/**/
	/*pr_info("first potential null dereference, but hey relocs are done (hopefully)\n");*/
	/*stub_trampoline_header = (struct stub_trampoline_header *)*/
	/*	__va(stub_header->smm_trampoline_header);*/
	/**/
 /* We skip the case with AMD_MEM_ENCTYPT, not needed for now.*/
 /* Technically we could skipp the case with 32bit config, wont be executed anyways during testing (LB doesnt work on a testing board when compiled in 32bit)*/
 /* This whole section will probably be stripped down anyways, for eg. we do not need kernel mappings in PGD (at least I dont see the use for it rn).*/
 /* The whole purpose of rewriting this section is to setup the (another) trampoline to jump not to the kernel startup code but to our relocation handler.*/
 /* There could be a more elegant way to do so, but there are couple of problems: */
 /*  - trampoline code has to be placed at SMBASE, meanwhile the trapoline for the kernel does not have such requirement, as long as it is in the lowest 1MB. This forces us to write new trampoline*/
 /*    basically and place it there. One consideration could be, does it have to be done so early in the boot? The whole logic here is to reserve low memory */
 /*    early enough so that it is available (if I understood correctly), but, SMRAM address space won't be allocated anyways, so (for later), we could just*/
 /*    move all this code to the later stage - so as a driver as planned initially - we know that copying that code to SMBASE from (loadable)module also works.*/
 /*  - not sure whether we can have conditional statement within the trampoline, even if (based on config or so), it would be a mess (even more than it is now lol).*/
 /* Rest of the assigned data is same as for the normal trampoline. TODO: could be that we need SoC specific definitions, for that we are back at the CBTABLE parsing,*/
 /* which is problematic if we run this code here - realmode code runs waaaaay before any drivers.*/
/*#ifdef CONFIG_X86_32*/
/*	trampoline_header->start = __pa_symbol(startup_32_smp);*/
/*	trampoline_header->gdt_limit = __BOOT_DS + 7;*/
/*	trampoline_header->gdt_base = __pa_symbol(boot_gdt);*/
/*#else*/
	/*
	 * Some AMD processors will #GP(0) if EFER.LMA is set in WRMSR
	 * so we need to mask it out.
	 */
	/*rdmsrl(MSR_EFER, smm_efer);*/
	/*stub_trampoline_header->efer = efer & ~EFER_LMA;*/
	/**/
	/*stub_trampoline_header->start = (u64) smm_test;*/
	/*smm_trampoline_cr4_features = &stub_trampoline_header->cr4;*/
	/*// hmm, it seems this is needed for hibernation? Not sure so lets leave it as it is.*/
	/**smm_trampoline_cr4_features = mmu_cr4_features;*/
	/**/
	/*stub_trampoline_header->flags = 0;*/
	/**/
	/*// commented out, seems we do not need it (?)*/
	/*//trampoline_lock = &trampoline_header->lock;*/
	/*/trampoline_lock = 0;*/
	/**/
	/*smm_trampoline_pgd = (u64 *) __va(stub_header->smm_trampoline_pgd);*/
	/**/
	/*/* Map the real mode stub as virtual == physical */
	/*smm_trampoline_pgd[0] = smm_pgd_entry.pgd;*/
	/**/
	/*
	 * Include the entirety of the kernel mapping into the trampoline
	 * PGD.  This way, all mappings present in the normal kernel page
	 * tables are usable while running on trampoline_pgd.
	 */
	/*for (y = pgd_index(__PAGE_OFFSET); i < PTRS_PER_PGD; i++)*/
	/*	trampoline_pgd[i] = init_top_pgt[i].pgd;*/
//#endif


//#endif
	// Was for testing before
	/*size_t test = 16; // + (real_mode_header->smm_test_end - real_mode_header->smm_test);*/
	/*smm(test);*/
}

/*
 * reserve_real_mode() gets called very early, to guarantee the
 * availability of low memory. This is before the proper kernel page
 * tables are set up, so we cannot set page permissions in that
 * function. Also trampoline code will be executed by APs so we
 * need to mark it executable at do_pre_smp_initcalls() at least,
 * thus run it as a early_initcall().
 */
static void __init set_real_mode_permissions(void)
{
	unsigned char *base = (unsigned char *) real_mode_header;
	size_t size = PAGE_ALIGN(real_mode_blob_end - real_mode_blob);

	size_t ro_size =
		PAGE_ALIGN(real_mode_header->ro_end) -
		__pa(base);

	size_t text_size =
		PAGE_ALIGN(real_mode_header->ro_end) -
		real_mode_header->text_start;

	unsigned long text_start =
		(unsigned long) __va(real_mode_header->text_start);

	set_memory_nx((unsigned long) base, size >> PAGE_SHIFT);
	set_memory_ro((unsigned long) base, ro_size >> PAGE_SHIFT);
	set_memory_x((unsigned long) text_start, text_size >> PAGE_SHIFT);
}

void __init init_real_mode(void)
{
	if (!real_mode_header)
		panic("Real mode trampoline was not allocated");
	/*if (!stub_header)*/
	/*	panic("SMM stub trampoline was not allocated");*/

	setup_real_mode();
	set_real_mode_permissions();
}

static int __init do_init_real_mode(void)
{
	x86_platform.realmode_init();
	return 0;
}
early_initcall(do_init_real_mode);
