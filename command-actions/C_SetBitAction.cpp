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

#include "C_SetBitAction.hpp"
#include "GeneratorTrace.hpp"
#include "GeneratorError.h"
#include "BufferUtils.hpp"


C_SetBitAction::C_SetBitAction(T_CmdAction        P_cmdAction,
                                 T_pControllers P_controllers)
  : C_CommandAction (P_cmdAction, P_controllers) {
}


C_SetBitAction::~C_SetBitAction() {
}


T_exeCode    C_SetBitAction::execute(T_pCmd_scenario P_pCmd,
                                      T_pCallContext  P_callCtxt,
                                      C_MessageFrame *P_msg,
                                      C_MessageFrame *P_ref) {

  T_exeCode           L_exeCode    = E_EXE_NOERROR ;
  T_pValueData        L_mem                        ;
  int                 L_position_octet             ;
  int                 L_index_octet                ;
  int                 L_index                      ;
  unsigned char       L_car                        ;
  const size_t        L_cNum = 8                   ;


  L_mem = P_callCtxt->get_memory(m_mem_id);


  switch (L_mem->m_type) {

  case E_TYPE_NUMBER : {
    
    unsigned long  L_value = L_mem->m_value.m_val_number ;
    size_t         L_value_size = sizeof(L_value);
    unsigned char  L_buf [50] ;

    if (m_position > (int)(L_value_size*8)) {
      GEN_ERROR(E_GEN_FATAL_ERROR, "bad position value ["
                << m_position << "] Incoherent size with memory size ["
                << L_value_size
                << "] ");
      L_exeCode = E_EXE_ERROR ;
      break ;
    } else {
      convert_ul_to_bin_network(L_buf,
                                L_value_size,
                                L_value);
      
      L_index_octet    = (m_position / L_cNum) + 1 ; 
      L_position_octet = m_position % L_cNum ; 
      
      L_index = L_value_size - L_index_octet ;
      L_car = L_buf[L_index] ;
      
      if (m_field_data_num == 1) {
        L_buf[L_index] = (1<<L_position_octet) | L_car ;
      } else {
        L_buf[L_index] = ~(1<<L_position_octet) & L_car ;
      }
    
      L_mem->m_value.m_val_number
        = convert_bin_network_to_ul (L_buf, L_value_size);
    }

  }
  break ;
  
  case E_TYPE_STRING :

    if (L_mem->m_value.m_val_binary.m_size == 0) {
      GEN_LOG_EVENT(LOG_LEVEL_TRAFFIC_ERR,
                    action_name_table[m_type] 
                    << " memory zone that was not previously stored");
      
      GEN_LOG_EVENT(LOG_LEVEL_TRAFFIC_ERR,
                    " or memory stored is not STRING type");
      return (L_exeCode);
    }

    // ctrl number field
    if (m_position > (int)(L_mem->m_value.m_val_binary.m_size * 8)) {
      GEN_ERROR(E_GEN_FATAL_ERROR, "bad position value ["
                << m_position << "] Incoherent size with memory size ["
                << L_mem->m_value.m_val_binary.m_size
                << "] ");
      L_exeCode = E_EXE_ERROR ;
      break ;
    } else {
      L_index_octet    = (m_position / L_cNum) + 1 ; 
      L_position_octet = m_position % L_cNum ; 
      
      L_index = L_mem->m_value.m_val_binary.m_size - L_index_octet ;
      L_car = L_mem->m_value.m_val_binary.m_value[L_index] ;
      
      if (m_field_data_num == 1) {
        L_car = (1<<L_position_octet) | L_car ;
      } else {
        L_car = ~(1<<L_position_octet) & L_car ;
        
      }
      
      L_mem->m_value.m_val_binary.m_value[L_index] = L_car;
      
    }
    break ;

  default:
    GEN_LOG_EVENT(LOG_LEVEL_TRAFFIC_ERR,
                  "unsupported type for set-bit action: action only supports type string or number");
    L_exeCode = E_EXE_ERROR ;
    break;
      
  }
    


  return (L_exeCode);
}





