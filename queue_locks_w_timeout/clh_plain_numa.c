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

#include "atomic_ops.h"
#include "clh.h"

#ifndef __GNUC__

void clh_plain_numa_acquire(clh_numa_lock *L, clh_numa_qnode *I)
{CLH_PLAIN_NUMA_ACQUIRE_BODY}

void clh_plain_numa_release(clh_numa_qnode **I)
{CLH_PLAIN_NUMA_RELEASE_BODY}

#endif __GNUC__
