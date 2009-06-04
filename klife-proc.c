#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/ctype.h>

#include "klife.h"
#include "klife-proc.h"

static struct proc_dir_entry *root;
static struct proc_dir_entry *boards;


static int proc_version_read (char *page, char **start, off_t off,
			      int count, int *eof, void *data);

static int proc_status_read (char *page, char **start, off_t off,
                             int count, int *eof, void *data);

static int proc_create_write (struct file *file, const char __user *buffer,
			      unsigned long count, void *data);

static int proc_destroy_write (struct file *file, const char __user *buffer,
			      unsigned long count, void *data);

/* board functions */
static int proc_board_name_read (char *page, char **start, off_t off,
				 int count, int *eof, void *data);

static int proc_board_mode_read (char *page, char **start, off_t off,
				 int count, int *eof, void *data);

static int proc_board_enabled_read (char *page, char **start, off_t off,
				    int count, int *eof, void *data);

static int proc_board_status_read (char *page, char **start, off_t off,
				   int count, int *eof, void *data);

static int proc_board_read (char *page, char **start, off_t off,
			    int count, int *eof, void *data);
static int proc_board_write (struct file *file, const char __user *buffer,
			     unsigned long count, void *data);


/* Utility functions */
typedef enum {
	REQ_SET,
	REQ_CLEAR,
	REQ_TOGGLE,
} change_request_kind_t;

static char* get_board_index_str (struct klife_board *board);

static inline const char* board_mode_as_string (klife_board_mode_t mode);
static inline const char* board_enabled_as_string (int enabled);

static int parse_change_request (char *data, unsigned long max_ofs, unsigned long *ofs,
				 change_request_kind_t *req, unsigned long *x, unsigned long *y);


/*
 * Top-level module proc entries
 */
int proc_register (struct klife_status *klife)
{
	struct proc_dir_entry *version, *status, *create, *destroy;

	root = proc_mkdir (KLIFE_PROC_ROOT, NULL);
	if (unlikely (!root))
		goto err;

	version = create_proc_read_entry (KLIFE_PROC_VERSION, 0644, root,
					  &proc_version_read, klife);
	status = create_proc_read_entry (KLIFE_PROC_STATUS, 0644, root,
					 &proc_status_read, klife);

	boards = proc_mkdir (KLIFE_PROC_BOARDS, root);
	if (unlikely (!boards))
		goto err;

	create = create_proc_entry (KLIFE_PROC_CREATE, 0644, boards);
	if (likely (create)) {
		create->write_proc = proc_create_write;
		create->data = klife;
	}
	else
		goto err;

	destroy = create_proc_entry (KLIFE_PROC_DESTROY, 0644, boards);
	if (likely (destroy)) {
		destroy->write_proc = proc_destroy_write;
		destroy->data = klife;
	}
	else
		goto err;

	return 0;
 err:
	proc_free ();
	return 1;
}


int proc_free ()
{
	remove_proc_entry (KLIFE_PROC_CREATE, boards);
	remove_proc_entry (KLIFE_PROC_DESTROY, boards);
	remove_proc_entry (KLIFE_PROC_BOARDS, root);
	remove_proc_entry (KLIFE_PROC_VERSION, root);
	remove_proc_entry (KLIFE_PROC_STATUS, root);
	remove_proc_entry (KLIFE_PROC_ROOT, NULL);
	return 0;
}


static int proc_calc_metrics (char *page, char **start, off_t off,
			      int count, int *eof, int len)
{
	if (len <= off+count)
		*eof = 1;
	*start = page + off;
	len -= off;
	if (len < 0)
		len = 0;
	if (len > count)
		len = count;
	return len;
}


static int proc_version_read (char *page, char **start, off_t off,
			      int count, int *eof, void *data)
{
	int len;

	len = sprintf (page, "Kernel Life game simulator %d.%d\n",
		       KLIFE_VER_MAJOR, KLIFE_VER_MINOR);
	return proc_calc_metrics (page, start, off, count, eof, len);
}


static int proc_status_read (char *page, char **start, off_t off,
			     int count, int *eof, void *data)
{
	int len;
	struct klife_status *klife = data;

	read_lock (&klife->lock);
	len = sprintf (page, "Boards: %d\nRunning: %d\nTotal ticks: %llu\n",
		       klife->boards_count, klife->boards_running, klife->ticks);
	read_unlock (&klife->lock);

	return proc_calc_metrics (page, start, off, count, eof, len);
}


static int proc_create_write (struct file *file, const char __user *buffer,
			      unsigned long count, void *data)
{
	size_t len;
	char* name = NULL;
	int ret;

	if (!count) {
		printk (KERN_WARNING "Error creating board, name is invalid\n");
		return -EINVAL;
	}

	/* copy board's name */
	name = kmalloc (count+1, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	/* initialize new board */
	len = count;
	len -= copy_from_user (name, buffer, count);
	if (len <= 0) {
		kfree (name);
		return -EINVAL;
	}

	while (len > 0 && name[len-1] == '\n')
		len--;
	name[len] = 0;
	printk (KERN_INFO "Create new board with name '%s'\n", name);
	ret = klife_create_board (name);
	if (ret)
		kfree (name);
	else
		ret = count;
	return ret;
}


static int proc_destroy_write (struct file *file, const char __user *buffer,
			      unsigned long count, void *data)
{
	return count;
}


/*
 * Board routines
 */
int proc_create_board (struct klife_board *board)
{
	char *name;
	struct proc_dir_entry *entry = NULL;

	BUG_ON (!board);
	write_lock (&board->lock);

	name = get_board_index_str (board);
	if (unlikely (!name))
		goto err;

	board->proc_entry = proc_mkdir (name, boards);
	kfree (name);

	if (unlikely (!board->proc_entry))
		goto err;

	entry = create_proc_read_entry (KLIFE_PROC_BRD_NAME, 0644, board->proc_entry,
					     &proc_board_name_read, board);
	if (unlikely (!entry))
		goto err;

	entry = create_proc_read_entry (KLIFE_PROC_BRD_STATUS, 0644, board->proc_entry,
				&proc_board_status_read, board);
	if (unlikely (!entry))
		goto err;

	entry = create_proc_read_entry (KLIFE_PROC_BRD_MODE, 0644, board->proc_entry,
				       &proc_board_mode_read, board);
	if (unlikely (!entry))
		goto err;

	entry = create_proc_read_entry (KLIFE_PROC_BRD_ENABLED, 0644, board->proc_entry,
					  &proc_board_enabled_read, board);
	if (unlikely (!entry))
		goto err;

	entry = create_proc_entry (KLIFE_PROC_BRD_BOARD, 0644, board->proc_entry);

	if (likely (entry)) {
		entry->write_proc = proc_board_write;
		entry->read_proc = proc_board_read;
		entry->data = board;
	}
	else
		goto err;

	write_unlock (&board->lock);
	return 0;

err:
	/* remove proc entries which was (probably) created */
	if (board->proc_entry)
		proc_delete_board (board);

	write_unlock (&board->lock);
	return 1;
}


static int proc_board_name_read (char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	struct klife_board *board = data;
	size_t len;

	read_lock (&board->lock);
	len = strlen (board->name);
	strncpy (page, board->name, count);
	strncat (page+len, "\n", count-len);
	len += 2;
	read_unlock (&board->lock);

	return proc_calc_metrics (page, start, off, count, eof, len);
}


/* Assume that board's spinlock held */
int proc_delete_board (struct klife_board *board)
{
	char* name = get_board_index_str (board);

	remove_proc_entry (KLIFE_PROC_BRD_BOARD, board->proc_entry);
	remove_proc_entry (KLIFE_PROC_BRD_NAME, board->proc_entry);
	remove_proc_entry (KLIFE_PROC_BRD_MODE, board->proc_entry);
	remove_proc_entry (KLIFE_PROC_BRD_ENABLED, board->proc_entry);
	remove_proc_entry (KLIFE_PROC_BRD_STATUS, board->proc_entry);
	remove_proc_entry (name, boards);
	kfree (name);

	return 0;
}


static char* get_board_index_str (struct klife_board *board)
{
	char *name;
	size_t len = 4;

	BUG_ON (!board);

	while (1) {
		name = kmalloc (len, GFP_KERNEL);

		if (unlikely (!name))
			goto err;

		if (snprintf (name, len, "%d", board->index)+1 > len) {
			kfree (name);
			len <<= 1;
		}
		else
			break;
	}

	return name;
err:
	return NULL;
}


static int proc_board_mode_read (char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	struct klife_board *board = data;
	int len;

	read_lock (&board->lock);
	len = snprintf (page, count, "%s\n", board_mode_as_string (board->mode));
	read_unlock (&board->lock);

	return proc_calc_metrics (page, start, off, count, eof, len);
}



static int proc_board_enabled_read (char *page, char **start, off_t off,
				    int count, int *eof, void *data)
{
	struct klife_board *board = data;
	int len;

	read_lock (&board->lock);
	len = snprintf (page, count, "%s\n", board_enabled_as_string (board->enabled));
	read_unlock (&board->lock);

	return proc_calc_metrics (page, start, off, count, eof, len);
}


static int proc_board_status_read (char *page, char **start, off_t off,
				   int count, int *eof, void *data)
{
	struct klife_board *board = data;
	int len;

	read_lock (&board->lock);
	len = snprintf (page, count, "Mode:\t\t%s\nEnabled:\t%s\nWidth:\t\t%d\nHeight:\t\t%d\n", board_mode_as_string (board->mode),
			board->enabled ? "yes" : "no", board->width, board->height);
	read_unlock (&board->lock);

	return proc_calc_metrics (page, start, off, count, eof, len);
}


static int proc_board_read (char *page, char **start, off_t off,
			    int count, int *eof, void *data)
{
/*	struct klife_board *board = data; */
	return 0;
}


/*
 * Routine parses and process change request.
 */
static int proc_board_write (struct file *file, const char __user *buffer,
			     unsigned long count, void *data)
{
	struct klife_board *board = data;
	change_request_kind_t req;
	unsigned long x, y;
	char *k_buf;
	unsigned long ofs = 0;

	k_buf = kmalloc (count, GFP_KERNEL);

	if (!k_buf)
		return -ENOMEM;

	x = y = 0;
	req = REQ_SET;
	count -= copy_from_user (k_buf, buffer, count);

	while (parse_change_request (k_buf, count, &ofs, &req, &x, &y)) {
		switch (req) {
		case REQ_SET:
			board_set_cell (board, x, y);
			break;
		case REQ_CLEAR:
			board_clear_cell (board, x, y);
			break;
		case REQ_TOGGLE:
			board_toggle_cell (board, x, y);
			break;
		}
	}

	kfree (k_buf);

	return ofs;
}


/*
 * Utility functions
 */
static inline const char* board_mode_as_string (klife_board_mode_t mode)
{
	switch (mode) {
	case KBM_STEP:
		return "step";
	case KBM_RUN:
		return "run";
	default:
		return "unknown";
	}
}


static inline const char* board_enabled_as_string (int enabled)
{
	switch (enabled) {
	case 0:
		return "0";
	default:
		return "1";
	}
}


/*
 * Skip spaces in buffer, Returns 1 if faced with non-space character,
 * or 0 if we faced the end of the buffer */
static inline int skip_spaces (char **p, const char *max_p)
{
	while (*p != max_p && isspace (**p)) {
		++(*p);
	}

	if (*p == max_p)
		return 0;
	else
		return 1;
}


/*
 * Routine parses one request at given position of buffer. If request
 * is processed, req structure filled and offset is updated.
 *
 * Every request occupy one line and can have the form:
 * 1. set X Y
 * 2. clear X Y
 * 3. toggle X Y
 *
 * Possible return value:
 * 1 - request parsed successfully,
 * 0 - request is invalid and skipped to next line
 */
static int parse_change_request (char *data, unsigned long max_ofs, unsigned long *ofs,
				 change_request_kind_t *req, unsigned long *x, unsigned long *y)
{
	static const struct {
		const char* cmd;
		change_request_kind_t kind;
	} table[] = {
		{ .cmd = "set ", .kind = REQ_SET },
		{ .cmd = "clear ", .kind = REQ_SET },
		{ .cmd = "toggle ", .kind = REQ_SET },
	};

	int ret = 0, i, len;
	char *p = data + *ofs;


	if (!skip_spaces (&p, data + max_ofs))
		goto finish;

	for (i = 0; i < sizeof(table) / sizeof(table[0]) && !ret; i++) {
		len = strlen (table[i].cmd);
		if (strncmp (p, table[i].cmd, len) == 0) {
			p += len;
			if (!skip_spaces (&p, data + max_ofs))
				goto finish;

			*req = table[i].kind;
			*x = simple_strtoul (p, &p, 10);

			if (!skip_spaces (&p, data + max_ofs))
				goto finish;

			*y = simple_strtoul (p, &p, 10);
			*ofs = p - data;

			printk (KERN_INFO "Parsed %s (%lu, %lu)\n", table[i].cmd, *x, *y);
			ret = 1;
		}
	}

 finish:
	/* search for newline or end of buffer */
	while (p != data + max_ofs && *p != '\n')
		p++;

	*ofs = p - data;

	return ret;
}
