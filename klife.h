#ifndef __KLIFE_H__
#define __KLIFE_H__

#include <linux/kernel.h>

#define KLIFE_VER_MAJOR 0
#define KLIFE_VER_MINOR 1


typedef struct klife_status {
	int boards_count;
	int boards_running;
	unsigned long long ticks;
};


#endif
