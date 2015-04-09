/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * (c)Copyright 2006 Hewlett-Packard Development Company, LP.
 *
 */

#include "C_RandomExtDataCtrl.hpp"
#include <stdlib.h>
#include <unistd.h>
#include <cstdlib>

C_RandomExtDataCtrl::C_RandomExtDataCtrl()
:C_ExternalDataControl()
{
    srand(getpid());
}

size_t C_RandomExtDataCtrl::select_line () {

  if (m_line_selected_max == 0) {
    m_line_selected =  0 ;
  } else {
    m_line_selected =  rand() % m_line_selected_max ;
  }
  
  return (m_line_selected);

}
