/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2012  Christian Urich

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "createnodes.h"

DM_DECLARE_NODE_NAME(CreateNodes, Modules)
CreateNodes::CreateNodes()
{
    std::vector<DM::View> data;
    data.push_back( DM::View("Nodes", DM::NODE, DM::WRITE));
    this->addData("sys",data);
}

void CreateNodes::run() {
    DM::System * sys = this->getData("sys");
    DM::View v = DM::View("Nodes", DM::NODE, DM::WRITE);

    for (int i = 0; i < 10000; i++) {

        DM::Node  n =  DM::Node(0,0,0);
        sys->addNode(n, v);
    }
}
