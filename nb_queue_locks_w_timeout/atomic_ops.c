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
#include "main.h"
#include "atomic_op_bodies.h"
/* This file is always compiled with gcc. */

void membar()
{
    if (!main_uses_cc) {
        fprintf(stderr, "unexpected call to non-inlined membar\n");
        exit(1);
    }
    MEMBAR_BODY
}

unsigned long
    swap(volatile unsigned long *ptr, unsigned long val)
{
    if (!main_uses_cc) {
        fprintf(stderr, "unexpected call to non-inlined swap\n");
        exit(1);
    }
    SWAP_BODY
}

unsigned long
    cas(volatile unsigned long *ptr, unsigned long old, unsigned long new)
{
    if (!main_uses_cc) {
        fprintf(stderr, "unexpected call to non-inlined cas\n");
        exit(1);
    }
    CAS_BODY
}

bool casx(volatile unsigned long long *ptr,
     unsigned long expected_high, unsigned long expected_low,
     unsigned long new_high, unsigned long new_low)
{
    if (!main_uses_cc) {
        fprintf(stderr, "unexpected call to non-inlined casx\n");
        exit(1);
    }
    {
        CASX_BODY
    }
}

void swapx(volatile unsigned long long *ptr,
    unsigned long new_high, unsigned long new_low,
    unsigned long long *old)
{
    if (!main_uses_cc) {
        fprintf(stderr, "unexpected call to non-inlined swapx\n");
        exit(1);
    }
    {
        SWAPX_BODY
    }
}

void mvx(volatile unsigned long long *from,
    volatile unsigned long long *to)
{
    if (!main_uses_cc) {
        fprintf(stderr, "unexpected call to non-inlined mvx\n");
        exit(1);
    }
    MVX_BODY
}

unsigned long
    tas(volatile unsigned long *ptr)
{
    if (!main_uses_cc) {
        fprintf(stderr, "unexpected call to non-inlined tas\n");
        exit(1);
    }
    {
        TAS_BODY
    }
}

unsigned long
    fai(volatile unsigned long *ptr)
{
    if (!main_uses_cc) {
        fprintf(stderr, "unexpected call to non-inlined fai\n");
        exit(1);
    }
    {
        FAI_BODY
    }
}
