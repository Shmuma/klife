#include "klife.h"
#include "klife-proc.h"

#include <linux/kernel.h>
#include <linux/slab.h>


/*
 * Internal routines
 */
static inline int enlarge_needed (struct klife_board *board, unsigned long x, unsigned long y);
static int enlarge_field (struct klife_board *board, unsigned int new_side);


/*
 * Internal macroses
 */

#define CELL_BYTE(x, y, width) (((y)*(width) + x) >> 3)
#define CELL_MASK(x) ((~0x7) & (x))


int klife_create_board (char *name)
{
	struct klife_board *board;

	board = kzalloc (sizeof (struct klife_board), GFP_KERNEL);

	if (!board)
		return -ENOMEM;

	write_lock (&klife.lock);
	board->name = name;
	board->index = klife.next_index;
	board->lock = RW_LOCK_UNLOCKED;
	board->mode = KBM_STEP;
	INIT_LIST_HEAD (&board->next);
	list_add (&board->next, &klife.boards);
	if (proc_create_board (board))
		goto err;

	klife.boards_count++;
	klife.next_index++;
	write_unlock (&klife.lock);

	return 0;
err:
	write_unlock (&klife.lock);
	list_del (&board->next);
	kfree (board);
	kfree (name);
	return -ENOMEM;
}



/* this routine assumes that klife status strucure lock is already held */
int klife_delete_board (struct klife_board *board)
{
	BUG_ON (!board);

	list_del (&board->next);

	write_lock (&board->lock);
	proc_delete_board (board);
	write_unlock (&board->lock);

	return 0;
}


/*
 * Board management routines
 */
int board_get_cell (struct klife_board *board, unsigned long x, unsigned long y)
{
	return 0;
}


int board_set_cell (struct klife_board *board, unsigned long x, unsigned long y)
{
	write_lock (&board->lock);

	if (enlarge_needed (board, x, y)) {
		printk (KERN_INFO "Enlarge needed to process set of cell %lu,%lu\n", x, y);
		enlarge_field (board, max (x, y) >> 3);
	}

	board->field[CELL_BYTE (x, y, board->field_side << 3)] |= CELL_MASK (x);

	write_unlock (&board->lock);

	return 0;
}


int board_clear_cell (struct klife_board *board, unsigned long x, unsigned long y)
{
	return 0;
}


int board_toggle_cell (struct klife_board *board, unsigned long x, unsigned long y)
{
	return 0;
}


/*
 * Internal routines
 */

/*
 * Routine checks that cell with given coordinates are inside of
 * allocated board's area. Assume that lock is held at least for reading.
 */
static inline int enlarge_needed (struct klife_board *board, unsigned long x, unsigned long y)
{
	return board->field_side*8 <= max(x, y);
}


/*
 * Realloc board's field to make it at least new_side side (in bytes).
 *
 * Return 0 if succeeded, -ERROR otherwise.
 */
static int enlarge_field (struct klife_board *board, unsigned int new_side)
{
/* 	unsigned int size = new_side * new_side; */
/* 	unsigned int new_pages; */
/* 	char *new_buf; */

/* 	new_pages = (size + (PAGE_SIZE-1)) / PAGE_SIZE; */

/* 	new_buf = __get_free_pages (__GFP_ZERO | GFP_KERNEL, ); */
}
