// SPDX-License-Identifier: GPL-2.0-only
/*
 * smram.c
 *
 * Driver for exporting SMRAM information from coreboot table to sysfs.
 *
 * Copyright 2025 9elements gmbh
 * Copyright 2025 Michal Gorlas <michal.gorlas@9elements.com>
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/string.h>

#include "coreboot_table.h"

static struct lb_pld_smram_descriptor_block *smm_info;

static ssize_t smram_tag_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", smm_info->tag);
}

static ssize_t smram_size_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", smm_info->size);
}

static ssize_t smram_nr_reg_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "%x\n", smm_info->number_of_smm_regions);
}

static ssize_t smram_subregions_size_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i;
    char local_buf[PAGE_SIZE];
    ssize_t ret = 0;

    for (i = 0; i < smm_info->number_of_smm_regions; i++){
        ret += scnprintf(local_buf + ret, sizeof(local_buf) - ret,
                    "0x%llx\n", smm_info->descriptor[i].physical_size);
    }
    return sysfs_emit(buf, "%s", local_buf);
}

static ssize_t smram_subregions_start_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i;
    char local_buf[PAGE_SIZE];
    ssize_t ret = 0;

    for (i = 0; i < smm_info->number_of_smm_regions; i++){
        ret += scnprintf(local_buf + ret, sizeof(local_buf) - ret,
                    "0x%llx\n", smm_info->descriptor[i].physical_start);
    }
    return sysfs_emit(buf, "%s", local_buf);
}

static ssize_t smram_subregions_state_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i;
    char local_buf[PAGE_SIZE];
    ssize_t ret = 0;

    for (i = 0; i < smm_info->number_of_smm_regions; i++){
    ret += scnprintf(local_buf + ret, sizeof(local_buf) - ret,
                    "0x%llx\n", smm_info->descriptor[i].region_state);
    }
    return sysfs_emit(buf, "%s", local_buf);
}

static struct kobj_attribute smm_info_tag_attribute = __ATTR(tag, 0400, smram_tag_out, NULL);
static struct kobj_attribute smm_info_size_attribute = __ATTR(size, 0400, smram_size_out, NULL);
static struct kobj_attribute smm_info_number_of_smm_regions_attribute = __ATTR(number_of_smm_regions, 0400, smram_nr_reg_out, NULL);
static struct kobj_attribute smm_info_subregions_size_attribute = __ATTR(subregions_size, 0400, smram_subregions_size_out, NULL);
static struct kobj_attribute smm_info_subregions_start_attribute = __ATTR(subregions_start, 0400, smram_subregions_start_out, NULL);
static struct kobj_attribute smm_info_subregions_state_attribute = __ATTR(subregions_state, 0400, smram_subregions_state_out, NULL);

static struct attribute *attrs[] = {
    &smm_info_tag_attribute.attr,
    &smm_info_size_attribute.attr,
    &smm_info_number_of_smm_regions_attribute.attr,
    &smm_info_subregions_size_attribute.attr,
    &smm_info_subregions_start_attribute.attr,
    &smm_info_subregions_state_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

struct kobject *smm_info_kobj;

static int smram_driver_probe(struct coreboot_device *dev)
{
    // the prints here are just for testing purposes, remove later
    pr_info("SMRAM info from cbtable");

    smm_info = &dev->smram_info;

    struct kobject *parent_kobj = kobject_get(firmware_kobj);

    smm_info_kobj = kobject_create_and_add("smram", parent_kobj);
    if (!smm_info_kobj) {
        pr_err("Failed to create kobj for smram\n");
        return -ENOMEM;
    }

    int ret;
    ret = sysfs_create_group(smm_info_kobj, &attr_group);

    if (ret) {
        kobject_put(smm_info_kobj);
    }

    pr_info("sysfs entries for SMRAM created\n");
    return ret;
}

static void smram_driver_remove(struct coreboot_device *dev)
{
    kobject_put(smm_info_kobj);
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
MODULE_DESCRIPTION("Driver for exporting SMRAM information from coreboot table to sysfs"); 
MODULE_LICENSE("GPL v2");
