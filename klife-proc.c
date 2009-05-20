#include <linux/kernel.h>
#include <linux/proc_fs.h>

#include "klife-proc.h"

static struct proc_dir_entry *root;


static int proc_version_read (char *page, char **start, off_t off, int count, int *eof, void *data);


int proc_register ()
{
	struct proc_dir_entry *version;

	root = proc_mkdir (KLIFE_PROC_ROOT, NULL);
	if (!root)
		return 1;

	version = create_proc_read_entry (KLIFE_PROC_VERSION, 0644, root, &proc_version_read, NULL);

	return 0;
}


int proc_free ()
{
	remove_proc_entry (KLIFE_PROC_VERSION, root);
	remove_proc_entry (KLIFE_PROC_ROOT, NULL);
	return 0;
}


static int proc_version_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return 0;
}
