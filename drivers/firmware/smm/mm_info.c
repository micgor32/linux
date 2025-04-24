// SPDX-License-Identifier: GPL-2.0-only
/*
 * smram.c
 *
 * Driver for exporting SMRAM information from coreboot table.
 *
 * Copyright 2025 9elements gmbh
 * Copyright 2025 Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "../google/coreboot_table.h"
#include "smm.h"

static struct lb_pld_mm_interface_info *mm_cbtable_info;
struct mm_info *mm_info;

static int mm_driver_probe(struct coreboot_device *dev)
{
	mm_cbtable_info = &dev->mm_info;
	// Check whether we've got the correct cb device
	if (mm_cbtable_info->tag != CB_TAG_PLD_MM_INTERFACE_INFO) {
		return -ENXIO;
	}

	mm_info = kmalloc(sizeof(struct mm_info), GFP_KERNEL);
	mm_info->revision = mm_cbtable_info->revision;
	mm_info->requires_long_mode_call = mm_cbtable_info->requires_long_mode_call;
	mm_info->register_mm_entry_swsmi = mm_cbtable_info->register_mm_entry_swsmi;
	return 0;
}

EXPORT_SYMBOL(mm_info);

static void mm_driver_remove(struct coreboot_device *dev)
{
	kfree(mm_info);
}

static const struct coreboot_device_id mm_info_ids[] = {
	{ .tag = CB_TAG_PLD_MM_INTERFACE_INFO },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(coreboot, mm_info_ids);

static struct coreboot_driver mm_driver = {
    .probe = mm_driver_probe,
    .remove = mm_driver_remove,
    .drv = {
        .name = "mm_info",
    },
    .id_table = mm_info_ids,
};

module_coreboot_driver(mm_driver);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("Driver for exporting MM info from coreboot table");
MODULE_LICENSE("GPL v2");
