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
    return scnprintf(buf, PAGE_SIZE, "0x%X\n", smm_info->tag);
}

static ssize_t smram_size_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "0x%X\n", smm_info->size);
}

static ssize_t smram_nr_reg_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "%x\n", smm_info->number_of_smm_regions);
}

static struct kobj_attribute smm_info_tag_attribute = __ATTR(tag, 0400, smram_tag_out, NULL);
static struct kobj_attribute smm_info_size_attribute = __ATTR(size, 0400, smram_size_out, NULL);
static struct kobj_attribute smm_info_number_of_smm_regions_attribute = __ATTR(number_of_smm_regions, 0400, smram_nr_reg_out, NULL);

static struct kobject *subregions_kobj;

static int smram_driver_probe(struct coreboot_device *dev)
{
    pr_info("SMRAM info from cbtable");

    smm_info = &dev->smram_info;

    struct kobject *parent_kobj = kobject_get(firmware_kobj);
	struct kobject *smm_kobj = kobject_create_and_add("smm", parent_kobj);
	if (!smm_kobj) {
	    pr_err("Failed to create kobj for smm\n");
	    return -ENOMEM;
	}

    struct kobject *smm_info_kobj = kobject_create_and_add("smram", smm_kobj);
    if (!smm_info_kobj) {
        pr_err("Failed to create kobj for smram\n");
        kobject_put(smm_kobj);
        return -ENOMEM;
    }

    int ret = sysfs_create_file(smm_info_kobj, &smm_info_tag_attribute.attr);
    ret |= sysfs_create_file(smm_info_kobj, &smm_info_size_attribute.attr);
    ret |= sysfs_create_file(smm_info_kobj, &smm_info_number_of_smm_regions_attribute.attr);
    if (ret) {
        pr_err("Failed to create kobj for smram attrs\n");
        kobject_put(smm_info_kobj);
        kobject_put(smm_kobj);
        return ret;
    }

    subregions_kobj = kobject_create_and_add("subregion", smm_info_kobj);
    if (!subregions_kobj) {
        pr_err("Failed to create subdir for subregions \n");
        kobject_put(smm_info_kobj);
        kobject_put(smm_kobj);
        return -ENOMEM;
    }

    for (int i = 0; i < smm_info->number_of_smm_regions; i++) {
        struct kobject *region_kobj;
        char name[2]; // assuming there are no more than 99 subregions (which is not a case afaik)

        scnprintf(name, sizeof(name), "%d", i);
        region_kobj = kobject_create_and_add(name, subregions_kobj);
        if (!region_kobj) {
            pr_err("Failed to create region subdirectory for region %d\n", i);
            kobject_put(subregions_kobj);
            kobject_put(smm_info_kobj);
            kobject_put(smm_kobj);
            return -ENOMEM;
        }

        //ret = sysfs_create_file(region_kobj, &subregion_physical_start_attribute.attr);
        //ret |= sysfs_create_file(region_kobj, &subregion_physical_size_attribute.attr);
        //ret |= sysfs_create_file(region_kobj, &subregion_region_state_attribute.attr);
        /*if (ret) {*/
        /*    pr_err("Failed to create sysfs files for region %d\n", i);*/
        /*    kobject_put(region_kobj);*/
        /*    kobject_put(subregions_kobj);*/
        /*    kobject_put(smm_info_kobj);*/
        /*    kobject_put(smm_kobj);*/
        /*    return -ENOMEM;    */
        /*}*/
    }

    pr_info("sysfs entries for smmram created\n");

    return 0;
}

static void smram_driver_remove(struct coreboot_device *dev)
{
    pr_info("parser exit");
    // perform clean up here
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
