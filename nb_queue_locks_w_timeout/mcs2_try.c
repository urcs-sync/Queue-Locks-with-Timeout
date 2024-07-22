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
#include "local_qnodes.h"
#include "mcs.h"
#include "thread_data.h"

/* The following must not be valid pointer values: */
#define AVAILABLE ((mcs2_try_qnode_ptr) 0x1)
#define LEAVING   ((mcs2_try_qnode_ptr) 0x2)
#define TRANSIENT ((mcs2_try_qnode_ptr) 0x3)
    /* Thread puts TRANSIENT in its next pointer when it wants to leave
        but was unable to break the link to it from its precedessor. */

#define qn_swap(p,v) \
    (mcs2_try_qnode_ptr) swap((volatile unsigned long *) (p), \
        (unsigned long) (v))

#define s_swap(p,v) \
    (qnode_status) swap((volatile unsigned long *) (p), \
        (unsigned long) (v))

#define alloc_qnode() (mcs2_try_qnode_ptr) \
    alloc_local_qnode(&my_thread_data()->lhn)

#define free_qnode(p) free_local_qnode((local_qnode *) p)

void mcs2_try_acquire(mcs2_try_lock *L)
{
    mcs2_try_qnode *pred;
    mcs2_try_qnode *I = alloc_qnode();
    mcs2_try_qnode *pred_next;
        
    I->next = 0;
    pred = qn_swap(&L->tail, I);
    if (!pred) {
        L->lock_holder = I;
        return;
    }

    I->status = waiting;
    while (1) {
        pred_next = qn_swap(&pred->next, I);
        /* If pred_next is not nil then my predecessor tried to leave or
            grant the lock before I was able to tell it who I am.  Since
            it doesn't know who I am, it won't be trying to change my
            status word, and since its CAS on the tail pointer, if any,
            will have failed, it won't have reclaimed its own qnode, so
            I'll have to. */
        if (pred_next == AVAILABLE) {
            L->lock_holder = I;
            free_qnode(pred);
            return;
        } else if (pred_next == LEAVING) {
            mcs2_try_qnode *new_pred = pred->prev;
            free_qnode(pred);
            pred = new_pred;
        } else if (pred_next == TRANSIENT) {
            mcs2_try_qnode *new_pred = pred->prev;
            if (s_swap(&pred->status, recycled) != waiting) {
                free_qnode(pred);
            } /* else when new predecessor changes this to available,
                leaving, or transient it will find recycled, and will
                reclaim old predecessor's node. */
            pred = new_pred;
        } else {    /* pred_next == nil */
            while (1) {
                qnode_status stat = I->status;
                if (stat == available) {
                    L->lock_holder = I;
                    free_qnode(pred);
                    return;
                } else if (stat == leaving) {
                    mcs2_try_qnode *new_pred = pred->prev;
                    free_qnode(pred);
                    pred = new_pred;
                    I->status = waiting;
                    break;  /* exit inner loop; continue outer loop */
                } else if (stat == transient) {
                    mcs2_try_qnode *new_pred = pred->prev;
                    if (s_swap(&pred->status, recycled) != waiting) {
                        free_qnode(pred);
                    } /* else when new predecessor changes this to available,
                        leaving, or transient it will find recycled, and will
                        reclaim old predecessor's node. */
                    pred = new_pred;
                    I->status = waiting;
                    break;  /* exit inner loop; continue outer loop */
                } /* else stat == waiting; continue inner loop */
            }
        }
    }
}

bool mcs2_try_timed_acquire(mcs2_try_lock *L, hrtime_t T)
{
    mcs2_try_qnode *pred;
    mcs2_try_qnode *I = alloc_qnode();
    mcs2_try_qnode *pred_next;
    hrtime_t start;

    I->next = 0;
    pred = qn_swap(&L->tail, I);
    if (!pred) {
        L->lock_holder = I;
        return true;
    }

    start = START_TIME;

    I->status = waiting;
    while (1) {
        pred_next = qn_swap(&pred->next, I);
        /* If pred_next is not nil then my predecessor tried to leave or
            grant the lock before I was able to tell it who I am.  Since
            it doesn't know who I am, it won't be trying to change my
            status word, and since its CAS on the tail pointer, if any,
            will have failed, it won't have reclaimed its own qnode, so
            I'll have to. */
        if (pred_next == AVAILABLE) {
            L->lock_holder = I;
            free_qnode(pred);
            return true;
        } else if (!pred_next) {
            while (1) {
                qnode_status stat;
                if (CUR_TIME - start > T) {
                    /* timed out */
                    I->prev = pred;
                    if (compare_and_store(&pred->next, I, 0)) {
                        /* predecessor doesn't even know I've been here */
                        mcs2_try_qnode *succ = qn_swap(&I->next, LEAVING);
                        if (succ) {
                            if (s_swap(&succ->status, leaving) == recycled) {
                                /* Timing window: successor already saw
                                    my modified next pointer and
                                    declined to modify it.  Nobody is
                                    going to look at my successor node,
                                    but they will see my next pointer
                                    and know what happened. */
                                free_qnode(succ);
                            } /* else successor will reclaim me when it
                                sees my change to its status word. */
                        } else {
                            /* I don't seem to have a successor. */
                            if (compare_and_store(&L->tail, I, pred)) {
                                free_qnode(I);
                            } /* else a newcomer hit the timing window
                                or my successor is in the process of
                                leaving.  Somebody will discover I'm
                                trying to leave, and will free my qnode
                                for me. */
                        }
                    } else {
                        /* Predecessor is trying to leave or to give me
                            (or somebody) the lock.  It has a pointer to
                            my qnode, and is planning to use it.  I
                            can't wait for it to do so, so I can't free
                            my own qnode. */
                        mcs2_try_qnode *succ = qn_swap(&I->next, TRANSIENT);
                        if (succ) {
                            if (s_swap(&succ->status, transient) == recycled) {
                                /* Timing window: successor already saw
                                    my modified next pointer and
                                    declined to modify it.  Nobody is
                                    going to look at my successor node,
                                    but they will see my next pointer
                                    and know what happened. */
                                free_qnode(succ);
                            } /* else successor will reclaim me when it
                                sees my change to its status word. */
                        } /* else I don't seem to have a successor, and
                            nobody can wait for my status word to
                            change.  This is the pathological case where
                            we can temporarily require non-linear storage. */
                    }
                    return false;
                }
                stat = I->status;
                if (stat == available) {
                    L->lock_holder = I;
                    free_qnode(pred);
                    return true;
                } else if (stat == leaving) {
                    mcs2_try_qnode *new_pred = pred->prev;
                    free_qnode(pred);
                    pred = new_pred;
                    I->status = waiting;
                    break;  /* exit inner loop; continue outer loop */
                } else if (stat == transient) {
                    mcs2_try_qnode *new_pred = pred->prev;
                    if (s_swap(&pred->status, recycled) != waiting) {
                        free_qnode(pred);
                    } /* else when new predecessor changes this to available,
                        leaving, or transient it will find recycled, and will
                        reclaim old predecessor's node. */
                    pred = new_pred;
                    I->status = waiting;
                    break;  /* exit inner loop; continue outer loop */
                } /* else stat == waiting; continue inner loop */
            }
        } else if (CUR_TIME - start > T) {
            /* timed out */
            /* pred_next is LEAVING or TRANSIENT */
            /* pred->next is now I */
            /* Put myself in transient state, so some successor can
                eventually clean up. */
            mcs2_try_qnode *succ;
            /* put predecessor's next field back, as it would be if I
                had timed out in the inner while loop and been unable to
                update predecessor's next pointer: */
            pred->next = pred_next;
            I->status = (pred_next == LEAVING ? leaving : transient);
            I->prev = pred;
            succ = qn_swap(&I->next, TRANSIENT);
            if (succ) {
                if (s_swap(&succ->status, transient) == recycled) {
                    /* Timing window: successor already saw my modified next
                        pointer and declined to modify it.  Nobody is going
                        to look at my successor node, but they will see my
                        next pointer and know what happened. */
                    free_qnode(succ);
                } /* else successor will reclaim me when it sees my change
                    to its status word. */
            } /* else I don't seem to have a successor, and nobody can wait
                for my status word to change.  This is the pathological case
                where we can temporarily require non-linear storage. */
            return false;
        } else if (pred_next == LEAVING) {
            mcs2_try_qnode *new_pred = pred->prev;
            free_qnode(pred);
            pred = new_pred;
        } else if (pred_next == TRANSIENT) {
            mcs2_try_qnode *new_pred = pred->prev;
            if (s_swap(&pred->status, recycled) != waiting) {
                free_qnode(pred);
            } /* else when new predecessor changes this to available,
                leaving, or transient it will find recycled, and will
                reclaim old predecessor's node. */
            pred = new_pred;
        }
    }
}

void mcs2_try_release(mcs2_try_lock *L)
{
    mcs2_try_qnode *I = L->lock_holder;
    mcs2_try_qnode *succ = qn_swap(&I->next, AVAILABLE);
    /* As a general rule, I can't reclaim my own qnode on release
        because my successor may be leaving, in which case somebody is
        going to have to look at my next pointer to realize that the
        lock is available.  The one exception (when I *can* reclaim my
        own node) is when I'm able to change the lock tail pointer back
        to nil. */
    if (succ) {
        if (s_swap(&succ->status, available) == recycled) {
            /* Timing window: successor already saw my modified next
                pointer and declined to modify it.  Nobody is going to
                look at my successor node, but they will see my next
                pointer and know what happened. */
            free_qnode(succ);
        } /* else successor (old or new) will reclaim me when it sees my
            change to the old successor's status word. */
    } else {
        /* I don't seem to have a successor. */
        if (compare_and_store(&L->tail, I, 0)) {
            free_qnode(I);
        } /* else a newcomer hit the timing window or my successor is in
            the process of leaving.  Somebody will discover I'm giving
            the lock away, and will free my qnode for me. */
    }
}
