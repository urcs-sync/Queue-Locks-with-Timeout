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

#include <stdio.h>
#include "clh.h"
#include "thread_data.h"

#define AVAILABLE ((clh_cc_qnode *) 1)
    /* must not be valid value for a pointer */

#define alloc_qnode() (clh_cc_qnode_ptr) \
    alloc_local_qnode(&my_thread_data()->lhn)

#define free_qnode(p) free_local_qnode((local_qnode *) p)

/* The routines in this file support a standard interface.
    Qnodes are allocated dynamically.
*/
void clh2_try_acquire(clh2_cc_lock *L)
{
    clh_cc_qnode *I = alloc_qnode();
    clh_cc_qnode *pred;

    I->prev = 0;
    pred = clh_qn_swap(&L->tail, I);
    if (!pred) {
        /* lock was free and uncontested; just return */
        L->lock_holder = I;
        return;
    }
    while (1) {
        clh_cc_qnode *pred_pred = pred->prev;
        if (pred_pred == AVAILABLE) {
            L->lock_holder = I;
            free_qnode(pred);
            return;
        } else if (pred_pred) {
            free_qnode(pred);
            pred = pred_pred;
        }
    }
}

bool clh2_try_timed_acquire(clh2_cc_lock *L, hrtime_t T)
{
    clh_cc_qnode *I = alloc_qnode();
    hrtime_t start;
    clh_cc_qnode *pred;

    I->prev = 0;
    pred = clh_qn_swap(&L->tail, I);
    if (!pred) {
        /* lock was free and uncontested; just return */
        L->lock_holder = I;
        return true;
    }
    
    if (pred->prev == AVAILABLE) {
        /* lock was free; just return */
        L->lock_holder = I;
        free_qnode(pred);
        return true;
    }
    start = START_TIME;

    while (CUR_TIME - start < T) {
        clh_cc_qnode *pred_pred = pred->prev;
        if (pred_pred == AVAILABLE) {
            L->lock_holder = I;
            free_qnode(pred);
            return true;
        } else if (pred_pred) {
            free_qnode(pred);
            pred = pred_pred;
        }
    }

    /* timed out; reclaim or abandon own node */

    if (compare_and_store(&L->tail, I, pred)) {
        /* last process in line */
        free_qnode(I);
    } else {
        I->prev = pred;
    }
    return false;
}

void clh2_try_release(clh2_cc_lock *L)
{
    clh_cc_qnode *I = L->lock_holder;
    if (compare_and_store(&L->tail, I, 0)) {
        /* no competition for lock; reclaim qnode */
        free_qnode(I);
    } else {
        I->prev = AVAILABLE;
    }
}
