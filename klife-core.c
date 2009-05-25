#include "klife.h"

#include <linux/kernel.h>
#include <linux/slab.h>


int klife_create_board (char *name)
{
	kfree (name);

	return 0;
}
