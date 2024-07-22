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

#ifndef ATOMIC_OPS_H
#define ATOMIC_OPS_H

#include "main.h"
#include "inline.h"
#include "atomic_op_bodies.h"

/********
    Someone who ought to know tells me that almost all Sun systems ever
    built implement TSO, and that those that don't have to be run in TSO
    mode in order for important software (like Solaris) to run correctly.
    This means in practice that the only time a memory barrier is needed
    is between a store and a subsequent load which must be ordered wrt
    one another.
 ********/
INLINE void membar()
BODY({MEMBAR_BODY})

INLINE unsigned long
    swap(volatile unsigned long *ptr, unsigned long val)
BODY({SWAP_BODY})

INLINE unsigned long
    cas(volatile unsigned long *ptr, unsigned long old, unsigned long new)
BODY({CAS_BODY})

INLINE bool
    casx(volatile unsigned long long *ptr,
         unsigned long expected_high,
         unsigned long expected_low,
         unsigned long new_high,
         unsigned long new_low)
BODY({CASX_BODY})

INLINE void
    swapx(volatile unsigned long long *ptr,
          unsigned long new_high,
          unsigned long new_low,
          unsigned long long *old)
BODY({SWAPX_BODY})

INLINE void
    mvx(volatile unsigned long long *from,
        volatile unsigned long long *to)
BODY({MVX_BODY})

INLINE unsigned long
    tas(volatile unsigned long *ptr)
BODY({TAS_BODY})

INLINE unsigned long
    fai(volatile unsigned long *ptr)
BODY({FAI_BODY})

#endif ATOMIC_OPS_H
