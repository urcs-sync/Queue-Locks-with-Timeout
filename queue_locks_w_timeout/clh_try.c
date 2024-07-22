/*  Copyright (c) Michael L. Scott and William Scherer III, 2001

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

bool clh_try_timed_acquire_slowpath(clh_cc_lock *L,
    clh_cc_qnode *I, clh_cc_qnode *pred, hrtime_t T)
{
    hrtime_t start = gethrtime();
    qnode_status stat;

    while (gethrtime() - start < T) {
        stat = pred->status;
        if (stat == available) {
            I->prev = pred;
            return true;
        }
        if (stat == leaving) {
            clh_cc_qnode *temp = pred->prev;
            /* tell predecessor that no one will ever look at its node
            again: */
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
            /* PREEMPTION VULNERABILITY: I'm stuck if successor is
                preempted while trying to leave */
        stat = s_swap(&pred->status, transient);
        if (stat == available) {
            I->prev = pred;
            return true;
        }
        if (stat == leaving) {
            clh_cc_qnode *temp = pred->prev;
            pred->status = recycled;
            pred = temp;
            continue;
        }
        break;
    }

/* assert:
if (stat != waiting) {
    fprintf(stderr, "Unexpected status in clh_timed_acquire (1): %d\n", stat);
    exit (1);
}
*/

    I->prev = pred;     /* so successor can find it */
    /* indicate leaving, so successor can't leave */
    while (1) {
        stat = s_compare_and_swap(&I->status, waiting, leaving);
        if (stat == waiting) break;

/* assert:
if (stat != transient) {
    fprintf(stderr, "Unexpected status in clh_timed_acquire (3): %d\n", stat);
    exit (1);
}
*/

        while (I->status != waiting);   /* spin */
            /* PREEMPTION VULNERABILITY: I'm stuck if successor is
                preempted while trying to leave */
    }

    /* if last process in line, link self out */
    if (compare_and_store(L, I, pred)) {
        pred->status = waiting;
        return false;
    }

    /* Here's why the transient flag is needed: Suppose we didn't have
    it.  Process B decides to leave.  It does a CAS in an attempt to
    change the lock tail pointer from itself back to A, its predecessor.
    But process C, its successor, exists, so the CAS fails.  But then
    C leaves, and completes its departure while B is still waiting for
    it to mark B's node recycled. */

    /* not last in line */
    while (1) {
        stat = I->status;
        if (stat == recycled) {
            pred->status = waiting;
            return false;
        }
/* assert:
        if (stat != leaving && stat != transient) {
            fprintf(stderr,
                "Unexpected status in clh_timed_acquire (2): %d\n", stat);
            exit(1);
        }
*/
        /* transient can happen above if successor starts to leave,
            swapping for my leaving, and then realizes that it has to
            skip over me */

        /* PREEMPTION VULNERABILITY: I'm stuck if successor is
            preempted while trying to leave */
    }
}

#ifndef __GNUC__

void clh_try_acquire(clh_cc_lock *L, clh_cc_qnode *I)
{CLH_TRY_ACQUIRE_BODY}

bool clh_try_timed_acquire(clh_cc_lock *L, clh_cc_qnode *I, hrtime_t T)
{CLH_TRY_TIMED_ACQUIRE_BODY}

void clh_try_release(clh_cc_qnode **I)
{CLH_TRY_RELEASE_BODY}

#endif __GNUC__
