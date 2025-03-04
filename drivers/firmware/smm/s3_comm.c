// SPDX-License-Identifier: GPL-2.0-only
/*
 * s3_comm.c
 *
 * Driver for exporting SMM-specific register information from coreboot table to sysfs.
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

static struct lb_pld_s3_communication *s3_info;

static ssize_t s3_tag_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", s3_info->tag);
}

static ssize_t s3_size_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", s3_info->size);
}

static ssize_t s3_acpi_enable_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", s3_info->pld_acpi_s3_enable);
}

static ssize_t s3_buff_size_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%llx\n", s3_info->comm_buffer.physical_size);
}

static ssize_t s3_buff_start_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%llx\n", s3_info->comm_buffer.physical_start);
}

static ssize_t s3_buff_state_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%llx\n", s3_info->comm_buffer.region_state);
}

static struct kobj_attribute s3_info_tag_attribute = __ATTR(tag, 0400, s3_tag_out, NULL);
static struct kobj_attribute s3_info_size_attribute = __ATTR(size, 0400, s3_size_out, NULL);
static struct kobj_attribute s3_info_acpi_enable_attribute = __ATTR(acpi_s3_enable, 0400, s3_acpi_enable_out, NULL);
static struct kobj_attribute s3_info_buff_size_attribute = __ATTR(buff_size, 0400, s3_buff_size_out, NULL);
static struct kobj_attribute s3_info_buff_start_attribute = __ATTR(buff_start, 0400, s3_buff_start_out, NULL);
static struct kobj_attribute s3_info_buff_state_attribute = __ATTR(buff_state, 0400, s3_buff_state_out, NULL);

static struct attribute *attrs[] = {
    &s3_info_tag_attribute.attr,
    &s3_info_size_attribute.attr,
    &s3_info_acpi_enable_attribute.attr,
    &s3_info_buff_size_attribute.attr,
    &s3_info_buff_start_attribute.attr,
    &s3_info_buff_state_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

struct kobject *s3_comm_kobj;

static int s3_communications_driver_probe(struct coreboot_device *dev)
{
    // the prints here are just for testing purposes, remove later
    pr_info("S3 related info from cbtable");

    s3_info = &dev->s3_comm;

    struct kobject *parent_kobj = kobject_get(firmware_kobj);

    s3_comm_kobj = kobject_create_and_add("s3_communications", parent_kobj);
    if (!s3_comm_kobj) {
        pr_err("Failed to create kobj for s3_communications\n");
        return -ENOMEM;
    }

    int ret;
    ret = sysfs_create_group(s3_comm_kobj, &attr_group);

    if (ret) {
        kobject_put(s3_comm_kobj);
    }

    pr_info("sysfs entries for s3 comms created\n");
    return ret;
}

static void s3_communications_driver_remove(struct coreboot_device *dev)
{
    kobject_put(s3_comm_kobj);
}

static const struct coreboot_device_id s3_info_ids[] = {
    { .tag = CB_TAG_PLD_S3_COMMUNICATION },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(coreboot, s3_info_ids);

static struct coreboot_driver s3_communications_driver = {
    .probe = s3_communications_driver_probe,
    .remove = s3_communications_driver_remove,
    .drv = {
        .name = "s3_comm",
    },
    .id_table = s3_info_ids,
};

module_coreboot_driver(s3_communications_driver);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("Driver for exporting SMRAM information from coreboot table to sysfs"); 
MODULE_LICENSE("GPL v2");
