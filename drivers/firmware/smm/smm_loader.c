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
//#include "coreboot_table.h"

static void* find_tag(u32 tag, struct coreboot_table_header *header)
{
    struct coreboot_table_entry *entry;
    void *tag_ptr = (void* )header + header->header_bytes;
    int i;

    pr_info("Coreboot Table Header: Entries = %u, Header Bytes = %u\n", header->table_entries, header->header_bytes);

    for (i = 0; i < header->table_entries; ++i) {
        entry = (struct coreboot_table_entry *)tag_ptr;
        pr_info("0x%x\n", entry->tag);
        if (entry->tag == tag) {
            pr_info("found tag 0x%x", entry->tag);
            return entry;
        }

        tag_ptr += entry->size;
    }
    pr_info("tag not found");
    return NULL;
}
  //TmpPtr = (UINT8 *)Header + Header->header_bytes;
  /*for (Idx = 0; Idx < Header->table_entries; Idx++) {*/
  /*  Record = (struct cb_record *)TmpPtr;*/
  /*  if (Record->tag == Tag) {*/
  /*    TagPtr = TmpPtr;*/
  /*    break;*/
  /*  }*/
  /**/
  /*  TmpPtr += Record->size;*/
  /*}*/
  /**/
  /*return TagPtr;*/
  /**/

    /*entry = &dev->entry;*/
    /*for (i = 0; i < dev->entry.size / sizeof(struct coreboot_table_entry); ++i) {*/
    /*    pr_info("0x%x\n", entry[i].tag);*/
    /*    if (entry[i].tag == tag) {*/
    /*        pr_info("found correct table");*/
    /*        return &entry[i];*/
    /*    }*/
    /*}*/

    // TODO: see whether there is a better way than just ack that table will be empty.
    //pr_info("the correct table was not found"); // temporary remove later
    // if no corresponding tag was find, we just return empty table entry which will later cause the probe to throw an error
    //return entry;

static void parse_registers_info(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_smm_registers *sreg;
    
    // TODO: check for not found entries, we do not want this to be empty later on
    //entry = find_tag(CB_TAG_PLD_SMM_REGISTER_INFO, dev);
    /*sreg = (struct lb_pld_smm_registers *)((u8 *)entry + sizeof(struct coreboot_table_entry));*/
    /*// for now just printing some info, later return entry*/
    /*pr_info("SMM Register Info (Tag: 0x%x):\n", entry->tag);*/
    /*pr_info("  Count: %u\n", sreg->count);*/
    /*for (int j = 0; j < sreg->count; ++j) {*/
    /*    pr_info("  Register ID: 0x%x, Value: 0x%x\n", sreg->registers[j].register_id, sreg->registers[j].value);*/
    /*}*/

}

static void parse_smram_descriptor_block(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_smram_descriptor_block *smram_desc;
    struct coreboot_table_header *header = (struct coreboot_table_header *)CB_HEADER_SIGNATURE;
    entry = find_tag(CB_TAG_PLD_SMM_SMRAM, header);
    if (!entry) {
        pr_info("regstr not found\n");
    }
    pr_info("entry tag 0x%x", entry->tag);
    smram_desc = &dev->smram_descriptor;
    //smram_desc = (struct lb_pld_smram_descriptor_block *)((u32 *)entry + sizeof(struct lb_pld_smram_descriptor));
    /*if (smram_desc != NULL) {*/
    /*    smram_desc->number_of_smm_regions = entry->number_of_smm_regions;*/
    /*    for (int i = 0; i < entry->number_of_smm_regions; i++) {*/
    /*        smram_desc->descriptor[i].physical_start = entry-> */
    /*    }*/
    /*}*/
    // s.a
    pr_info("SMRAM desc (Tag: 0x%x):\n", smram_desc->tag);
    pr_info("nr of smm regions 0x%x\n", smram_desc->number_of_smm_regions);
}

static void parse_spi_info(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_spi_flash_info *spi_flash_inf;

    //entry = find_tag(LB_TAG_PLD_SPI_FLASH_INFO, dev);
    // s.a
    /*spi_flash_inf =  (struct lb_pld_spi_flash_info *)((u8 *)entry + sizeof(struct coreboot_table_entry));*/
    /*pr_info("SPI flash info (Tag: 0x%x):\n", entry->tag);*/
    /*pr_info("spi value 0x%x\n", spi_flash_inf->spi_address.value);*/
    /**/
}

static void parse_nv_variable(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_nv_variable_info *nv_var_info;

    //entry = find_tag(LB_TAG_PLD_NV_VARIABLE_INFO, dev);
    // s.a
    /*nv_var_info = (struct lb_pld_nv_variable_info *)((u8 *)entry + sizeof(struct coreboot_table_entry));*/
    /*pr_info("nv var info (Tag: 0x%x):\n", entry->tag);*/
    /*pr_info("spi value 0x%x\n", nv_var_info->revision);*/
}

static void parse_s3_comm(struct coreboot_device *dev)
{
    struct coreboot_table_entry *entry;
    struct lb_pld_s3_communication *s3_comm;

    //entry = find_tag(CB_TAG_PLD_S3_COMMUNICATION, dev);
    // s.a
    /*s3_comm = (struct lb_pld_s3_communication *)((u8 *)entry + sizeof(struct coreboot_table_entry));*/
    /*pr_info("S3 comms (Tag: 0x%x):\n", entry->tag);*/
    /*pr_info("s3 enabled 0x%x\n", s3_comm->pld_acpi_s3_enable);*/
 
}

static int smm_loader_probe(struct coreboot_device *dev) {
    pr_info("SMM Loader: Probing Coreboot device...\n");


    /*void *SmramDesc = find_tag(CB_TAG_PLD_SMM_SMRAM, dev);*/
    /*pr_info("%x", SmramDesc);*/
    /*if (SmramDesc != NULL) {*/
    /*    struct lb_pld_smram_descriptor_block *smram_desc = (struct lb_pld_smram_descriptor_block *)((u8 *)SmramDesc + sizeof(struct coreboot_table_entry));*/
    /*    pr_info("SMRAM Descriptor Block (Tag: 0x%x):\n", ((struct coreboot_table_entry *)SmramDesc)->tag);*/
    /*    pr_info("Number of SMM regions: 0x%x\n", smram_desc->number_of_smm_regions);*/
    /*}*/
    // for now just checking whether it works, later let parsers just return a dts used by other functions
    //parse_registers_info(dev);
    parse_smram_descriptor_block(dev);
    //parse_spi_info(dev);
    //parse_nv_variable(dev);
    //parse_s3_comm(dev);
    return 0;
}

static void smm_loader_remove(struct coreboot_device *dev) {
    // leave empty for now
}


static const struct coreboot_device_id smm_loader_ids[] = {
    //{ .tag = CB_TAG_PLD_SMM_REGISTER_INFO },
    { .tag = CB_TAG_PLD_SMM_SMRAM },
    //{ .tag = LB_TAG_PLD_SPI_FLASH_INFO },  
    /*{ .tag = LB_TAG_PLD_NV_VARIABLE_INFO }, */
    /*{ .tag = CB_TAG_PLD_S3_COMMUNICATION },*/
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
