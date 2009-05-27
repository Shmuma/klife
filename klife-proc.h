#ifndef __KLIFE_PROC_H__
#define __KLIFE_PROC_H__

#include "klife.h"

#define KLIFE_PROC_ROOT "klife"
#define KLIFE_PROC_VERSION "version"
#define KLIFE_PROC_STATUS "status"
#define KLIFE_PROC_BOARDS "boards"
#define KLIFE_PROC_CREATE "create"
#define KLIFE_PROC_DESTROY "destroy"

/* board entryes */
#define KLIFE_PROC_BRD_NAME "name"
#define KLIFE_PROC_BRD_MODE "mode"
#define KLIFE_PROC_BRD_STATUS "status"

extern int proc_register (struct klife_status *klife);
extern int proc_free (void);

extern int proc_create_board (struct klife_board *board);
extern int proc_delete_board (struct klife_board *board);

#endif
