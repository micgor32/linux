// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for installing SMI handler and locking down SMM
 *
 * Copyright (c) 2025 9elements GmbH
 *
 * Author: Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/acpi.h>

#include "smm_loader.h"
#include "coreboot_table.h"

static struct coreboot_table_entry* find_tag(u32 tag, struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    int i;

    entry = &dev->entry;
    for (i = 0; i < dev->entry.size / sizeof(struct coreboot_table_entry); ++i) {
        if (entry[i].tag == tag) {
            pr_info("found correct table");
            return &entry[i];
        }
    }

    // TODO: see whether there is a better way than just ack that table will be empty.
    pr_info("the correct table was not found"); // temporary remove later
    // if no corresponding tag was find, we just return empty table entry which will later cause the probe to throw an error
    return entry;
}

static void parse_registers_info(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_smm_registers *sreg;
    
    // TODO: check for not found entries, we do not want this to be empty later on
    entry = find_tag(CB_TAG_PLD_SMM_REGISTER_INFO, dev);
    sreg = (struct lb_pld_smm_registers *)((u8 *)entry + sizeof(struct coreboot_table_entry));
    // for now just printing some info, later return entry
    pr_info("SMM Register Info (Tag: 0x%x):\n", entry->tag);
    pr_info("  Count: %u\n", sreg->count);
    for (int j = 0; j < sreg->count; ++j) {
        pr_info("  Register ID: 0x%x, Value: 0x%x\n", sreg->registers[j].register_id, sreg->registers[j].value);
    }

}

static void parse_smram_descriptor_block(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_smram_descriptor_block *smram_desc;

    // same here add a check
    entry = find_tag(CB_TAG_PLD_SMM_SMRAM, dev);
    // see whether u8 is enough
    smram_desc = (struct lb_pld_smram_descriptor_block *)((u8 *)entry + sizeof(struct coreboot_table_entry));
    // s.a
    pr_info("SMRAM desc (Tag: 0x%x):\n", entry->tag);
    pr_info("nr of smm regions 0x%x\n", smram_desc->number_of_smm_regions);
}

static void parse_spi_info(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_spi_flash_info *spi_flash_inf;

    entry = find_tag(LB_TAG_PLD_SPI_FLASH_INFO, dev);
    // s.a
    spi_flash_inf =  (struct lb_pld_spi_flash_info *)((u8 *)entry + sizeof(struct coreboot_table_entry));
    pr_info("SPI flash info (Tag: 0x%x):\n", entry->tag);
    pr_info("spi value 0x%x\n", spi_flash_inf->spi_address.value);
 
}

static void parse_nv_variable(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_nv_variable_info *nv_var_info;

    entry = find_tag(LB_TAG_PLD_NV_VARIABLE_INFO, dev);
    // s.a
    nv_var_info = (struct lb_pld_nv_variable_info *)((u8 *)entry + sizeof(struct coreboot_table_entry));
    pr_info("nv var info (Tag: 0x%x):\n", entry->tag);
    pr_info("spi value 0x%x\n", nv_var_info->revision);
}

static void parse_s3_comm(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_s3_communication *s3_comm;

    entry = find_tag(CB_TAG_PLD_S3_COMMUNICATION, dev);
    // s.a
    s3_comm = (struct lb_pld_s3_communication *)((u8 *)entry + sizeof(struct coreboot_table_entry));
    pr_info("S3 comms (Tag: 0x%x):\n", entry->tag);
    pr_info("s3 enabled 0x%x\n", s3_comm->pld_acpi_s3_enable);
 
}

static int smm_loader_probe(struct coreboot_device *dev) {
    pr_info("SMM Loader: Probing Coreboot device...\n");
    // for now just checking whether it works, later let parsers just return a dts used by other functions
    parse_registers_info(dev);
    parse_smram_descriptor_block(dev);
    parse_spi_info(dev);
    parse_nv_variable(dev);
    parse_s3_comm(dev);
    return 0;
}

static void smm_loader_remove(struct coreboot_device *dev) {
    // leave empty for now
}


static const struct coreboot_device_id smm_loader_ids[] = {
    { .tag = CB_TAG_PLD_SMM_REGISTER_INFO },
    { .tag = CB_TAG_PLD_SMM_SMRAM },
    { .tag = LB_TAG_PLD_SPI_FLASH_INFO },  
    { .tag = LB_TAG_PLD_NV_VARIABLE_INFO }, 
    { .tag = CB_TAG_PLD_S3_COMMUNICATION },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(coreboot, smm_loader_ids);

static struct coreboot_driver smm_loader_driver = {
    .probe = smm_loader_probe,
    .remove = smm_loader_remove,
    .drv = {
        .name = "smm_loader",
    },
    .id_table = smm_loader_ids,
};

module_coreboot_driver(smm_loader_driver);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("SMM Loader driver - installs permanent SMI handler"); 
MODULE_LICENSE("GPL v2");
