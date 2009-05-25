#include <linux/kernel.h>
#include <linux/proc_fs.h>

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

	spin_lock (&klife->lock);
	len = sprintf (page, "Boards: %d\nRunning: %d\nTotal ticks: %llu\n", klife->boards_count, klife->boards_running, klife->ticks);
	spin_unlock (&klife->lock);

	return proc_calc_metrics (page, start, off, count, eof, len);
}


static int proc_create_write (struct file *file, const char __user *buffer,
			      unsigned long count, void *data)
{
	return 0;
}


static int proc_destroy_write (struct file *file, const char __user *buffer,
			      unsigned long count, void *data)
{
	return 0;
}
