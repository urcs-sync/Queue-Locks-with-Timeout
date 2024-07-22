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

#ifndef MAIN_H
#define MAIN_H

typedef unsigned int bool;
#define true 1
#define false 0

extern bool main_uses_cc;

typedef enum {
    available,
    waiting,
    leaving,
    transient,
    recycled
} qnode_status;

enum {FIFO_windows, mallocs, NUM_SPECIALS};
extern volatile unsigned long special_events[NUM_SPECIALS];
extern char *special_names[];

/**********************************************************************/

#define compare_and_store(p,o,n) \
    (cas((volatile unsigned long *) (p), \
         (unsigned long) (o), \
         (unsigned long) (n)) == (unsigned long) (o))

#define s_swap(p,v) \
    (qnode_status) swap((volatile unsigned long *) (p), \
        (unsigned long) (v))

#define s_compare_and_swap(p,o,n) \
    (qnode_status) cas((volatile unsigned long *) (p), \
         (unsigned long) (o), \
         (unsigned long) (n))

#endif MAIN_H
