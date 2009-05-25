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

	spin_lock (&klife.lock);
	board->name = name;
	board->index = klife.next_index;
	board->lock = SPIN_LOCK_UNLOCKED;
	INIT_LIST_HEAD (&board->next);

	if (unlikely (!klife.boards))
		klife.boards = board;
	else
		list_add (&board->next, &klife.boards->next);

	if (proc_create_board (board))
		goto err;

	klife.boards_count++;
	klife.next_index++;
	spin_unlock (&klife.lock);

	return 0;
err:
	spin_unlock (&klife.lock);
	list_del (&board->next);
	kfree (board);
	kfree (name);
	return -ENOMEM;
}



int klife_delete_board (struct klife_board *board)
{
	BUG_ON (!board);

	spin_lock (&board->lock);
	proc_delete_board (board);
	spin_unlock (&board->lock);

	spin_lock (&klife.lock);
	list_del (&board->next);
	spin_unlock (&klife.lock);

	return 0;
}
