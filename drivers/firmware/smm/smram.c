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

#include "coreboot_table.h"
#include "smm.h"

static struct lb_pld_smram_descriptor_block *smram_cbtable_info;
struct smram_info *smram;

static int smram_driver_probe(struct coreboot_device *dev)
{
	smram_cbtable_info = &dev->smram_info;
	// Check whether we've got the correct cb device
	if (smram_cbtable_info->tag != CB_TAG_PLD_SMM_SMRAM) {
		return -ENXIO;
	}

	smram = kmalloc(
		sizeof(struct smram_info) +
			sizeof(struct smram_descriptor) *
				(smram_cbtable_info->number_of_smm_regions - 1),
		GFP_KERNEL);
	smram->nr_of_smm_regions = smram_cbtable_info->number_of_smm_regions;
	// Check whether the number of regions is present, if not, no need to iterate and likely
	// the informations are missing in the cbtable.
	if (smram->nr_of_smm_regions == 0) {
		kfree(smram);
		return -ENXIO;
	}

	for (int i = 0; i < smram->nr_of_smm_regions; i++) {
		smram->descriptor[i].physical_start =
			smram_cbtable_info->descriptor[i].physical_start;
		smram->descriptor[i].cpu_start =
			smram_cbtable_info->descriptor[i].physical_start;
		smram->descriptor[i].physical_size =
			smram_cbtable_info->descriptor[i].physical_size;
		smram->descriptor[i].region_state =
			smram_cbtable_info->descriptor[i].region_state;
	}

	return 0;
}

EXPORT_SYMBOL(smram);

static void smram_driver_remove(struct coreboot_device *dev)
{
	kfree(smram);
}

static const struct coreboot_device_id smm_info_ids[] = {
	{ .tag = CB_TAG_PLD_SMM_SMRAM },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(coreboot, smm_info_ids);

static struct coreboot_driver smram_driver = {
    .probe = smram_driver_probe,
    .remove = smram_driver_remove,
    .drv = {
        .name = "smram",
    },
    .id_table = smm_info_ids,
};

module_coreboot_driver(smram_driver);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("Driver for exporting SMRAM information from coreboot table");
MODULE_LICENSE("GPL v2");
