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
    Define INLINE "storage class" macro and BODY macro that allow us to
    inline separately compiled functions in gcc, but give them real
    bodies in Sun cc.
*/

#ifndef INLINE_H
#define INLINE_H

#ifdef __GNUC__
#define INLINE static __inline__
#define BODY(code) code
#else
#define INLINE extern
#define BODY(code) ;
#endif

#endif INLINE_H
