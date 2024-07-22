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

#include "tas.h"

int backoff_base = 625;
int backoff_cap = 2500;
volatile int foo;

#ifdef __GNUC__
__inline__
#endif
void backoff(int *b)
{
    int i;
    for (i = *b; i; i--) {
#ifdef __GNUC__
        __asm__ __volatile__("nop");
#else
        foo++;
#endif
    }
    *b <<= 1;
    if (*b > backoff_cap) {
        *b = backoff_cap;
    }
}

void tatas_acquire_slowpath(tatas_lock *L)
{
    int b = backoff_base;
    do {
        backoff(&b);
        if (*L) continue;
    } while (tas(L));
}

bool tatas_timed_acquire_slowpath(tatas_lock *L, hrtime_t T)
{
    hrtime_t start = gethrtime();
    int b = backoff_base;
    do {
        if (gethrtime() - start > T) return false;
        backoff(&b);
        if (*L) continue;
    } while (tas(L));
    return true;
}

#ifndef __GNUC__

void tatas_acquire(tatas_lock *L)
{TATAS_ACQUIRE_BODY}

bool tatas_timed_acquire(tatas_lock *L, hrtime_t T)
{TATAS_TIMED_ACQUIRE_BODY}

void tatas_release(tatas_lock *L)
{TATAS_RELEASE_BODY}

#endif __GNUC__
