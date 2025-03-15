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

#include <linux/cpu.h>

#define SMM_DEFAULT_SMBASE 0x30000
#define SMM_DEFAULT_SIZE   0x10000
#define SMM_ENTRY_OFFSET 0x8000
#define SMM_MINIMUM_STACK_SIZE 32

#define	LAPIC_INT_ASSERT 0x04000
#define	LAPIC_DM_SMI 0x00200


u64 lshift(u64 opr, uint count);

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

struct smm_registers_info {
	u32 count;
	u32 revision;
	struct generic_register registers[];
};

struct smram_info {
	u32 nr_of_smm_regions;
	struct smram_descriptor descriptor[1];
	u32 stack_size;
	u64 perm_smsize; 
	u32 perm_smbase;
	u64 smm_save_state_size;
	u32 cr3;

};

struct spi_flash_info {
	u16 revision;
	u16 flags;
	struct generic_register spi_address;
};

struct s3_comm_info {
	u64 physical_start;
	u64 physical_size;
	u64 region_state;
	u8 pld_acpi_s3_enable;
};


struct smm_state {
	int cpu_count;
	uintptr_t perm_smbase;
	size_t perm_smsize;
	size_t smm_save_state_size;
	size_t nr_cnn_save_states;
	u32 cr3;
};

struct smm_data {
	struct smram_info smram;
	struct smm_registers_info registers;
	struct spi_flash_info spi_flash_info;
	struct s3_comm_info s3_info; // wont be used for now
	int cpu_count;
};

struct stub_data {
	u32 stack_size;
	u32 stack_top;
	u32 c_handler; 
	u32 cr3;
	u16 apic_to_cpu_num[CONFIG_NR_CPUS]; // Medicore approach. It assumes the person builing the kernel wont put bigger number than they need, otherwise we just waste space.
} __packed;

#endif
