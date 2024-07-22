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

#include "clh.h"
#include "thread_data.h"

bool clh_try_swap_timed_acquire_slowpath(clh_cc_lock *L,
    clh_cc_qnode *I, clh_cc_qnode *pred, hrtime_t T)
{
    hrtime_t start = START_TIME;
    qnode_status stat;

    while (CUR_TIME - start < T) {
        stat = pred->status;
        if (stat == available) {
            I->prev = pred;
            return true;
        }
        if (stat == leaving) {
            clh_cc_qnode *temp = pred->prev;
            pred->status = recycled;
            pred = temp;
        }
        /* stat might also be transient, if somebody left the queue just
        before I entered. */
    }

    /* timed out */

    while (1) {
        do {
            stat = pred->status;
        } while (stat == transient);
        stat = s_swap(&pred->status, transient);
        if (stat == available) {
            I->prev = pred;
            return true;
        }
        if (stat == transient) continue;
        if (stat == leaving) {
            clh_cc_qnode *temp = pred->prev;
            pred->status = recycled;
            pred = temp;
            continue;
        }
        break;
    }

    I->prev = pred;     /* so successor can find it */
    /* indicate transient, so successor can't leave */
    while (1) {
        stat = s_swap(&I->status, transient);
        if (stat == waiting) break;
        while (I->status == transient);     /* spin */
    }

    /* if last process in line, link self out */
    if (*L == I) {
        /* I appear to be the end of the line */
        clh_cc_qnode *tail = clh_qn_swap(L, pred);
        if (tail == I) {
            /* I was indeed the end */
            pred->status = waiting;
            return false;
        }
        /* Else some other process(es) got into the timing window.
            I have the tail of that victim list.  The head has a pointer
            to me.  The tail can't leave because the lock doesn't point
            to it and nobody will mark its node recycled.  The head can't
            leave because my status is transient. */
        special_events[FIFO_windows]++;
        /* put victims back: */
        I->prev = clh_qn_swap(L, tail);
            /* Note that return from swap may not be pred, if yet more
            processes have entered the queue.  I need to arrange for the head
            of the victim list to link to this new predecessor, while I
            remember the old predecessor (whose transient flag I have yet to
            clear).  Note also that the new predecessor can't leave the queue
            because it isn't at the tail anymore. */
    }
    I->status = leaving;
        
    /* not last in line */
    while (1) {
        stat = I->status;
        if (stat == recycled) {
            pred->status = waiting;
            return false;
        }
    }
}

#ifndef __GNUC__

void clh_try_swap_acquire(clh_cc_lock *L, clh_cc_qnode *I)
{CLH_TRY_SWAP_ACQUIRE_BODY}

bool clh_try_swap_timed_acquire(clh_cc_lock *L, clh_cc_qnode *I, hrtime_t T)
{CLH_TRY_SWAP_TIMED_ACQUIRE_BODY}

void clh_try_swap_release(clh_cc_qnode **I)
{CLH_TRY_SWAP_RELEASE_BODY}

#endif __GNUC__
