#ifndef __KLIFE_H__
#define __KLIFE_H__

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>

#define KLIFE_VER_MAJOR 0
#define KLIFE_VER_MINOR 1


struct klife_board;


struct klife_status {
	spinlock_t lock;
	int boards_count;
	int boards_running;
	int next_index;
	unsigned long long ticks;
	struct klife_board *boards;	/* linked list */
};


struct klife_board {
	spinlock_t lock;
	struct list_head next;
	int index;
	char* name;
	struct proc_dir_entry *proc_entry;
};


int klife_create_board (char *name);

extern struct klife_status klife;

#endif
