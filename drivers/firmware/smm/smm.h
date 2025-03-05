/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * smm.h
 *
 * Internal header for SMM driver.
 *
 * Copyright 2025 9elements gmbh
 * Copyright 2025 Michal Gorlas <michal.gorlas@9elements.com>
 */

#ifndef __SMM_H
#define __SMM_H

struct generic_register {
	u8 register_id;
	u8 address_space_id;
	u8 register_bit_width;
	u8 register_bit_offset;
	u32 value;
	u32 address;
};

struct smram_descriptor {
	u32 physical_start;
	u32 physical_size;
	u32 cpu_start;
	u32 region_state;
};

struct smm_registers_info {
	u32 count;
	u32 revision;
	struct generic_register registers[];
};

struct smram_info {
	u32 nr_of_smm_regions;
	struct smram_descriptor descriptor[1];
};

struct spi_flash_info {
	u16 revision;
	u16 flags;
	struct generic_register spi_address;
};

struct s3_comm_info {
	u32 physical_start;
	u32 physical_size;
	u32 region_state;
	u8 pld_acpi_s3_enable;
};

#endif
