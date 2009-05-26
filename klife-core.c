#include "klife.h"
#include "klife-proc.h"

#include <linux/kernel.h>
#include <linux/slab.h>


int klife_create_board (char *name)
{
	struct klife_board *board;

	board = kmalloc (sizeof (struct klife_board), GFP_KERNEL);

	if (!board)
		return -ENOMEM;

	write_lock (&klife.lock);
	board->name = name;
	board->index = klife.next_index;
	board->lock = RW_LOCK_UNLOCKED;
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



int klife_delete_board (struct klife_board *board)
{
	BUG_ON (!board);

	write_lock (&klife.lock);

	list_del (&board->next);

	write_lock (&board->lock);
	proc_delete_board (board);
	write_unlock (&board->lock);

	write_unlock (&klife.lock);

	return 0;
}
