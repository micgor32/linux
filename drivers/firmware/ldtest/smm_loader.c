// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for installing SMI handler and locking down SMM
 *
 * Copyright (c) 2025 9elements GmbH
 *
 * Author: Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <asm/apic.h>
#include <linux/spinlock.h>
#include <linux/smp.h>
#include <asm/realmode.h>
#include <asm/io.h>
#include <asm/irq_vectors.h>
#include <linux/delay.h>

#define	LAPIC_INT_ASSERT 0x04000
#define	LAPIC_DM_SMI 0x00200

// testing

#define APM_CNT		0xb2

#define PM1_STS		0x00
#define   PWRBTN_STS	(1 <<  8)
#define   RTC_STS	(1 << 10)
#define PM1_EN		0x02
#define   PWRBTN_EN	(1 <<  8)
#define   GBL_EN	(1 <<  5)
#define PM1_CNT		0x04
#define   SCI_EN	(1 << 0)
#define PM_LV2		0x14
#define PM_LV3		0x15
#define PM_LV4		0x16
#define PM_LV5		0x17
#define PM_LV6		0x18
#define GPE0_STS	0x20
#define SMI_EN		0x30
#define   PERIODIC_EN	(1 << 14)
#define   TCO_EN	(1 << 13)
#define   APMC_EN	(1 <<  5)
#define   BIOS_EN	(1 <<  2)
#define   EOS		(1 <<  1)
#define   GBL_SMI_EN	(1 <<  0)
#define SMI_STS		0x34
#define ALT_GP_SMI_EN	0x38
#define ALT_GP_SMI_STS	0x3a
#define DEFAULT_PMBASE		0x00000600


// end

//static DEFINE_SPINLOCK(reloc_lock);
//
static void smi(int cpu, bool back)
{
	int y = 1000;
	const bool x2apic = boot_cpu_has(X86_FEATURE_X2APIC);

	apic->send_IPI(cpu, LAPIC_INT_ASSERT | LAPIC_DM_SMI);

	while (x2apic && y--) {
		cpu_relax();
	}
	
	back = true;
}

static void ipi(void *inf) 
{
	int cpu = smp_processor_id();
	pr_info("this is: %d", cpu);
	printk(KERN_INFO "testssttst\n");
	apic->send_IPI(cpu, LAPIC_INT_ASSERT | LAPIC_DM_SMI);
}

static cpumask_var_t reloc_cpumask;

static void initiate_relocation(void)
{
	//printk(KERN_INFO "boot_cpu is %d", get_boot_cpu_id());
	//on_each_cpu(ipi, NULL, 1);
	//on_each_cpu_cond_mask(NULL, ipi, NULL, true, cpu_online_mask); 
	//apic->send_IPI(0, LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	//apic->send_IPI(smp_processor_id(), LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	/*int cpu;*/
	/*unsigned long start;*/
	/*cpus_read_lock();*/
	/*cpumask_copy(reloc_cpumask, cpu_online_mask);*/
	/*cpumask_clear_cpu(get_cpu(), reloc_cpumask);*/
	/*for_each_online_cpu(cpu) {*/
	/*	cpumask_clear_cpu(cpu, reloc_cpumask);*/
	/*}*/
	/**/
	/*if (!cpumask_empty(reloc_cpumask)) {*/
	/*	preempt_disable();*/
	/*	smp_call_function_many(reloc_cpumask, ipi, NULL, 0);*/
	/*	preempt_enable();*/
	/*} else {*/
	/*	pr_info("shit");*/
	/*}*/
	/*start = jiffies;*/
	/*while (!cpumask_empty(reloc_cpumask)) {*/
	/*	if (!time_before(jiffies, start + 2*HZ)) {*/
	/*		pr_err("Timeout waiting for mce inject %lx\n",*/
	/*			*cpumask_bits(reloc_cpumask));*/
	/*		break;*/
	/*	}*/
	/*	cpu_relax();*/
	/*}*/
	/**/
	/*put_cpu();*/
	/*cpus_read_unlock();*/
	u32 smi_en = 0;
	u16 pmbase = DEFAULT_PMBASE;

	smi_en |= TCO_EN;
	smi_en |= APMC_EN;
	smi_en |= BIOS_EN;
	smi_en |= EOS | GBL_SMI_EN;

	outl(smi_en, pmbase + PM1_EN);
	
	outb(0x00, 0xb2);

	int i;
	/*for_each_cpu(i, cpu_online_mask) {*/
	/*	printk("this is cpu %d", i);*/
	/*	apic->send_IPI(i, LAPIC_INT_ASSERT | LAPIC_DM_SMI);*/
	/*}*/
	/*for_each_possible_cpu(i) {*/
	/*	cpumask_set_cpu(i, reloc_cpumask);*/
	/*}*/
	//apic->default_send_IPI_single_phys(0, LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	/*cpumask_set_cpu(0, reloc_cpumask);*/
	/*cpumask_set_cpu(1, reloc_cpumask);*/
	/**/
	/*if (cpumask_test_cpu(0, reloc_cpumask))*/
	/*	printk("test");*/
	/**/
	/*int this; */
	/*this = get_cpu();*/
	/*//apic->send_IPI(this, LAPIC_INT_ASSERT | LAPIC_DM_SMI);*/
	/*smp_call_function_many(reloc_cpumask, ipi, NULL, 1);*/
	/*put_cpu();*/
	printk(KERN_INFO "done");
}


	//smi(smp_processor_id(), back);
	//apic->send_IPI(smp_processor_id(), LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	//apic->send_IPI(smp_processor_id(), LAPIC_DM_SMI | LAPIC_DM_SMI);
	/*for_each_online_cpu(i){*/
	/*	if (!is_for_smm) */
	/*		pr_info("not good");*/
	/*}*/

	//fsleep(10000000);

	//apic->send_IPI(4, LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	//enable_preempt();
	//cpus_
//}

static int __init smm_loader_init(void)
{
	/*int i, bsp;*/
	/*bsp = smp_processor_id();*/
	/*for_each_online_cpu(i)*/
	/*	pr_info("cpu id: %d, %d", i, smp_processor_id());*/
	/**/
	/*pr_info("we send smi to the processor that catches this instruction so %d", smp_processor_id());*/
	initiate_relocation();
	//
	/*for_each_possible_cpu(i) {*/
	/*	if (i != bsp)*/
	/*		pr_info("we shall send smi: %d", i);*/
	/*}*/
	/**/
	/*i = bsp + 1;*/
	/*if (i == 23)*/
	/*	i = bsp - 1;*/
	/**/
	/*pr_info("sending second smi to %d", i);*/
	//smi(i, true);
	
	return 0;
}

static void __exit smm_loader_exit(void)
{
	// placeholder for now
	printk(KERN_INFO "SMM initialized");
	// no need for a "big" cleanup, the critical parts of memory was already freed in get_cb_data(), we have no deadlock here - proof: try executing as out-of-tree, rmmod should go without issues
	// meaning the remaining "parser" modules may be removed and do not create cross-dependenceis with this loader.
}

module_init(smm_loader_init);
module_exit(smm_loader_exit);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("SMM Loader driver - installs permanent SMI handler");
MODULE_LICENSE("GPL v2");
