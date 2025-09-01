// SPDX-License-Identifier: GPL-2.0

//!

use core::mem::ManuallyDrop;
use kernel::{
    acpi, c_str,
    device::{self, Core, DeviceContext},
    io::Resource,
    of, platform,
    prelude::*,
    types::ARef,
    uapi::resource_size_t,
};

#[repr(C)]
struct CorebootTableHeader {
    signature: [u8; 4],
    header_bytes: u32,
    header_checksum: u32,
    table_bytes: u32,
    table_checksum: u32,
    table_entries: u32,
}

impl CorebootTableHeader {
    fn new(pdev: &platform::Device, data: &[u8]) -> Result<Self> {
        if data.len() < size_of::<CorebootTableHeader> {
            dev_err!(pdev.as_ref(), "Not enough place for headaer");
            return Err(EINVAL);
        }

        let mut signature = [0u8, 4];
        signature.copy_from_slice(&data[0..4]);

        Ok(CorebootTableHeader {
            signature,
            header_bytes: u32::from_le_bytes([data[4], data[5]]),
            header_checksum: (),
            table_bytes: (),
            table_checksum: (),
            table_entries: (),
        })
    }
}

#[repr(C)]
struct CorebootTableEntry {
    tag: u32,
    size: u32,
}

struct Info(u32);

/* Points to a CBMEM entry */
#[repr(C)]
struct LbCbmemRef {
    tag: u32,
    size: u32,

    cbmem_addr: u64,
}

const LB_TAG_CBMEM_ENTRY: u32 = 0x31;

/* Corresponds to LB_TAG_CBMEM_ENTRY */
#[repr(C)]
struct LbCbmemEntry {
    tag: u32,
    size: u32,

    address: u64,
    entry_size: u32,
    id: u32,
}

// /* Describes framebuffer setup by coreboot */
#[repr(C)]
struct LbFramebuffer {
    tag: u32,
    size: u32,

    physical_address: u64,
    x_resolution: u32,
    y_resolution: u32,
    bytes_per_line: u32,
    bits_per_pixel: u8,
    red_mask_pos: u8,
    red_mask_size: u8,
    green_mask_pos: u8,
    green_mask_size: u8,
    blue_mask_pos: u8,
    blue_mask_size: u8,
    reserved_mask_pos: u8,
    reserved_mask_size: u8,
}

#[repr(C)]
union CorebootDeviceUnion {
    entry: ManuallyDrop<CorebootTableEntry>,
    cbmem_reg: ManuallyDrop<LbCbmemRef>,
    cbmem_entry: ManuallyDrop<LbCbmemEntry>,
    framebuffer: ManuallyDrop<LbFramebuffer>,
    raw: ManuallyDrop<KVec<u8>>,
}

/* A device, additionally with information from coreboot. */
#[repr(C)]
struct CorebootTable {
    dev: ARef<platform::Device>,
    //data: CorebootDeviceUnion,
}

kernel::of_device_table!(
    OF_TABLE,
    MODULE_OF_TABLE,
    <CorebootTable as platform::Driver>::IdInfo,
    [(of::DeviceId::new(c_str!("coreboot")), Info(42))] // or 42?
);

kernel::acpi_device_table!(
    ACPI_TABLE,
    MODULE_ACPI_TABLE,
    <CorebootTable as platform::Driver>::IdInfo,
    [(acpi::DeviceId::new(c_str!("BOOT0000")), Info(0))] //        [(acpi::DeviceId::new(c_str!("BOOT0000")), Info(0))]
);

// static inline struct coreboot_device *dev_to_coreboot_device(struct device *dev)
// {
// 	return container_of(dev, struct coreboot_device, dev);
// }

// /* A driver for handling devices described in coreboot tables. */
// struct coreboot_driver {
// 	int (*probe)(struct coreboot_device *);
// 	void (*remove)(struct coreboot_device *);
// 	struct device_driver drv;
// 	const struct coreboot_device_id *id_table;
// };

// /* use a macro to avoid include chaining to get THIS_MODULE */
// #define coreboot_driver_register(driver) \
// 	__coreboot_driver_register(driver, THIS_MODULE)
// /* Register a driver that uses the data from a coreboot table. */
// int __coreboot_driver_register(struct coreboot_driver *driver,
// 			       struct module *owner);

// /* Unregister a driver that uses the data from a coreboot table. */
// void coreboot_driver_unregister(struct coreboot_driver *driver);

// /* module_coreboot_driver() - Helper macro for drivers that don't do
//  * anything special in module init/exit.  This eliminates a lot of
//  * boilerplate.  Each module may only use this macro once, and
//  * calling it replaces module_init() and module_exit()
//  */
// #define module_coreboot_driver(__coreboot_driver) \
// 	module_driver(__coreboot_driver, coreboot_driver_register, \
// 			coreboot_driver_unregister)

// #endif /* __COREBOOT_TABLE_H */
impl platform::Driver for CorebootTable {
    type IdInfo = Info;
    const OF_ID_TABLE: Option<of::IdTable<Self::IdInfo>> = Some(&OF_TABLE);
    const ACPI_ID_TABLE: Option<acpi::IdTable<Self::IdInfo>> = Some(&ACPI_TABLE);

    fn probe(
        pdev: &platform::Device<Core>,
        info: Option<&Self::IdInfo>,
    ) -> Result<Pin<KBox<Self>>> {
        let len: resource_size_t;
        let dev = pdev.as_ref();
        let header: kernel::io::resource::Region;

        dev_info!(dev, "Probe Rust Platform driver sample.\n");

        if let Some(info) = info {
            dev_info!(dev, "Probed with info: '{}'.\n", info.0);
        }

        let is_res: Option<&Resource>; // Resource;
        is_res = pdev.resource_by_index(0);
        let res: &Resource = is_res.expect("device not found");

        dev_info!(
            dev,
            "hmmmm '{}'\n",
            res.name()
                .map_or("None", |s| s.to_str().unwrap_or_default())
        );

        if dev.fwnode().is_some_and(|node| node.is_of_node()) {
            Self::properties_parse(dev)?;
        } else {
            dev_info!(dev, "yyyy kurwa")
        }

        //let size = size_of::<CorebootTableHeader> as u64;
        // size_of::<CorebootTableHeader>.into::<u64>();
        // header = res
        //     .request_region(
        //         res.start(),
        //         size,
        //         res.name().expect("kurwa").to_cstring()?,
        //         res.flags(),
        //     )
        //     .expect("Header not found");

        let drvdata = KBox::new(Self { dev: pdev.into() }, GFP_KERNEL)?;

        Ok(drvdata.into())
    }
}

impl CorebootTable {
    //fn populate_table(dev: &device::Device) {}
    fn properties_parse(dev: &device::Device) -> Result {
        let fwnode = dev.fwnode().ok_or(ENOENT)?;

        if let Ok(idx) =
            fwnode.property_match_string(c_str!("compatible"), c_str!("test,rust-device"))
        {
            dev_info!(dev, "matched compatible string idx = {}\n", idx);
        }

        Ok(())
    }
}

impl Drop for CorebootTable {
    fn drop(&mut self) {
        dev_info!(self.dev.as_ref(), "Remove coreboot table driver.")
    }
}

kernel::module_platform_driver! {
    type: CorebootTable,
    name: "coreboot_table",
    authors: ["Me haha"],
    description: "coreboot table Rust implementation",
    license: "GPL v2",
}
