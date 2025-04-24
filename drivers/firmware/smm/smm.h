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

#define PAYLOAD_MM_RET_SUCCESS     0
#define PAYLOAD_MM_RET_FAILURE	   1
#define PAYLOAD_MM_UNLOCK_SMRAM    1
#define PAYLOAD_MM_REGISTER_ENTRY  2
#define PAYLOAD_MM_LOCK_SMRAM      3

u64 lshift(u64 opr, uint count);
u64 unpack_cbuint64(struct cbuint64 inp);

struct generic_register {
	u8 register_id;
	u8 address_space_id;
	u8 register_bit_width;
	u8 register_bit_offset;
	u32 value;
	u64 address;
};

struct smram_descriptor {
	u64 physical_start;
	u64 physical_size;
	u64 cpu_start;
	u64 region_state;
};

struct smram_info {
	u32 nr_of_smm_regions;
	struct smram_descriptor descriptor[1];
};

struct s3_comm_info {
	u64 physical_start;
	u64 physical_size;
	u64 region_state;
	u8 pld_acpi_s3_enable;
};

struct mm_info {
	u8 revision;
	u8 requires_long_mode_call;
	u8 register_mm_entry_swsmi;
};

struct smm_data {
	u32 nr_of_smm_regions;
	struct smram_descriptor region[1];
	struct s3_comm_info s3_info; 
	struct mm_info mm_info;
	int cpu_count;
};

#endif
