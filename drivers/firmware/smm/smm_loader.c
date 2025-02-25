// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for installing SMI handler and locking down SMM
 *
 * Copyright (c) 2025 9elements GmbH
 *
 * Author: Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/acpi.h>

//#include "smm_loader"

static int __init smm_loader_init(void)
{
    read_cbtable()

    return 0;
}




static void __exit smm_loader_exit(void)
{
    printk(KERN_INFO "Permanent SMI handler installed\n");
}

module_init(smm_loader_init);
module_exit(smm_loader_exit);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("SMM Loader driver - installs permanent SMI handler"); 
MODULE_LICENSE("GPL v2");
