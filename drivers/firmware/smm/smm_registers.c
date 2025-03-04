// SPDX-License-Identifier: GPL-2.0-only
/*
 * smm_registers.c
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

static struct lb_pld_smm_registers *smm_registers;

static ssize_t smm_regs_tag_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", smm_registers->tag);
}

static ssize_t smm_regs_size_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", smm_registers->size);
}

static ssize_t smm_regs_rev_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", smm_registers->revision);
}

static ssize_t smm_regs_count_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "0x%X\n", smm_registers->count);
}

static ssize_t smm_regs_reg_id_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i;
    char local_buf[PAGE_SIZE];
    ssize_t ret = 0;

    for (i = 0; i < smm_registers->count; i++){
        ret += scnprintf(local_buf + ret, sizeof(local_buf) - ret,
                    "0x%X\n", smm_registers->registers[i].register_id);
    }
    return sysfs_emit(buf, "%s", local_buf);
}

static ssize_t smm_regs_reg_aid_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i;
    char local_buf[PAGE_SIZE];
    ssize_t ret = 0;

    for (i = 0; i < smm_registers->count; i++){
        ret += scnprintf(local_buf + ret, sizeof(local_buf) - ret,
                    "0x%X\n", smm_registers->registers[i].address_space_id);
    }
    return sysfs_emit(buf, "%s", local_buf);
}
static ssize_t smm_regs_reg_wth_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i;
    char local_buf[PAGE_SIZE];
    ssize_t ret = 0;

    for (i = 0; i < smm_registers->count; i++){
        ret += scnprintf(local_buf + ret, sizeof(local_buf) - ret,
                    "0x%X\n", smm_registers->registers[i].register_bit_width);
    }
    return sysfs_emit(buf, "%s", local_buf);
}
static ssize_t smm_regs_reg_off_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i;
    char local_buf[PAGE_SIZE];
    ssize_t ret = 0;

    for (i = 0; i < smm_registers->count; i++){
        ret += scnprintf(local_buf + ret, sizeof(local_buf) - ret,
                    "0x%X\n", smm_registers->registers[i].register_bit_offset);
    }
    return sysfs_emit(buf, "%s", local_buf);
}
static ssize_t smm_regs_reg_value_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i;
    char local_buf[PAGE_SIZE];
    ssize_t ret = 0;

    for (i = 0; i < smm_registers->count; i++){
        ret += scnprintf(local_buf + ret, sizeof(local_buf) - ret,
                    "0x%X\n", smm_registers->registers[i].value);
    }
    return sysfs_emit(buf, "%s", local_buf);
}
static ssize_t smm_regs_reg_addr_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i;
    char local_buf[PAGE_SIZE];
    ssize_t ret = 0;

    for (i = 0; i < smm_registers->count; i++){
        ret += scnprintf(local_buf + ret, sizeof(local_buf) - ret,
                    "0x%llx\n", smm_registers->registers[i].address);
    }
    return sysfs_emit(buf, "%s", local_buf);
}


static struct kobj_attribute smm_regs_tag_attribute = __ATTR(tag, 0400, smm_regs_tag_out, NULL);
static struct kobj_attribute smm_regs_size_attribute = __ATTR(size, 0400, smm_regs_size_out, NULL);
static struct kobj_attribute smm_regs_rev_attribute = __ATTR(revision, 0400, smm_regs_rev_out, NULL);
static struct kobj_attribute smm_regs_count_attribute = __ATTR(count, 0400, smm_regs_count_out, NULL);
static struct kobj_attribute smm_regs_reg_id_attribute = __ATTR(ids, 0400, smm_regs_reg_id_out, NULL);
static struct kobj_attribute smm_regs_reg_aid_attribute = __ATTR(address_ids, 0400, smm_regs_reg_aid_out, NULL);
static struct kobj_attribute smm_regs_reg_wth_attribute = __ATTR(bit_widths, 0400, smm_regs_reg_wth_out, NULL);
static struct kobj_attribute smm_regs_reg_off_attribute = __ATTR(bit_offsets, 0400, smm_regs_reg_off_out, NULL);
static struct kobj_attribute smm_regs_reg_value_attribute = __ATTR(values, 0400, smm_regs_reg_value_out, NULL);
static struct kobj_attribute smm_regs_reg_addr_attribute = __ATTR(addresses, 0400, smm_regs_reg_addr_out, NULL);

static struct attribute *attrs[] = {
    &smm_regs_tag_attribute.attr,
    &smm_regs_size_attribute.attr,
    &smm_regs_rev_attribute.attr,
    &smm_regs_count_attribute.attr,
    &smm_regs_reg_id_attribute.attr,
    &smm_regs_reg_aid_attribute.attr,
    &smm_regs_reg_wth_attribute.attr,
    &smm_regs_reg_off_attribute.attr,
    &smm_regs_reg_value_attribute.attr,
    &smm_regs_reg_addr_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

struct kobject *regs_info_kobj;

static int smm_regs_driver_probe(struct coreboot_device *dev)
{
    pr_info("SMM registers info from cbtable\n");
    smm_registers = &dev->smm_registers;

    struct kobject *parent_kobj = kobject_get(firmware_kobj);

    regs_info_kobj = kobject_create_and_add("smm_registers", parent_kobj);
    if (!regs_info_kobj) {
        pr_err("Failed to create kobj for registers\n");
        return -ENOMEM;
    }

    int ret;
    ret = sysfs_create_group(regs_info_kobj, &attr_group);

    if (ret) {
        kobject_put(regs_info_kobj);
    }

    pr_info("sysfs entries for smm registres created\n");
    return ret;
}

static void smm_regs_driver_remove(struct coreboot_device *dev)
{
    kobject_put(regs_info_kobj);
}

static const struct coreboot_device_id smm_info_ids[] = {
    { .tag = CB_TAG_PLD_SMM_REGISTER_INFO },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(coreboot, smm_info_ids);

static struct coreboot_driver smram_driver = {
    .probe = smm_regs_driver_probe,
    .remove = smm_regs_driver_remove,
    .drv = {
        .name = "smm_registers",
    },
    .id_table = smm_info_ids,
};

module_coreboot_driver(smram_driver);

MODULE_AUTHOR("Michal Gorlas <michal.gorlas@9elements.com>");
MODULE_DESCRIPTION("Driver for exporting SMM-specific registers information from coreboot table to sysfs"); 
MODULE_LICENSE("GPL v2");
