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

/*
    Code to manage a local but shared pool of MCS style qnodes.
    All nodes are allocated by, and used by, a given thread, and may
    reside in local memory on an ncc-numa machine.  The nodes belonging
    to a given thread form a circular singly linked list.  The head
    pointer points to the node most recently successfully allocated.
    A thread allocates from its own pool by searching forward from the
    pointer for a node that's marked "unused".  A thread (any thread)
    deallocates a node by marking it "unused".
*/

#ifndef LOCAL_QNODES_H
#define LOCAL_QNODES_H

#include "main.h"
#include "inline.h"
#include "mcs.h"
#include "clh.h"

/*
    The declaration below adds a "next_in_pool" pointer after the
    regular data of a real qnode.  In practice we might be able to
    overlap these fields with the real qnode, being careful to make sure
    that serial numbers and the A-B-A problem were handled correctly in
    the non-blocking algorithms.  That's difficult to do in a generic
    way, though, and for experimental purposes I really want to share
    the qnode allocation/deallocation code.  I'm choosing to waste some
    space in order to simplify the code base.
*/
typedef struct local_qnode {
    union {
        /* This union must be at the *beginning* of the struct,
            and must have members of all types managed by this code.
            The union members are never accessed by name, but pointers
            to a local_qnode are casted to be pointers to objects of
            the various types represented by the union members, and we
            have to be able to do that without trashing the next and
            next_group pointers that follow. */
        clh_cc_qnode ccq;
        mcs2_try_qnode m2tq;
    } real_qnode;
    bool allocated;
    struct local_qnode *next_in_pool;
} local_qnode;

/* The code in thread_data.[ch] creates, for each thread, a
    local_head_node for every kind of node that thread allocates.
    That's one for each element of the union in the local_qnode type
    above.  Because the head node is accessed by only one thread, no
    synchronization or atomic ops are necessary.
*/
typedef struct local_head_node {
    local_qnode *try_this_one;     /* pointer into circular list */
    local_qnode initial_qnode;
} local_head_node;

extern void init_local_hn(local_head_node *hp);

#define ALLOC_LOCAL_QNODE_BODY \
    local_qnode *p = hp->try_this_one;                          \
    if (!p->allocated) {                                        \
        p->allocated = true;                                    \
        return p;                                               \
    } else {                                                    \
        local_qnode *q = p->next_in_pool;                       \
        while (q != p) {                                        \
            if (!q->allocated) {                                \
                hp->try_this_one = q;                           \
                q->allocated = true;                            \
                return q;                                       \
            } else {                                            \
                q = q->next_in_pool;                            \
            }                                                   \
        }                                                       \
        /* Oops! all qnodes are in use.                         \
            Allocate a new one and link into list. */           \
        special_events[mallocs]++;                              \
        q = (local_qnode *) malloc(sizeof(local_qnode));        \
        q->next_in_pool = p->next_in_pool;                      \
        p->next_in_pool = q;                                    \
        hp->try_this_one = q;                                   \
        q->allocated = true;                                    \
        return q;                                               \
    }

INLINE local_qnode *alloc_local_qnode(local_head_node *hp)
BODY({ALLOC_LOCAL_QNODE_BODY})

#define free_local_qnode(p) ((p)->allocated = false)

#endif LOCAL_QNODES_H
