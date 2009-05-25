#include <linux/kernel.h>
#include <linux/proc_fs.h>

#include "klife.h"
#include "klife-proc.h"

static struct proc_dir_entry *root;


static int proc_version_read (char *page, char **start, off_t off, 
			      int count, int *eof, void *data);

static int proc_status_read (char *page, char **start, off_t off,
                             int count, int *eof, void *data);


int proc_register (struct klife_status *klife)
{
	struct proc_dir_entry *version, *status;

	root = proc_mkdir (KLIFE_PROC_ROOT, NULL);
	if (!root)
		return 1;

	version = create_proc_read_entry (KLIFE_PROC_VERSION, 0644, root, 
					  &proc_version_read, klife);
	status = create_proc_read_entry (KLIFE_PROC_STATUS, 0644, root,
					 &proc_status_read, klife);
	return 0;
}


int proc_free ()
{
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

	len = sprintf (page, "Boards: %d\nRunning: %d\nTotal ticks: %llu\n", klife->boards_count, klife->boards_running, klife->ticks);
	return proc_calc_metrics (page, start, off, count, eof, len);
}
