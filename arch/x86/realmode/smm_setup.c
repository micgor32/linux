#include <asm/setup.h>

void smm_init(size_t);

int smm(size_t size)
{
	printk(KERN_INFO "start smm setup");
	smm_init(size);
	return 0;
}
