// SPDX-License-Identifier: GPL-2.0-only
/*
 * spi_info.c
 *
 * Driver for exporting SPI flash information structure from coreboot table to sysfs.
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

static struct lb_pld_spi_flash_info *spi_info;

static ssize_t spi_info_tag_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", spi_info->tag);
}

static ssize_t spi_info_size_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", spi_info->size);
}

static ssize_t spi_info_rev_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", spi_info->revision);
}

static ssize_t spi_info_flags_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", spi_info->flags);
}

static ssize_t spi_info_reg_id_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", spi_info->spi_address.register_id);
}

static ssize_t spi_info_reg_aid_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", spi_info->spi_address.address_space_id);
}

static ssize_t spi_info_reg_wth_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", spi_info->spi_address.register_bit_width);
    
}

static ssize_t spi_info_reg_off_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", spi_info->spi_address.register_bit_offset);
    
}

static ssize_t spi_info_reg_value_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", spi_info->spi_address.value);
    
}

static ssize_t spi_info_reg_addr_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%llx\n", spi_info->spi_address.address);
}

static struct kobj_attribute spi_info_tag_attribute = __ATTR(tag, 0400, spi_info_tag_out, NULL);
static struct kobj_attribute spi_info_size_attribute = __ATTR(size, 0400, spi_info_size_out, NULL);
static struct kobj_attribute spi_info_rev_attribute = __ATTR(revision, 0400, spi_info_rev_out, NULL);
static struct kobj_attribute spi_info_flags_attribute = __ATTR(flags, 0400, spi_info_flags_out, NULL);
static struct kobj_attribute spi_info_reg_id_attribute = __ATTR(id, 0400, spi_info_reg_id_out, NULL);
static struct kobj_attribute spi_info_reg_aid_attribute = __ATTR(address_id, 0400, spi_info_reg_aid_out, NULL);
static struct kobj_attribute spi_info_reg_wth_attribute = __ATTR(bit_width, 0400, spi_info_reg_wth_out, NULL);
static struct kobj_attribute spi_info_reg_off_attribute = __ATTR(bit_offset, 0400, spi_info_reg_off_out, NULL);
static struct kobj_attribute spi_info_reg_value_attribute = __ATTR(values, 0400, spi_info_reg_value_out, NULL);
static struct kobj_attribute spi_info_reg_addr_attribute = __ATTR(addresse, 0400, spi_info_reg_addr_out, NULL);

static struct attribute *attrs[] = {
    &spi_info_tag_attribute.attr,
    &spi_info_size_attribute.attr,
    &spi_info_rev_attribute.attr,
    &spi_info_flags_attribute.attr,
    &spi_info_reg_id_attribute.attr,
    &spi_info_reg_aid_attribute.attr,
    &spi_info_reg_wth_attribute.attr,
    &spi_info_reg_off_attribute.attr,
    &spi_info_reg_value_attribute.attr,
    &spi_info_reg_addr_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

struct kobject *spi_info_kobj;

static int spi_info_driver_probe(struct coreboot_device *dev)
{
    pr_info("SPI flash info from cbtable\n");
    spi_info = &dev->spi_info;

    struct kobject *parent_kobj = kobject_get(firmware_kobj);

    spi_info_kobj = kobject_create_and_add("spi_info", parent_kobj);
    if (!spi_info_kobj) {
        pr_err("Failed to create kobj for spi\n");
        return -ENOMEM;
    }

    int ret;
    ret = sysfs_create_group(spi_info_kobj, &attr_group);

    if (ret) {
        kobject_put(spi_info_kobj);
    }

    pr_info("sysfs entries for spi struct created\n");
    return ret;
}

static void spi_info_driver_remove(struct coreboot_device *dev)
{
    kobject_put(spi_info_kobj);
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
MODULE_DESCRIPTION("Driver for exporting SPI flash information structure information from coreboot table to sysfs"); 
MODULE_LICENSE("GPL v2");
