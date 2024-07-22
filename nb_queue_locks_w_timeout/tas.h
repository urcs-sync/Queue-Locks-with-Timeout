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

#ifndef TAS_H
#define TAS_H

#include <sys/time.h>
#include "atomic_ops.h"
#include "main.h"
#include "inline.h"

typedef volatile unsigned long tatas_lock;

extern void tatas_acquire_slowpath(tatas_lock *L);
extern bool tatas_timed_acquire_slowpath(tatas_lock *L, hrtime_t T);

#define TATAS_ACQUIRE_BODY \
    if (tas(L)) { \
        tatas_acquire_slowpath(L); \
    }

#define TATAS_TIMED_ACQUIRE_BODY \
    if (tas(L)) { \
        return tatas_timed_acquire_slowpath(L, T); \
    } else { \
        return true; \
    }

#define TATAS_RELEASE_BODY \
    *L = 0;

INLINE void tatas_acquire(tatas_lock *L)
BODY({TATAS_ACQUIRE_BODY})

INLINE bool tatas_timed_acquire(tatas_lock *L, hrtime_t T)
BODY({TATAS_TIMED_ACQUIRE_BODY})

INLINE void tatas_release(tatas_lock *L)
BODY({TATAS_RELEASE_BODY})

#endif
