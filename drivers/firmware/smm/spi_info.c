// SPDX-License-Identifier: GPL-2.0-only
/*
 * spi_info.c
 *
 * Driver for exporting SPI flash information structure from coreboot table.
 *
 * Copyright 2025 9elements gmbh
 * Copyright 2025 Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "coreboot_table.h"
#include "smm.h"

static struct lb_pld_spi_flash_info *spi_cbtable_info;
struct spi_flash_info *spi_info;
extern u64 lshift(u64, uint);
extern u64 unpack_cbuint64(struct cbuint64);

static int spi_info_driver_probe(struct coreboot_device *dev)
{
	spi_cbtable_info = &dev->spi_info;
	if (spi_cbtable_info->tag != LB_TAG_PLD_SPI_FLASH_INFO) {
		return -ENXIO;
	}

	spi_info = kmalloc(sizeof(struct spi_flash_info), GFP_KERNEL);
	spi_info->revision = spi_cbtable_info->revision;
	spi_info->flags = spi_cbtable_info->flags;
	spi_info->spi_address.register_id =
		spi_cbtable_info->spi_address.register_id;
	spi_info->spi_address.address_space_id =
		spi_cbtable_info->spi_address.address_space_id;
	spi_info->spi_address.register_bit_width =
		spi_cbtable_info->spi_address.register_bit_width;
	spi_info->spi_address.register_bit_offset =
		spi_cbtable_info->spi_address.register_bit_offset;
	spi_info->spi_address.value = spi_cbtable_info->spi_address.value;
	spi_info->spi_address.address = unpack_cbuint64(spi_cbtable_info->spi_address.address);

	return 0;
}

EXPORT_SYMBOL_GPL(spi_info);

static void spi_info_driver_remove(struct coreboot_device *dev)
{
	kfree(spi_info);
}

static const struct coreboot_device_id spi_info_ids[] = {
	{ .tag = LB_TAG_PLD_SPI_FLASH_INFO },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(coreboot, spi_info_ids);

static struct coreboot_driver spi_info_driver = {
    .probe = spi_info_driver_probe,
    .remove = spi_info_driver_remove,
    .drv = {
        .name = "spi_info",
    },
    .id_table = spi_info_ids,
};

module_coreboot_driver(spi_info_driver);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION(
	"Driver for exporting SPI flash information structure information from coreboot table to sysfs");
MODULE_LICENSE("GPL v2");
