// SPDX-License-Identifier: GPL-2.0-only
/*
 * s3_comm.c
 *
 * Driver for exporting SMM-specific register information from coreboot table to sysfs.
 *
 * Copyright 2025 9elements gmbh
 * Copyright 2025 Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "coreboot_table.h"
#include "smm.h"

static struct lb_pld_s3_communication *s3_cbtable_info;
struct s3_comm_info *s3_info;

static int s3_communications_driver_probe(struct coreboot_device *dev)
{
	s3_cbtable_info = &dev->s3_comm;
	if (s3_cbtable_info->tag != CB_TAG_PLD_S3_COMMUNICATION) {
		return -ENXIO;
	}

	s3_info = kmalloc(sizeof(struct s3_comm_info), GFP_KERNEL);
	s3_info->physical_start = s3_cbtable_info->comm_buffer.physical_start;
	s3_info->pld_acpi_s3_enable = s3_cbtable_info->pld_acpi_s3_enable;
	s3_info->physical_size = s3_cbtable_info->comm_buffer.physical_size;
	s3_info->region_state = s3_cbtable_info->comm_buffer.region_state;

	return 0;
}

EXPORT_SYMBOL(s3_info);

static void s3_communications_driver_remove(struct coreboot_device *dev)
{
	kfree(s3_info);
}

static const struct coreboot_device_id s3_info_ids[] = {
	{ .tag = CB_TAG_PLD_S3_COMMUNICATION },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(coreboot, s3_info_ids);

static struct coreboot_driver s3_comm_driver = {
    .probe = s3_communications_driver_probe,
    .remove = s3_communications_driver_remove,
    .drv = {
        .name = "s3_comm",
    },
    .id_table = s3_info_ids,
};

module_coreboot_driver(s3_comm_driver);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION(
	"Driver for exporting SMRAM information from coreboot table to sysfs");
MODULE_LICENSE("GPL v2");
