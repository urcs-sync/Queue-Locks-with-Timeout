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

#include "thread_data.h"

/*
    Allocate space for thread stack, aligned so we can easily find
    static data at the bottom.
    Call appropriate initializers for that static data.
*/
void *new_stack(int tid)
{
    thread_data *p = (thread_data *) memalign(STACK_SIZE, STACK_SIZE);
    void *stack = (void *) (p + 1);
    init_local_hn(&p->lhn);
    p->recent_time = p->time_countdown = 0;
        /* initialization not strictly necessary, but what the heck */
    p->tid = tid;
    return stack;
}

size_t stack_size()
{
    return STACK_SIZE - sizeof(thread_data);
}

#ifndef __GNUC__

thread_data *my_thread_data()
{
    int dummy;
    return (thread_data *) (((unsigned long) (&dummy))
                            & ((unsigned long) (- ((int) STACK_SIZE))));
}

#endif
