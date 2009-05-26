#include <linux/kernel.h>
#include <linux/module.h>

#include "klife.h"
#include "klife-proc.h"


struct klife_status klife;

static void klife_delete_boards (void);


static int klife_init (void)
{
	klife.lock = RW_LOCK_UNLOCKED;
	klife.boards_count = 0;
	klife.boards_running = 0;
	klife.next_index = 0;
	klife.ticks = 0UL;
	INIT_LIST_HEAD (&klife.boards);

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
	klife_delete_boards ();

#ifdef CONFIG_PROC_FS
	proc_free ();
#endif
	printk (KERN_INFO "klife module unloaded\n");
}


static void klife_delete_boards (void)
{
	struct list_head *list;
	struct klife_board *board;

	write_lock (&klife.lock);
	// iterate over all boards and free them
	list_for_each (list, &klife.boards) {
		board = list_entry (list, struct klife_board, next);
		klife_delete_board (board);
	}
	write_unlock (&klife.lock);
}


module_init (klife_init);
module_exit (klife_exit);

MODULE_AUTHOR ("Max Lapan <max.lapan@gmail.com>");
MODULE_DESCRIPTION ("Study module: life game simulator");
MODULE_LICENSE ("GPL");
