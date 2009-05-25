#include "klife.h"

#include <linux/kernel.h>
#include <linux/slab.h>


int klife_create_board (char *name)
{
	struct klife_board *board;

	board = kmalloc (sizeof (struct klife_board), GFP_KERNEL);

	if (!board)
		return -ENOMEM;

	spin_lock (&klife.lock);
	board->name = name;
	board->index = 0;
	board->lock = SPIN_LOCK_UNLOCKED;
	klife.boards_count++;
	spin_unlock (&klife.lock);

	return 0;
}
