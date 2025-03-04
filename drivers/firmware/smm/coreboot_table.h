/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * coreboot_table.h
 *
 * Internal header for coreboot table access.
 *
 * Copyright 2014 Gerd Hoffmann <kraxel@redhat.com>
 * Copyright 2017 Google Inc.
 * Copyright 2017 Samuel Holland <samuel@sholland.org>
 * Copyright 2025 9elements gmbh
 * Copyright 2025 Michal Gorlas <michal.gorlas@9elements.com>
 */

#ifndef __COREBOOT_TABLE_H
#define __COREBOOT_TABLE_H

#include <linux/device.h>
#include <linux/mod_devicetable.h>

/* Coreboot table header structure */
struct coreboot_table_header {
	char signature[4];
	u32 header_bytes;
	u32 header_checksum;
	u32 table_bytes;
	u32 table_checksum;
	u32 table_entries;
};

/* List of coreboot entry structures that is used */
/* Generic */
struct coreboot_table_entry {
	u32 tag;
	u32 size;
};

/* Points to a CBMEM entry */
struct lb_cbmem_ref {
	u32 tag;
	u32 size;

	u64 cbmem_addr;
};

#define LB_TAG_CBMEM_ENTRY 0x31
/* Corresponds to LB_TAG_CBMEM_ENTRY */
struct lb_cbmem_entry {
	u32 tag;
	u32 size;

	u64 address;
	u32 entry_size;
	u32 id;
};

#define CB_TAG_PLD_SMM_SMRAM          0x51
struct lb_pld_smram_descriptor {
	u64 physical_start;
	u64 physical_size;
	u64 region_state;
};

struct lb_pld_smram_descriptor_block {
	u32 tag;
	u32 size;
	u32 number_of_smm_regions;
	struct lb_pld_smram_descriptor descriptor[1];
};

#define CB_TAG_PLD_SMM_REGISTER_INFO  0x50
struct lb_pld_generic_register {
	u8 register_id;
	u8 address_space_id;
	u8 register_bit_width;
	u8 register_bit_offset;
	u32 value;
	u64 address;
};

struct lb_pld_smm_registers {
	u32 tag;
	u32 size;
	u32 revision;
	u32 count;
	struct lb_pld_generic_register registers[];
};

#define LB_TAG_PLD_SPI_FLASH_INFO     0x52
struct lb_pld_spi_flash_info {
	u32 tag;
	u32 size;
	u16 revision;
	u16 flags;
	struct lb_pld_generic_register spi_address;
};

#define CB_TAG_PLD_S3_COMMUNICATION   0x54
struct lb_pld_s3_communication {
	u32 tag;
	u32 size;
	struct lb_pld_smram_descriptor comm_buffer;
	u8 pld_acpi_s3_enable;
	u8 pad[3];
};

/* Describes framebuffer setup by coreboot */
struct lb_framebuffer {
	u32 tag;
	u32 size;

	u64 physical_address;
	u32 x_resolution;
	u32 y_resolution;
	u32 bytes_per_line;
	u8  bits_per_pixel;
	u8  red_mask_pos;
	u8  red_mask_size;
	u8  green_mask_pos;
	u8  green_mask_size;
	u8  blue_mask_pos;
	u8  blue_mask_size;
	u8  reserved_mask_pos;
	u8  reserved_mask_size;
};

/* A device, additionally with information from coreboot. */
struct coreboot_device {
	struct device dev;
	union {
		struct coreboot_table_entry entry;
		struct lb_cbmem_ref cbmem_ref;
		struct lb_cbmem_entry cbmem_entry;
		struct lb_framebuffer framebuffer;
		struct lb_pld_smram_descriptor_block smram_info;
		struct lb_pld_smm_registers smm_registers;
		struct lb_pld_spi_flash_info spi_info;
		struct lb_pld_s3_communication s3_comm;
		DECLARE_FLEX_ARRAY(u8, raw);
	};
};

static inline struct coreboot_device *dev_to_coreboot_device(struct device *dev)
{
	return container_of(dev, struct coreboot_device, dev);
}

/* A driver for handling devices described in coreboot tables. */
struct coreboot_driver {
	int (*probe)(struct coreboot_device *);
	void (*remove)(struct coreboot_device *);
	struct device_driver drv;
	const struct coreboot_device_id *id_table;
};

/* use a macro to avoid include chaining to get THIS_MODULE */
#define coreboot_driver_register(driver) \
	__coreboot_driver_register(driver, THIS_MODULE)
/* Register a driver that uses the data from a coreboot table. */
int __coreboot_driver_register(struct coreboot_driver *driver,
			       struct module *owner);

/* Unregister a driver that uses the data from a coreboot table. */
void coreboot_driver_unregister(struct coreboot_driver *driver);

/* module_coreboot_driver() - Helper macro for drivers that don't do
 * anything special in module init/exit.  This eliminates a lot of
 * boilerplate.  Each module may only use this macro once, and
 * calling it replaces module_init() and module_exit()
 */
#define module_coreboot_driver(__coreboot_driver) \
	module_driver(__coreboot_driver, coreboot_driver_register, \
			coreboot_driver_unregister)

#endif /* __COREBOOT_TABLE_H */
