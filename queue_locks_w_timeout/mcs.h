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

#ifndef MCS_H
#define MCS_H

#include <sys/time.h>
#include "atomic_ops.h"
#include "main.h"
#include "inline.h"

typedef struct mcs_plain_qnode {
    volatile bool waiting;
    struct mcs_plain_qnode *volatile next;
} mcs_plain_qnode;
typedef mcs_plain_qnode *volatile mcs_plain_qnode_ptr;
typedef mcs_plain_qnode_ptr mcs_plain_lock;

typedef struct mcs_try_qnode {
    struct mcs_try_qnode *volatile prev;
    struct mcs_try_qnode *volatile next;
} mcs_try_qnode;
typedef mcs_try_qnode *volatile mcs_try_qnode_ptr;
typedef mcs_try_qnode_ptr mcs_try_lock;

/**********************************************************************/

#define restored_tag    ((unsigned long) 0x1)
#define is_restored(p)  ((unsigned long) (p) & restored_tag)
#define is_transient(p) is_restored(p)

#define mcs_qn_swap(p,v) \
    (mcs_plain_qnode_ptr) swap ((volatile unsigned long *) (p), \
          (unsigned long) (v))

#define mcs_tqn_swap(p,v) \
    (mcs_try_qnode_ptr) swap ((volatile unsigned long *) (p), \
          (unsigned long) (v))

/**********************************************************************/

/* Parameter I, below, points to a qnode record allocated
   (in an enclosing scope) in shared memory locally-accessible
   to the invoking processor. */

/******** no timeout: ********/

#define MCS_PLAIN_ACQUIRE_BODY \
    mcs_plain_qnode *pred; \
    I->next = 0; \
    pred = mcs_qn_swap(L, I); \
    if (!pred) {                /* lock was free */ \
        return; \
    }                           /* else queue was non-empty */ \
    I->waiting = true;          /* word on which to spin */ \
    pred->next = I;             /* Make predecessor point to me. */ \
    while (I->waiting);         /* spin */

INLINE void mcs_plain_acquire (mcs_plain_lock *L, mcs_plain_qnode *I)
BODY({MCS_PLAIN_ACQUIRE_BODY})

#define MCS_PLAIN_RELEASE_BODY \
    mcs_plain_qnode *succ; \
    if (!(succ = I->next)) {    /* optimization */ \
        /* Try to fix global pointer. */ \
        if (compare_and_store(L, I, 0)) { \
            return;             /* I was last in line. */ \
        } \
        do { \
            succ = I->next; \
        } while (!succ);        /* wait for successor */ \
    } \
    succ->waiting = false;

INLINE void mcs_plain_release (mcs_plain_lock *L, mcs_plain_qnode *I)
BODY({MCS_PLAIN_RELEASE_BODY})

/******** blocking timeout, bounded space: ********/

extern void mcs_try_acquire_slowpath(mcs_try_lock *L,
    mcs_try_qnode *pred, mcs_try_qnode *I);

#define MCS_TRY_ACQUIRE_BODY \
    mcs_try_qnode *pred; \
    I->next = 0; \
    pred = mcs_tqn_swap(L, I); \
    if (!pred) {                  /* lock was free */ \
        return; \
    } else { \
        mcs_try_acquire_slowpath(L, pred, I); \
    }

INLINE void mcs_try_acquire(mcs_try_lock *L, mcs_try_qnode *I)
BODY({MCS_TRY_ACQUIRE_BODY})

extern bool mcs_try_timed_acquire_slowpath(mcs_try_lock *L,
    mcs_try_qnode *pred, mcs_try_qnode *I, hrtime_t T);

#define MCS_TRY_TIMED_ACQUIRE_BODY \
    mcs_try_qnode *pred; \
    I->next = 0; \
    pred = mcs_tqn_swap(L, I); \
    if (!pred) { \
        return true; \
    } else { \
        return mcs_try_timed_acquire_slowpath(L, pred, I, T); \
    }

INLINE bool mcs_try_timed_acquire(mcs_try_lock *L,
    mcs_try_qnode *I, hrtime_t T)
BODY({MCS_TRY_TIMED_ACQUIRE_BODY})

extern void mcs_try_release_slowpath(mcs_try_lock *L, mcs_try_qnode *I);

#define MCS_TRY_RELEASE_BODY \
    mcs_try_qnode *succ = I->next; \
    if (!succ) { \
        if (compare_and_store(L, I, 0)) { \
            mcs_try_qnode *temp; \
            do { \
                temp = I->next; \
            } while (is_transient(temp)); \
            return; \
        } \
    } \
    mcs_try_release_slowpath(L, I);

INLINE void mcs_try_release(mcs_try_lock *L, mcs_try_qnode *I)
BODY({MCS_TRY_RELEASE_BODY})

#endif MCS_H
