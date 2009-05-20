#include <linux/kernel.h>
#include <linux/module.h>

#include "klife-proc.h"


static int klife_init (void)
{
#ifdef CONFIG_PROC_FS
	if (proc_register ()) {
		printk (KERN_WARNING "klife module failed to initialize /proc interface\n");
		return 1;
	}
#else
	printk (KERN_ERR "klife module needs /proc\n");
	return -ENODATA;
#endif
	printk (KERN_INFO "klife module initialized\n");
	return 0;
}


static void klife_exit (void)
{
#ifdef CONFIG_PROC_FS
	proc_free ();
#endif
	printk (KERN_INFO "klife module unloaded\n");
}


module_init (klife_init);
module_exit (klife_exit);

MODULE_AUTHOR ("Max Lapan <max.lapan@gmail.com>");
MODULE_DESCRIPTION ("Study module: life game simulator");
MODULE_LICENSE ("GPL");



