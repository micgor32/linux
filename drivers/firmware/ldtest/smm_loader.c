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
	//apic->send_IPI_self(LAPIC_INT_ASSERT | LAPIC_DM_SMI);
}

static cpumask_var_t reloc_cpumask;

static void initiate_relocation(void)
{
	//printk(KERN_INFO "boot_cpu is %d", get_boot_cpu_id());
	//on_each_cpu(ipi, NULL, 1);
	//on_each_cpu_cond_mask(NULL, ipi, NULL, true, cpu_online_mask); 
	//apic->send_IPI(24, LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	//apic->send_IPI(smp_processor_id(), LAPIC_INT_ASSERT | LAPIC_DM_SMI);
	outb(0x00, 0xb2);
	int cpu;
	unsigned long start;
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
	printk(KERN_INFO "done");
}

	//outb(0x00, 0xb2);

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
