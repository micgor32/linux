#include <linux/console.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <asm/pci_x86.h>
#include <asm/realmode.h>

#define SMRAM		0x9d
#define   D_OPEN	(1 << 6)
#define   D_CLS		(1 << 5)
#define   D_LCK		(0 << 4)
#define   G_SMRAME	(0 << 3)
#define   C_BASE_SEG	((0 << 2) | (1 << 1) | (0 << 0))

static u8 longjmp0x30000x8000[] = { 0xEA, 0x00, 0x80, 0x00, 0x30 };
static u8 longjmpsmmhandler[] = { 0xEA, 0x00, 0x00, 0x00, 0x00 };

extern struct real_mode_header *real_mode_header;

static int smm_handler_copied = 0;

static void smm_install(size_t size)
{
	/*void __iomem *addr = ioremap((resource_size_t)0x38000, size);	*/
	/**/
	/*memcpy_toio(addr, __va(real_mode_header->smm_test), size);	*/
	/**/
	/*wbinvd();*/
}


/*void smm_test(void)*/
/*{*/
/*	printk("well here I am\n");*/
/*}*/

void smm_init(size_t size)
{
	//uintptr_t c = (uintptr_t)smm_test;

	printk("smm init \n");

	/* Put SMM code to 0xa0000 */
	//smm_install(size);
	
	//void __iomem *addr = ioremap((resource_size_t)0x7ca8d3ee, 4);
	//memcpy_toio(addr, (void *)smm_test, 4);

	/* Put relocation code to 0x38000 and relocate SMBASE */
	//smm_relocate();

	printk("lmao init DONE\n");
}

