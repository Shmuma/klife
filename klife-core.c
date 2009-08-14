#include "klife.h"
#include "klife-proc.h"

#include <linux/kernel.h>
#include <linux/slab.h>


/*
 * Internal routines
 */
static inline int enlarge_needed (struct klife_board *board, unsigned long x, unsigned long y);
static int enlarge_field (struct klife_board *board, unsigned int new_side);
static inline unsigned int get_field_side (unsigned int pages_power);
static void copy_field (char *src, unsigned int src_side, char *dst, unsigned int dst_side);


/*
 * Internal macroses
 */
#define CELL_BYTE(x, y, width) (((y)*(width) + x) >> 3)
#define CELL_MASK(x) (1UL << ((x) & 0x7))


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
	free_pages ((unsigned long)board->field, board->pages_power);
	write_unlock (&board->lock);

	return 0;
}


/*
 * Board management routines
 */
int board_get_cell (struct klife_board *board, unsigned long x, unsigned long y)
{
	int res;

	read_lock (&board->lock);

	if (max (x, y) > board->side)
		return -EINVAL;

	res = board->field[CELL_BYTE (x, y, board->field_side)] & CELL_MASK (x);
	res = res ? 1 : 0;

	read_unlock (&board->lock);

	return res;
}


int board_set_cell (struct klife_board *board, unsigned long x, unsigned long y)
{
	int ret = 0;

	write_lock (&board->lock);

	if (enlarge_needed (board, x, y)) {
		printk (KERN_INFO "Enlarge needed to process set of cell %lu,%lu\n", x, y);
		ret = enlarge_field (board, max (x, y));
	}

	if (!ret) {
		/* check for side change */
		if (board->side < max (x, y))
			board->side = max (x, y) + 1;

/* 		printk (KERN_INFO "%lu,%lu, ofs = %lu, mask = %lx\n", x, y, CELL_BYTE (x, y, board->field_side << 3), CELL_MASK (x)); */
		board->field[CELL_BYTE (x, y, board->field_side)] |= CELL_MASK (x);
	}

	write_unlock (&board->lock);

	return 0;
}


int board_clear_cell (struct klife_board *board, unsigned long x, unsigned long y)
{
	int ret = 0;

	write_lock (&board->lock);

	if (enlarge_needed (board, x, y)) {
		printk (KERN_INFO "Enlarge needed to process set of cell %lu,%lu\n", x, y);
		ret = enlarge_field (board, max (x, y));
	}

	if (!ret)
		board->field[CELL_BYTE (x, y, board->field_side)] &= ~CELL_MASK (x);

	write_unlock (&board->lock);

	return 0;
}


int board_toggle_cell (struct klife_board *board, unsigned long x, unsigned long y)
{
	int ret = 0;

	write_lock (&board->lock);

	if (enlarge_needed (board, x, y)) {
		printk (KERN_INFO "Enlarge needed to process set of cell %lu,%lu\n", x, y);
		ret = enlarge_field (board, max (x, y));
	}

	if (!ret)
		board->field[CELL_BYTE (x, y, board->field_side)] ^= CELL_MASK (x);

	write_unlock (&board->lock);

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
	return board->field_side <= max(x, y);
}


/*
 * Realloc board's field to make it at least new_side side (in bits). Write lock must be held.
 *
 * Return 0 if succeeded, -ERROR otherwise.
 */
static int enlarge_field (struct klife_board *board, unsigned int new_side)
{
	unsigned int pages;
	unsigned int new_bytes, new_power, tmp, new_side_actual;
	char *new_buf;

	new_bytes = (new_side + 7) >> 3;
	pages = (new_bytes * (new_bytes << 3) + PAGE_SIZE - 1) / PAGE_SIZE;

	/* We know needed amount of pages to provide required board side, but we must find nearest
	 * greater 2^X. */
	new_power = 0;
	tmp = 1;
	while (tmp < pages) {
		new_power++;
		tmp <<= 1;
	}

	new_side_actual = get_field_side (new_power);

	printk (KERN_INFO "Enlarge field (requested side %u). %llu pages -> %llu pages. Result side %u\n",
		new_side, board->field ? (1ULL << board->pages_power) : 0, 1ULL << new_power, new_side_actual);

	new_buf = (char*)__get_free_pages (__GFP_ZERO | GFP_KERNEL, new_power);

	if (unlikely (!new_buf)) {
		printk (KERN_WARNING "Failed to allocate %llu pages\n", 1ULL << new_power);
		return -ENOMEM;
	}

	if (board->field) {
		/* now we must move existing data */
		copy_field (board->field, board->field_side >> 3, new_buf, new_side_actual >> 3);
		/* ok, free old board */
		free_pages ((unsigned long)board->field, board->pages_power);
	}

	board->field = new_buf;
	board->pages_power = new_power;
	board->field_side = new_side_actual;

	return 0;
}



/*
 * Routine calculates side of field which have 2^pages_power pages allocated.
 *
 * It calculates integer square root of 2^pages_power*PAGE_SIZE. Algorithm is borrowed from this
 * book: Henry S. Warren. Hacker's delight.
 */
#define UPDATE_APPROX(n1, s, bits)			\
	do {						\
		if (n1 > ((1ULL << bits) - 1)) {	\
			s += bits >> 1;			\
			n1 >>= bits;			\
		}					\
	} while (0);


static inline unsigned int get_field_side (unsigned int pages_power)
{
	unsigned long n = (1UL << pages_power) * PAGE_SIZE;
	unsigned int s, g0, g1;
	unsigned long n1;

	BUG_ON (pages_power+PAGE_SHIFT > 32);
	n <<= 3;

	pr_debug ("get_field_side: n = %lu, let's calculate isqrt of it\n", n);

	n1 = n - 1;
	s = 1;
	UPDATE_APPROX (n1, s, 16);
	UPDATE_APPROX (n1, s, 8);
	UPDATE_APPROX (n1, s, 4);
	UPDATE_APPROX (n1, s, 2);

	g0 = 1 << s;
	g1 = (g0 + (n >> s)) >> 1;

	pr_debug ("n1 = %lu, s = %u, g0 = %u, g1 = %u\n", n1, s, g0, g1);

	while (g1 < g0) {
		g0 = g1;
		g1 = (g0 + (n / g0)) >> 1;
		pr_debug ("g0 = %u, g1 = %u\n", g0, g1);
	}

	/* ok, we got isqrt of n in bits, but we must round this down to nearest byte */
	g0 = (g0 >> 3) << 3;
	pr_debug ("Result is %u\n", g0);

	return g0;
}


/*
 * Copy field data from source to dest, take in attention that dest is larger than src.
 * Dest is already filled with zeroes.
 */
static void copy_field (char *src, unsigned int src_side, char *dst, unsigned int dst_side)
{
	unsigned int x;

	for (x = 0; x < src_side; x++)
		memcpy (dst + x * dst_side, src + x * src_side, src_side);
}



/* Debug helpers */
void klife_dump_board (struct klife_board *board)
{
	unsigned long i, lim;

	read_lock (&board->lock);
	printk (KERN_INFO "\nKlife debug dump of board '%s', side %d:\n", board->name, board->field_side);

	printk (KERN_INFO "Field: ");

	if (!board->field)
		printk ("empty\n");
	else {
		printk ("\n");

		lim = (1UL << board->pages_power) * PAGE_SIZE;

		for (i = 0; i < lim; i++) {
			printk ("%02x ", (int)board->field[i]);
			if ((i+1) % (board->field_side >> 3) == 0)
				printk ("\n" KERN_INFO);
		}

		printk ("\n");
	}

	read_unlock (&board->lock);
}
