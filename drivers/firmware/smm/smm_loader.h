// SPDX-License-Identifier: GPL-2.0
/*
 * Internal header for SMM loader 
 *
 * Copyright (c) 2025 9elements GmbH
 *
 * Author: Michal Gorlas <michal.gorlas@9elements.com>
 */

#ifndef __SMM_LOADER_H
#define __SMM_LOADER_H

#define CB_TAG_PLD_SMM_REGISTER_INFO  0x50
struct lb_pld_generic_register {
	u8 register_id;
	u8 address_space_id;
	u8 register_bit_width;
	u8 register_bit_offset;
	u32 value;
	u64 address;
};

#define CB_TAG_PLD_SMM_SMRAM          0x51
struct lb_pld_smm_registers {
	u32 tag;
	u32 size;
	u32 revision;
	u32 count;
	struct lb_pld_generic_register registers[];
};

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

#define LB_TAG_PLD_SPI_FLASH_INFO     0x52
struct lb_pld_spi_flash_info {
	u32 tag;
	u32 size;
	u16 revision;
	u16 flags;
	struct lb_pld_generic_register spi_address;
};

#define LB_TAG_PLD_NV_VARIABLE_INFO   0x53
struct lb_pld_nv_variable_info {
	u32 tag;
	u32 size;
	u32 revision;
	u32 variable_store_base;
	u32 variable_store_size;
};

#define CB_TAG_PLD_S3_COMMUNICATION   0x54
struct lb_pld_s3_communication {
	u32 tag;
	u32 size;
	struct lb_pld_smram_descriptor comm_buffer;
	u8 pld_acpi_s3_enable;
	u8 pad[3];
};

//:w
//void parse_registers_info(struct coreboot_device *dev);

/*struct smm_info {*/
/* this will be used to */
/*}*/

#endif
