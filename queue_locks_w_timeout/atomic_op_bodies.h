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

#ifndef ATOMIC_OP_BODIES_H
#define ATOMIC_OP_BODIES_H

/* Atomic ops */
/* inspired by Linux kernel smp.c, semaphore.c */

#define MEMBAR_BODY \
    __asm__ __volatile__("membar #StoreLoad");

#define SWAP_BODY \
    __asm__ __volatile__("                                          \
            swap [%2], %0                                           \
"               : "=&r"(val)                                        \
                : "0"(val), "r"(ptr)                                \
                : "memory");                                        \
    return val;

#define CAS_BODY \
    __asm__ __volatile__("                                          \
            cas [%2], %3, %0                                        \
"               : "=&r"(new)                                        \
                         : "0"(new), "r"(ptr), "r"(old)             \
                         : "memory");                               \
    return new;

/*  Sadly, gcc doesn't seem to understand Sparc extended (64-bit)
    registers.  I have to gather bits into registers and split them
    out again. */
#define CASX_BODY \
    bool success;                                                   \
    __asm__ __volatile__("                                          \
            sllx %1, 32, %%l4;                                      \
            or   %%l4, %2, %%l4;                                    \
            sllx %3, 32, %%l5;                                      \
            or   %%l5, %4, %%l5;                                    \
            casx [%5], %%l4, %%l5;                                  \
            cmp  %%l4, %%l5;                                        \
            be,pt %%xcc,1f;                                         \
            mov  1, %0;                                             \
            mov  %%g0, %0;                                          \
        1:                                                          \
"                : "=r"(success)                                    \
                 : "r"(expected_high), "r"(expected_low),           \
                   "r"(new_high), "r"(new_low),                     \
                   "r"(ptr)                                         \
                 : "l4", "l5", "memory");                           \
    return success;

/*  Swap is a deprecated instruction in the Sparc V9 instruction set,
    and as such has no 64-bit version.  The only way to get the effect
    of a 64-bit swap is with CASX in a loop. */
#define SWAPX_BODY \
    __asm__ __volatile__("                                          \
            ldx  [%3], %%l4;                                        \
            sllx %0, 32, %%l3;                                      \
        1:  or   %%l3, %1, %%l5;                                    \
            casx [%3], %%l4, %%l5;                                  \
            cmp  %%l4, %%l5;                                        \
            bne,pn %%xcc,1b;                                        \
            mov  %%l5, %%l4;                                        \
            stx  %%l4, [%2]                                         \
"               :: "r"(new_high), "r"(new_low),                     \
                   "r"(old), "r"(ptr)                               \
                 : "l3", "l4", "l5", "memory");

#define MVX_BODY \
    __asm__ __volatile__("                                          \
            ldx  [%0], %%l4;                                        \
            stx  %%l4, [%1]                                         \
"               :: "r"(from), "r"(to)                               \
                 : "l4", "memory");

#define TAS_BODY \
    unsigned long result;                                           \
    __asm__ __volatile__("                                          \
            ldstub [%1], %0                                         \
"           : "=r"(result)                                          \
            : "r"(ptr)                                              \
            : "memory");                                            \
    return result;

#define FAI_BODY \
    unsigned long found = *ptr;                                     \
    unsigned long expected;                                         \
    do {                                                            \
        expected = found;                                           \
    } while ((found = cas(ptr, expected, expected+1)) != expected); \
    return found;

#endif ATOMIC_OP_BODIES_H
