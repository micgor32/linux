// SPDX-License-Identifier: GPL-2.0-only
/*
 * smm_registers.c
 *
 * Driver for exporting SMM-specific register information from coreboot table.
 *
 * Copyright 2025 9elements gmbh
 * Copyright 2025 Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "coreboot_table.h"
#include "smm.h"

static struct lb_pld_smm_registers *smm_regs_cbtable_info;
struct smm_registers_info *smm_regs;
extern u64 lshift(u64, uint);
extern u64 unpack_cbuint64(struct cbuint64);

static int smm_regs_driver_probe(struct coreboot_device *dev)
{
	smm_regs_cbtable_info = &dev->smm_registers;
	// check whether we've got the correct cb device
	if (smm_regs_cbtable_info->tag != CB_TAG_PLD_SMM_REGISTER_INFO) {
		return -ENXIO;
	}

	smm_regs = kmalloc(sizeof(struct smm_registers_info) +
				   sizeof(struct generic_register) *
					   (smm_regs_cbtable_info->count - 1),
			   GFP_KERNEL);
	smm_regs->count = smm_regs_cbtable_info->count;
	smm_regs->revision = smm_regs_cbtable_info->revision;

	if (smm_regs->count == 0) {
		kfree(smm_regs);
		return -ENXIO;
	}

	for (int i = 0; i < smm_regs->count; i++) {
		smm_regs->registers[i].register_id =
			smm_regs_cbtable_info->registers[i].register_id;
		smm_regs->registers[i].address_space_id =
			smm_regs_cbtable_info->registers[i].address_space_id;
		smm_regs->registers[i].register_bit_width =
			smm_regs_cbtable_info->registers[i].register_bit_width;
		smm_regs->registers[i].register_bit_offset =
			smm_regs_cbtable_info->registers[i].register_bit_offset;
		smm_regs->registers[i].value =
			smm_regs_cbtable_info->registers[i].value;
		smm_regs->registers[i].address =
			unpack_cbuint64(smm_regs_cbtable_info->registers[i].address);
	}

	return 0;
}

EXPORT_SYMBOL(smm_regs);

static void smm_regs_driver_remove(struct coreboot_device *dev)
{
	kfree(smm_regs);
}

static const struct coreboot_device_id smm_info_ids[] = {
	{ .tag = CB_TAG_PLD_SMM_REGISTER_INFO },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(coreboot, smm_info_ids);

static struct coreboot_driver smm_regs_driver = {
    .probe = smm_regs_driver_probe,
    .remove = smm_regs_driver_remove,
    .drv = {
        .name = "smm_registers",
    },
    .id_table = smm_info_ids,
};

module_coreboot_driver(smm_regs_driver);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION(
	"Driver for exporting SMM-specific registers information from coreboot table");
MODULE_LICENSE("GPL v2");
