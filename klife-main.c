#include <linux/kernel.h>
#include <linux/module.h>

#include "klife.h"
#include "klife-proc.h"


struct klife_status klife;


static int klife_init (void)
{
	klife.lock = SPIN_LOCK_UNLOCKED;
	klife.boards_count = 0;
	klife.boards_running = 0;
	klife.ticks = 0UL;

#ifdef CONFIG_PROC_FS
	if (proc_register (&klife)) {
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



