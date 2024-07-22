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

#include "local_qnodes.h"

void init_local_hn(local_head_node *hp)
{
    local_qnode *iq = &hp->initial_qnode;
    iq->allocated = false;
    hp->try_this_one = iq->next_in_pool = iq;
}

#ifndef __GNUC__

local_qnode * alloc_local_qnode(local_head_node* hp)
{
    ALLOC_LOCAL_QNODE_BODY
}

#endif __GNUC__
