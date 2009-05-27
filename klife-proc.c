#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

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

static int proc_board_status_read (char *page, char **start, off_t off,
				   int count, int *eof, void *data);

/* Utility functions */
static char* get_board_index_str (struct klife_board *board);

static inline const char* board_mode_as_string (klife_board_mode_t mode);
static inline const char* board_status_as_string (klife_board_status_t status);


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


int proc_create_board (struct klife_board *board)
{
	char *name;
	struct proc_dir_entry *mode, *status;

	BUG_ON (!board);
	write_lock (&board->lock);

	name = get_board_index_str (board);
	if (unlikely (!name))
		goto err;

	board->proc_entry = proc_mkdir (name, boards);
	kfree (name);

	create_proc_read_entry (KLIFE_PROC_BRD_NAME, 0644, board->proc_entry,
				&proc_board_name_read, board);

	mode = create_proc_read_entry (KLIFE_PROC_BRD_MODE, 0644, board->proc_entry,
				       &proc_board_mode_read, board);

	status = create_proc_read_entry (KLIFE_PROC_BRD_STATUS, 0644, board->proc_entry,
					 &proc_board_status_read, board);

	write_unlock (&board->lock);
	return 0;

err:
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

	remove_proc_entry (KLIFE_PROC_BRD_NAME, board->proc_entry);
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



static int proc_board_status_read (char *page, char **start, off_t off,
				   int count, int *eof, void *data)
{
	struct klife_board *board = data;
	int len;

	read_lock (&board->lock);
	len = snprintf (page, count, "%s\n", board_status_as_string (board->status));
	read_unlock (&board->lock);

	return proc_calc_metrics (page, start, off, count, eof, len);
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


static inline const char* board_status_as_string (klife_board_status_t status)
{
	switch (status) {
	case KBS_DISABLED:
		return "disabled";
	case KBS_ENABLED:
		return "enabled";
	default:
		return "unknown";
	}
}
