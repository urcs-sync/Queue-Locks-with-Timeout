/*  Copyright (c) Michael L. Scott, 2001, 2002

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*  This file implements a hack to get really fast access to
    thread-specific data.  We put stuff at the bottom of the stack, and
    align the stack so that if you zero out the low bits of the address
    of anything in the stack you get the address of the thread-specific
    stuff.
*/

#ifndef THREAD_DATA_H
#define THREAD_DATA_H

#include "main.h"
#include "local_qnodes.h"
#include "inline.h"
#include <sys/time.h>

extern void *new_stack(int tid);
extern size_t stack_size();

typedef struct {
    local_head_node lhn;
    int tid;
    hrtime_t recent_time;
    int time_countdown;
} thread_data;

#define STACK_SIZE          16384   /* 4K is too small */

INLINE thread_data *my_thread_data()
/* this code must match thread_data.c */
BODY({\
    int dummy; \
    return (thread_data *) (((unsigned long) (&dummy)) \
                            & ((unsigned long) (- ((int) STACK_SIZE)))); \
})

/* gethrtime takes something like 270ns (90 cycles) to call.  The
following macros arrange to *really* call it only 1 time out of ten */

#define START_TIME ( \
    my_thread_data()->time_countdown = 10, \
    my_thread_data()->recent_time = gethrtime())
#define CUR_TIME ( \
    my_thread_data()->time_countdown-- ? \
        my_thread_data()->recent_time : START_TIME)

#endif THREAD_DATA_H
