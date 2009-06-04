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


struct klife_board {
	rwlock_t lock;
	struct list_head next;

	/* generic information */
	int index;
	char* name;

	/* board operation mode and status */
	klife_board_mode_t mode;
	int enabled;

	/* board dimensions */
	int width;
	int height;

	/* board field's data */
	char *field;

	/* proc parent */
	struct proc_dir_entry *proc_entry;
};


int klife_create_board (char *name);
int klife_delete_board (struct klife_board *board);


/* Board's cell management */
int board_get_cell (struct klife_board *board, unsigned long x, unsigned long y);
int board_set_cell (struct klife_board *board, unsigned long x, unsigned long y);
int board_clear_cell (struct klife_board *board, unsigned long x, unsigned long y);
int board_toggle_cell (struct klife_board *board, unsigned long x, unsigned long y);

extern struct klife_status klife;

#endif
