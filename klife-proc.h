#ifndef __KLIFE_PROC_H__
#define __KLIFE_PROC_H__

#include "klife.h"

#define KLIFE_PROC_ROOT "klife"
#define KLIFE_PROC_VERSION "version"
#define KLIFE_PROC_STATUS "status"


extern int proc_register (struct klife_status *klife);
extern int proc_free (void);

#endif
