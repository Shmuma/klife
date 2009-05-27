#ifndef __KLIFE_H__
#define __KLIFE_H__

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>

#define KLIFE_VER_MAJOR 0
#define KLIFE_VER_MINOR 1


struct klife_board;


struct klife_status {
	rwlock_t lock;
	int boards_count;
	int boards_running;
	int next_index;
	unsigned long long ticks;
	struct list_head boards;
};


typedef enum {
	KBM_STEP,
	KBM_RUN,
} klife_board_mode_t;


typedef enum {
	KBS_DISABLED,
	KBS_ENABLED,
} klife_board_status_t;


struct klife_board {
	rwlock_t lock;
	struct list_head next;
	int index;
	char* name;
	klife_board_mode_t mode;
	klife_board_status_t status;
	struct proc_dir_entry *proc_entry;
};


int klife_create_board (char *name);
int klife_delete_board (struct klife_board *board);

extern struct klife_status klife;

#endif
