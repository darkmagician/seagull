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

#include "C_MessageBinary.hpp"
#include "BufferUtils.hpp"

#include "GeneratorTrace.hpp"
#include "GeneratorError.h"

#include "ProtocolData.hpp"

#include "ReceiveMsgContext.h"
#include "C_CallContext.hpp"
#include "C_ResponseTimeLog.hpp"


C_MessageBinary::C_MessageBinary(C_ProtocolBinary *P_protocol) {

  unsigned long L_nbFields, L_i ;

  GEN_DEBUG(1, "C_MessageBinary::C_MessageBinary() start");

   m_header_id = -1 ;
   m_nb_body_values = 0 ;

   
   ALLOC_TABLE(m_body_val, 
               C_ProtocolBinary::T_pBodyValue,
               sizeof(C_ProtocolBinary::T_BodyValue),
               MAX_BODY_VALUES);

   for(L_i = 0 ; L_i < MAX_BODY_VALUES ; L_i++) {
     m_body_val[L_i].m_id = -1 ;
   }
   
   m_protocol = P_protocol ;
   m_call_id = 0 ;
   L_nbFields = m_protocol->get_nb_field_header() ;
   ALLOC_TABLE(m_header_values, 
               T_pValueData, 
               sizeof(T_ValueData), 
               L_nbFields);
   
   for(L_i = 0 ; L_i < L_nbFields; L_i++) {
     // Reset Value Data ok because struct used
     memset(&m_header_values[L_i], 0, sizeof(T_ValueData));
     
     // Set the id in header value
     m_header_values[L_i].m_id = L_i ;
   }

   // Init m_id
   memset (&m_id, 0, sizeof(T_ValueData));
   m_id.m_type = E_TYPE_NUMBER ;
   
   m_header_body_field_separator = m_protocol->get_header_body_field_separator () ;
   
   m_session_id = NULL ;
   GEN_DEBUG(1, "C_MessageBinary::C_MessageBinary() end");
}

void C_MessageBinary::set_header_value (int P_id, unsigned long P_val) {

  GEN_DEBUG(1, "C_MessageBinary::set_header_value() start: P_id:" << P_id << " value ul:" << P_val);
  m_header_values[P_id].m_type = E_TYPE_NUMBER ;
  m_header_values[P_id].m_value.m_val_number = P_val ;
  GEN_DEBUG(1, "C_MessageBinary::set_header_value() end");

}

void C_MessageBinary::set_header_value (char* P_name, unsigned long P_val) {

  C_ProtocolBinary::T_pHeaderField L_desc ;
  int                        L_id   ;

  GEN_DEBUG(1, "C_MessageBinary::set_header_value() start: name:" << P_name << " value ul:" << P_val);
  L_desc = m_protocol -> get_header_field_description(P_name) ;
  if (L_desc != NULL) {
    L_id = L_desc -> m_id ;
    m_header_values[L_id].m_type = E_TYPE_NUMBER ;
    m_header_values[L_id].m_value.m_val_number = P_val ;
  }
  GEN_DEBUG(1, "C_MessageBinary::set_header_value() end");

}

void C_MessageBinary::set_header_value (char* P_name, char *P_val) {
  C_ProtocolBinary::T_pHeaderField L_desc ;
  int                        L_id   ;

  GEN_DEBUG(1, "C_MessageBinary::set_header_value() start: name:" << P_name << " value char:" << *P_val);
  L_desc = m_protocol -> get_header_field_description(P_name) ;
  if (L_desc != NULL) {
    L_id = L_desc -> m_id ;
    m_header_values[L_id].m_type = E_TYPE_NUMBER ;
    m_header_values[L_id].m_value.m_val_number = convert_char_to_ul(P_val) ;
  }
  GEN_DEBUG(1, "C_MessageBinary::set_header_value() end");
}

void C_MessageBinary::set_header_value (int P_id, T_pValueData P_val) {

  GEN_DEBUG(1, "C_MessageBinary::set_header_value() start: P_id:" << P_id );
  m_protocol->set_header_value(P_id, &(m_header_values[P_id]), P_val);
  GEN_DEBUG(1, "C_MessageBinary::set_header_value() end");
}


C_MessageBinary::~C_MessageBinary() {

  unsigned long L_i ;
  unsigned long L_nbFields;

  GEN_DEBUG(1, "\nC_MessageBinary::~C_MessageBinary() start: name " << name());
  m_call_id = 0 ;

  if (m_protocol->get_complex_header_presence())
  {
    L_nbFields = m_protocol->get_nb_field_header();
    GEN_DEBUG(2, "nb header fields = " << L_nbFields);

    for(L_i = 0; L_i < L_nbFields; L_i++) {
      m_protocol->delete_header_value(&m_header_values[L_i]);
      GEN_DEBUG(2, "delete_header value [" << L_i << "] OK");
    }
  }
  FREE_TABLE(m_header_values) ;
  GEN_DEBUG(2, "FREE_TABLE(m_header_values) OK\n");

  m_header_id = -1 ;
  
  GEN_DEBUG(2, "m_nb_body_values = " << m_nb_body_values);

  for(L_i = 0; L_i < (unsigned long)m_nb_body_values; L_i++) {
    m_protocol->delete_body_value(&m_body_val[L_i]);

    GEN_DEBUG(2, "delete body value [" << L_i << "] OK");
  }
  FREE_TABLE(m_body_val);
  GEN_DEBUG(2, "FREE_TABLE(m_body_val) OK");
  m_nb_body_values = 0 ;

  m_protocol->reset_value_data (&m_id);
  m_header_body_field_separator = NULL ;
  m_protocol = NULL ;

  if (m_session_id != NULL) {
    resetMemory(*m_session_id);
    FREE_VAR(m_session_id)  ;
  }

  GEN_DEBUG(1, "C_MessageBinary::~C_MessageBinary() end");

}

bool C_MessageBinary::update_fields (C_MessageFrame* P_msg) {
  return (true);
}

bool C_MessageBinary::compare_types (C_MessageFrame* P_msg) {

  bool             L_result ;
  C_MessageBinary *L_msg ;

  L_msg = dynamic_cast<C_MessageBinary*>(P_msg) ;

  GEN_DEBUG(1, "C_MessageBinary::compare_types() start");
  if (L_msg == NULL) {
    L_result = false ;
  } else {
    GEN_DEBUG(1, "C_MessageBinary::compare_types() get_type() " << get_type());
    L_result = (L_msg -> get_type()  == get_type()) ? true : false ;
    GEN_DEBUG(1, "C_MessageBinary::compare_types()  L_result of compare the type of message" << L_result);
  }
  GEN_DEBUG(1, "C_MessageBinary::compare_types() end");

  return (L_result) ;
}

unsigned long  C_MessageBinary::get_type () {

  int           L_idx = m_protocol -> get_header_type_id() ;
  unsigned long L_type = 0;
  bool          L_found = false ;
  int           L_i ;

  GEN_DEBUG(1, "C_MessageBinary::get_type() start");

  if (L_idx != -1) {
      L_type = m_header_values[L_idx].m_value.m_val_number ;
      GEN_DEBUG(1, "C_MessageBinary::get_type() when L_idx != -1 the type is " 
		<< L_type );
  } else {
      L_idx = m_protocol -> get_header_type_id_body() ;
      GEN_DEBUG(1, "C_MessageBinary::get_type() the L_idx is " 
		<< L_idx);
      GEN_DEBUG(1, "C_MessageBinary::get_type() the m_nb_body_values is " 
		<< m_nb_body_values);

      for (L_i=0; L_i < m_nb_body_values ; L_i++) { // remenber: to be optimized
          if (m_body_val[L_i].m_id == L_idx) { 
	    L_found=true; 
	    break; 
	  }
      }
      
      GEN_DEBUG(1, "C_MessageBinary::get_type() the L_i is " 
		<< L_i) ;

      if (L_found == true) {
        L_type = m_body_val[L_i].m_value.m_val_number ;

        GEN_DEBUG(1, "C_MessageBinary::get_type()  L_type " << L_type << 
                  " and m_body_val[L_i].m_value.m_val_signed " 
                  << m_body_val[L_i].m_value.m_val_signed);
        GEN_DEBUG(1, "C_MessageBinary::get_type()  L_type " << L_type << 
                  " and m_body_val[L_i].m_value.m_val_number " 
                  << m_body_val[L_i].m_value.m_val_number);
      }
  }

  GEN_DEBUG(1, "C_MessageBinary::get_type() returns " << L_type);
  return (L_type) ;
}

void  C_MessageBinary::set_type (unsigned long P_type) {
  int L_idx = m_protocol -> get_header_type_id() ;
  GEN_DEBUG(1, "C_MessageBinary::set_type() start");
  m_header_values[L_idx].m_type = E_TYPE_NUMBER ;
  m_header_values[L_idx].m_value.m_val_number = P_type ;
  GEN_DEBUG(1, "C_MessageBinary::set_type() end");
}

unsigned long  C_MessageBinary::get_call_id () {
  GEN_DEBUG(1, "C_MessageBinary::get_call_id() start");
  GEN_DEBUG(1, "C_MessageBinary::get_call_id() end");
  return (m_call_id);
}


void C_MessageBinary::set_call_id (unsigned long P_id) {
  GEN_DEBUG(1, "C_MessageBinary::set_call_id() start");
  m_call_id = P_id ;
  GEN_DEBUG(1, "C_MessageBinary::set_call_id() end");
}

unsigned long C_MessageBinary::decode (unsigned char *P_buffer, 
				       size_t         P_size,
				       C_ProtocolFrame::T_pMsgError P_error) {
  
  unsigned char *L_ptr = P_buffer ;
  unsigned long  L_ret = 0 ; 
  unsigned long  L_size_to_decode = 0 ;
  unsigned long  L_body_size = 0 ;
  int            L_ret_decode ;

  GEN_DEBUG(1, "C_MessageBinary::decode() start");
  L_size_to_decode = m_protocol->get_header_size() ;

  GEN_DEBUG(1, "C_MessageBinary::decode() header size [" << L_size_to_decode << "]");
  // first decode message size 
  
  if (P_size < L_size_to_decode) {

    // segmentation case
    *P_error = C_ProtocolFrame::E_MSG_ERROR_DECODING_SIZE_LESS ;
    GEN_DEBUG(1, "C_MessageBinary::decode() size < header");

  } else {
    L_ptr += L_size_to_decode ;

    if (m_protocol->get_msg_length_start_detected()) {
      L_body_size 
        = m_protocol->decode_msg_size (P_buffer, P_size)
        - L_size_to_decode + m_protocol->get_msg_length_start() ;
    } else {
      if(m_protocol->get_header_length_excluded () ) {
        L_body_size 
          = m_protocol->decode_msg_size (P_buffer, P_size) ;
      } else {
        L_body_size 
          = m_protocol->decode_msg_size (P_buffer, P_size)
          - L_size_to_decode ;
      }
    }

    GEN_DEBUG(1, "C_MessageBinary::decode() body size [" << L_body_size << "]");

    L_size_to_decode += L_body_size ;

    if (P_size < L_size_to_decode) {

      // segmentation case
      // *P_error = C_ProtocolBinaryFrame::E_MSG_ERROR_DECODING_SIZE_LESS ;
      *P_error = C_ProtocolFrame::E_MSG_ERROR_DECODING_SIZE_LESS ;
      
      GEN_DEBUG(1, "C_MessageBinary::decode() size < body");
      
    } else { // ok lets decode the buffer

      GEN_DEBUG(1, "C_MessageBinary::decode() message complete [buf size " 
		   << L_size_to_decode <<" | body size "<< L_body_size << "]");
      // first decode the header

      m_header_id = m_protocol -> decode_header (P_buffer, 
						 L_size_to_decode,
						 m_header_values) ;

      // now decode the body
      if ((m_header_id != -1) || (m_protocol->get_header_type_id() == -1)) {

	m_nb_body_values = MAX_BODY_VALUES ;

	L_ret_decode = m_protocol->decode_body(L_ptr,
					       L_body_size,
					       m_body_val,
					       &m_nb_body_values,
					       &m_header_id);
	if (L_ret_decode != -1) {
	  GEN_DEBUG(1, "C_MessageBinary::decode() message decoded ");
	  *P_error = C_ProtocolFrame::E_MSG_OK ;
	  L_ret = P_size - L_size_to_decode ;
	} else {
	  
	  GEN_ERROR(E_GEN_FATAL_ERROR, "message error (body content)");
          //	  *P_error = C_ProtocolBinaryFrame::E_MSG_ERROR_DECODING;
	  *P_error = C_ProtocolFrame::E_MSG_ERROR_DECODING;
	  L_ret = 0 ;

	}
	  

      } else {
        GEN_ERROR(E_GEN_FATAL_ERROR, "Unrecognized buffer header");	
	*P_error = C_ProtocolFrame::E_MSG_ERROR_DECODING;
	L_ret = 0 ;
      }

      
    }

  }

  // TO BE IMPLEMENTED
  // check the size of buffer according to the
  // protocol definition

  GEN_DEBUG(1, "C_MessageBinary::decode() end");
  
  return (L_ret) ;
}


void C_MessageBinary::encode (unsigned char* P_buffer, 
			     size_t* P_size,
			     C_ProtocolFrame::T_pMsgError P_error) {

  unsigned char *L_ptr = P_buffer ;
  size_t         L_size_header, L_size_body, L_size_all ;

  size_t         L_size_buffer ;
  
  GEN_DEBUG(1, "C_MessageBinary::encode() start ");

  L_size_all = 0 ;
  L_size_header = *P_size ;

  C_ProtocolFrame::T_MsgError L_error = C_ProtocolFrame::E_MSG_OK ;

  L_size_buffer = *P_size ;

  // start with header
  m_protocol->encode_header
    (m_header_id, m_header_values, L_ptr, &L_size_header, &L_error);

  if (L_error == C_ProtocolFrame::E_MSG_OK) {
    L_size_buffer -= L_size_header ;

    L_size_body = L_size_buffer ;
  
    L_ptr += L_size_header ;
    L_size_all += L_size_header ;

    L_error = m_protocol->encode_body(m_nb_body_values,
                                      m_body_val,
                                      L_ptr,
                                      &L_size_body);

    if (L_error == C_ProtocolFrame::E_MSG_OK) {
      L_size_all += L_size_body ;

      if (m_protocol->get_msg_length_start_detected()) {
        m_protocol->update_length(P_buffer, 
                                  (L_size_all-(m_protocol->get_msg_length_start())));
      } else {
        if(m_protocol->get_header_length_excluded () ) { 
          m_protocol->update_length(P_buffer, L_size_body);
        } else {
          m_protocol->update_length(P_buffer, L_size_all);
        }
      }

      *P_size = L_size_all ;
    }

  }

  *P_error = L_error ;
  
  GEN_DEBUG(1, "C_MessageBinary::encode() end");
}

void C_MessageBinary::set_header_id_value (int P_id) {
  
  GEN_DEBUG(1, "C_MessageBinary::set_header_id_value(" << P_id << ") start");
  m_header_id = P_id ;
  m_protocol->get_header_values (P_id, m_header_values) ;
  m_protocol->get_body_values (P_id, m_body_val, &m_nb_body_values);
  
  GEN_DEBUG(1, "C_MessageBinary::set_header_id_value() end");
}

void C_MessageBinary::set_body_value (C_ProtocolBinary::T_pBodyValue P_val) {
  
  GEN_DEBUG(1, "C_MessageBinary::set_body_value() start current nb: " << m_nb_body_values );

  if (m_nb_body_values == MAX_BODY_VALUES) {
    GEN_FATAL(E_GEN_FATAL_ERROR, 
	  "Maximum number of values ["
	  << MAX_BODY_VALUES << "] reached" << iostream_endl) ;
  } else {
    m_protocol->set_body_value(&m_body_val[m_nb_body_values], P_val);
    m_nb_body_values++ ;
  }

  GEN_DEBUG(1, "C_MessageBinary::set_body_value() end - new nb: " << m_nb_body_values);
}

C_MessageBinary& C_MessageBinary::operator= (C_MessageBinary & P_val) {

  m_protocol  = P_val.m_protocol ;
  m_call_id   = P_val.m_call_id ;
  m_header_id = P_val.m_header_id ;

  if (m_protocol->get_complex_header_presence())
  {
    m_protocol -> reset_header_values(m_protocol->get_nb_field_header(), 
                                      m_header_values);
  }
  m_protocol -> reset_body_values(m_nb_body_values, m_body_val);

  m_nb_body_values = P_val.m_nb_body_values ;
  m_protocol -> set_header_values (m_header_values, P_val.m_header_values);
  m_protocol -> set_body_values (m_nb_body_values, m_body_val, P_val.m_body_val);

  return (*this) ;
}


void C_MessageBinary::get_header_value (T_pValueData P_res, 
                                        int          P_id) {
  *P_res = m_header_values[P_id] ;
}


bool C_MessageBinary::get_body_value (T_pValueData P_res,
                                      int          P_id, int P_occurence) {

  int  L_i ;
  bool L_found = false ;
  int L_occurence = P_occurence;


  // Search the body value in the array
  for (L_i=0 ; L_i < m_nb_body_values ; L_i++) {
      L_occurence -= get_body_value(P_id, L_occurence, P_res, &m_body_val[L_i]);
      if(L_found = (L_occurence == 0))
        break;
  }

  return (L_found);
}


int C_MessageBinary::get_body_value (int P_id, int P_occurence,
                                      T_pValueData P_val,
                                      C_ProtocolBinary::T_pBodyValue P_body_val) {
    int L_occurence = P_occurence;
    if (P_body_val->m_id == P_id) 
      L_occurence--;

    if (L_occurence == 0) {
      m_protocol->get_body_value(P_val, P_body_val) ;
    } else if (P_body_val->m_sub_val != NULL) { // grouped
      for (int L_i=0 ; L_i < (int)P_body_val->m_value.m_val_number ; L_i++) {
           L_occurence -= get_body_value(P_id, L_occurence, P_val, &P_body_val->m_sub_val[L_i]);
           if (L_occurence == 0)
               break;
      }
    }
    return (P_occurence-L_occurence);

}

bool C_MessageBinary::set_body_value (int P_id, int P_occurence, T_pValueData P_val) {


  int  L_i ;
  bool L_found = false ;
  int L_occurence = P_occurence;


  for (L_i=0 ; L_i < m_nb_body_values ; L_i++) {
      L_occurence -= set_body_value(P_id, L_occurence, &m_body_val[L_i], P_val);
      if(L_found = (L_occurence == 0))
        break;
  }

  return (L_found);

}

int C_MessageBinary::set_body_value (int P_id, int P_occurence,
                                      C_ProtocolBinary::T_pBodyValue P_body_val,
                                      T_pValueData P_val) {

    int L_occurence = P_occurence;
    if (P_body_val->m_id == P_id)
      L_occurence--;


    if (L_occurence == 0) {
      m_protocol->set_body_value(P_body_val, P_val) ;
    } else if (P_body_val->m_sub_val != NULL) { // grouped
      for (int L_i=0 ; L_i < (int)P_body_val->m_value.m_val_number ; L_i++) {
           L_occurence -= set_body_value(P_id, L_occurence, &P_body_val->m_sub_val[L_i], P_val);
           if (L_occurence == 0)
               break;
      }
    }

    return (P_occurence-L_occurence);

}



iostream_output& operator<< (iostream_output& P_stream, C_MessageBinary &P_msg) {
  P_stream << "[" ;
  P_msg.m_protocol->print_header(P_stream, 
				 P_msg.m_header_id, 
				 P_msg.m_header_values) ;
  P_msg.m_protocol->print_body(P_stream,
			       P_msg.m_nb_body_values,
			       P_msg.m_body_val);
  P_stream << GEN_HEADER_LOG << GEN_HEADER_LEVEL(LOG_LEVEL_MSG) << "]" ;
  return (P_stream) ;
}

C_ProtocolBinary* C_MessageBinary::get_protocol() {
  return (m_protocol);
}

T_pValueData C_MessageBinary::get_session_id (C_ContextFrame *P_ctxt) {
  return (((this)->*(m_protocol->get_m_session_method()))(P_ctxt)) ;
}

T_pValueData C_MessageBinary::getSessionFromOpenId (C_ContextFrame *P_ctxt) {

  T_pReceiveMsgContext L_recvCtx = NULL ;
  C_CallContext*       L_callCtx = NULL ;
  GEN_DEBUG(1, "C_MessageBinary::getSessionFromOpenId() start");
  
  if (m_session_id == NULL) {
    if (P_ctxt != NULL) {
      if ((L_callCtx = dynamic_cast<C_CallContext*>(P_ctxt)) != NULL) {
        ALLOC_VAR(m_session_id, T_pValueData, sizeof(T_ValueData));
        m_session_id->m_type = E_TYPE_NUMBER ;
        m_session_id->m_value.m_val_number = 
          L_callCtx->m_channel_table[L_callCtx->m_channel_received] ;
        m_session_id->m_id = m_protocol->get_m_session_id_id() ;

      } else if ((L_recvCtx = dynamic_cast<T_pReceiveMsgContext>(P_ctxt)) != NULL) {
        ALLOC_VAR(m_session_id, T_pValueData, sizeof(T_ValueData));
        m_session_id->m_type = E_TYPE_NUMBER ;
        m_session_id->m_value.m_val_number = L_recvCtx->m_response ;
        m_session_id->m_id =  m_protocol->get_m_session_id_id() ;
      }
    }
  }

  GEN_DEBUG(1, "C_MessageBinary::getSessionFromOpenId() end");
  return (m_session_id);
}

T_pValueData C_MessageBinary::getSessionFromField (C_ContextFrame *P_ctxt) {

  T_pValueData                               L_id_value = NULL  ;
  int                                        L_i                ;
  C_ProtocolBinary::T_pManagementSessionId   L_session_elt      ;
  int                                        L_nb_manag_session ;
  T_ValueData                                L_tmp_id_value     ;


  GEN_DEBUG(1, "C_MessageBinary::get_session_id() start");

  // retrieve the number of managment session
  L_nb_manag_session = m_protocol->get_nb_management_session () ;

  for (L_i = 0 ; L_i < L_nb_manag_session ; L_i++) {
    L_session_elt = m_protocol->get_manage_session_elt(L_i);

    switch (L_session_elt->m_msg_id_type) {
    case C_ProtocolBinary::E_MSG_ID_HEADER:
      L_id_value = &m_header_values[L_session_elt->m_msg_id_id];
      // To be verify
      if (L_session_elt->m_msg_id_value_type == E_TYPE_STRING) {
        memcpy(&L_tmp_id_value,L_id_value,sizeof(L_tmp_id_value)) ;
        m_protocol->reset_value_data(&m_id);
        m_protocol->convert_to_string(&m_id, &L_tmp_id_value);
        L_id_value = &m_id ;
      }
      break ;

    case C_ProtocolBinary::E_MSG_ID_BODY:
      L_id_value = getSessionFromBody(L_session_elt->m_msg_id_id);
      break ;
    }

    if (L_id_value != NULL) {
      break;
    }
  } // for (L_i...)

  GEN_DEBUG(1, "C_MessageBinary::get_session_id() end");
  return (L_id_value) ;

}

T_pValueData C_MessageBinary::get_out_of_session_id () {
  return (NULL) ;
}

bool C_MessageBinary::check(C_MessageFrame    *P_ref, 
			    unsigned int       P_levelMask,
			    T_CheckBehaviour   P_behave) {

  bool                       L_ret   = true  ;
  int                        L_nb            ;
  int                        L_i, L_j, L_id  ;
  bool                       L_found = false ;

  C_MessageBinary  *L_ref ;

  C_ProtocolBinary::T_pHeaderBodyValue L_descr         ;
  C_ProtocolBinary::T_pHeaderValue L_descrVal      ; 

  L_ref = dynamic_cast<C_MessageBinary*>(P_ref);
  
  // check body
  L_nb = L_ref->m_nb_body_values ;
  
  if (P_levelMask & _check_level_mask[E_CHECK_LEVEL_FIELD_PRESENCE]) {
    // check that the fields of the scenario are present
    for (L_i = 0 ; L_i < L_nb; L_i++) {
      L_found = false ;
      L_id = L_ref->m_body_val[L_i].m_id ;
      for(L_j=0; L_j <m_nb_body_values; L_j++) {
	if (m_body_val[L_j].m_id == L_id) {
	  L_found = true ;
	  break ;
	}
      }
      if (L_found == false) {
	L_ret = false ;
	L_descr = m_protocol->get_header_body_value_description(L_id);
	if (L_descr != NULL) {
	  L_descrVal = m_protocol->get_header_value_description(m_header_id);

	  GEN_LOG_EVENT        (_check_behaviour_mask[P_behave], 
			    "check failed in [" 
			    << m_protocol -> get_header_name() 
			    << "] ["
			    << L_descrVal->m_name
			    << "]");
	  GEN_LOG_EVENT_NO_DATE(_check_behaviour_mask[P_behave],
			    "                [" 
			    << m_protocol->get_header_body_name()
			    << "] [" << L_descr->m_name << "] not found");
	}
      } else {
	// check of the value to be done here
      }
    }
  }



  if (P_levelMask & _check_level_mask[E_CHECK_LEVEL_FIELD_ADDED]) {
    
    // check that there is no field added from the scenario
    for (L_i = 0 ; L_i < m_nb_body_values; L_i++) {
      L_found = false ;
      L_id = m_body_val[L_i].m_id ;
      for(L_j=0; L_j <m_nb_body_values; L_j++) {
	if (L_ref->m_body_val[L_j].m_id == L_id) {
	  L_found = true ;
	  break ;
	}
      }
      if (L_found == false) {
	L_ret = false ;
	L_descr = m_protocol->get_header_body_value_description(L_id);
	if (L_descr != NULL) {
	  L_descrVal = m_protocol->get_header_value_description(m_header_id);
	  

	  GEN_LOG_EVENT        (_check_behaviour_mask[P_behave], 
			    "check failed in [" 
			    << m_protocol -> get_header_name() 
			    << "] ["
			    << L_descrVal->m_name
			    << "]");
	  GEN_LOG_EVENT_NO_DATE(_check_behaviour_mask[P_behave], 
			    "     additional ["
			    << m_protocol->get_header_body_name()
			    << "] [" 
			    << L_descr->m_name << "] found");

	}
      }
    }
  }

  return (L_ret) ;
}


int C_MessageBinary::check_field_presence (int P_id, int P_occurence,
                                      C_ProtocolBinary::T_pBodyValue P_body_val) {
    int L_occurence = P_occurence;
    if (P_body_val->m_id == P_id)
      L_occurence--;


    if (L_occurence == 0) {
      // do nothing;
    } else if (P_body_val->m_sub_val != NULL) { // grouped
      for (int L_i=0 ; L_i < (int)P_body_val->m_value.m_val_number ; L_i++) {
           L_occurence -= check_field_presence(P_id , L_occurence,  &P_body_val->m_sub_val[L_i]);
           if (L_occurence == 0)
               break;
      }
    }

    return (P_occurence-L_occurence);

}


bool C_MessageBinary::check_field_presence (int              P_id, int P_occurence,
					    T_CheckBehaviour P_behave,
					    int              P_instance,
					    int              P_sub_id) {
  bool                                 L_ret   = true  ;
  int                                  L_i , L_j ;
  bool                                 L_found = false ;
  C_ProtocolBinary::T_pHeaderBodyValue L_descr         ;
  C_ProtocolBinary::T_pHeaderValue     L_descrVal      ; 
  unsigned long                        L_max_nb_field_header ;
  int L_occurence = P_occurence;



  L_max_nb_field_header = m_protocol->get_m_max_nb_field_header () ;

  if (P_id < (int)L_max_nb_field_header) { return (true) ; }

  L_j = P_id - (int)L_max_nb_field_header;

  // check that the fields of the scenario are present
  for (L_i=0 ; L_i < m_nb_body_values ; L_i++) {
      L_occurence -= check_field_presence(L_j, L_occurence, &m_body_val[L_i]);
      if(L_found = (L_occurence == 0))
        break;
  }

  if (L_found == false) {
    L_ret = false ;
    L_descr = m_protocol->get_header_body_value_description(L_j);
    if (L_descr != NULL) {
      L_descrVal = m_protocol->get_header_value_description(m_header_id);
      
      if (L_descrVal != NULL) {
        GEN_LOG_EVENT        (_check_behaviour_mask[P_behave], 
                              "check failed [" 
                              << m_protocol -> get_header_name() 
                              << "] ["
                              << L_descrVal->m_name
                              << "]");
      } else {
	GEN_LOG_EVENT        (_check_behaviour_mask[P_behave], 
			      "check failed [" 
			      << m_protocol -> get_header_name() 
			      << "] ["
			      << m_protocol->message_name(m_header_id)
			      << "]");
      }
      
      GEN_LOG_EVENT_NO_DATE(_check_behaviour_mask[P_behave],
                            "                [" 
                            << m_protocol->get_header_body_name()
                            << "] [" << L_descr->m_name << "] not found");
    }
  }

  return (L_ret) ;
}

bool C_MessageBinary::check_field_value (C_MessageFrame   *P_ref,
		                         int               P_id, int P_occurence,
					 T_CheckBehaviour  P_behave,
					 int               P_instance,
					 int               P_sub_id) {

  GEN_DEBUG(1, "C_MessageBinary::check_field_value() start");

  C_MessageBinary                     *L_ref                 ;
  unsigned long                        L_max_nb_field_header ;
  int                                  L_id     = P_id       ;
  bool                                 L_check  = false      ;

  T_ValueData                          L_value_ref           ;
  T_ValueData                          L_value               ;
  bool                                 L_found   = true      ;

  C_ProtocolBinary::T_pHeaderBodyValue L_descr               ;
  C_ProtocolBinary::T_pHeaderField     L_headerField         ;


  L_ref                 = dynamic_cast<C_MessageBinary*>(P_ref)    ;
  L_max_nb_field_header = m_protocol->get_m_max_nb_field_header () ;

  if (L_id >= (int) L_max_nb_field_header) {
    // case body
    L_id -= L_max_nb_field_header ;

    L_found =  L_ref->get_body_value (&L_value_ref, L_id, P_occurence);
    if (L_found == false ) {
      L_descr = m_protocol->get_header_body_value_description(L_id);
      if (L_descr != NULL) {
        GEN_LOG_EVENT        (_check_behaviour_mask[P_behave], 
                              "check failed in [" 
                              <<  m_protocol->message_name(L_ref->m_header_id) 
                              << "] " << m_protocol->message_name() 
                              << ", value of " << m_protocol->message_component_name ()
                              << " ["
                              << L_descr->m_name
                              << "] is not present in this in reference message");
      }
      return (L_found);
    }
    L_found = get_body_value (&L_value, L_id, P_occurence);
    if (L_found == false ) {
      L_descr = m_protocol->get_header_body_value_description(L_id);
      if (L_descr != NULL) {
        GEN_LOG_EVENT        (_check_behaviour_mask[P_behave], 
                              "check failed in [" 
                              <<  m_protocol->message_name(L_ref->m_header_id) 
                              << "] " << m_protocol->message_name() 
                              << ", value of " << m_protocol->message_component_name ()
                              << " ["
                              << L_descr->m_name
                              << "] is not present in message received");
      }
      return (L_found);
    }
    
    L_check = (L_value_ref == L_value) ;
  } else {
    // case header
    L_check = (L_ref->m_header_values[L_id] == m_header_values[L_id]) ;
  }

  if (L_check == false) {
    if (P_id <  (int) L_max_nb_field_header) {
      L_headerField = m_protocol->get_header_field_description(L_id);
      if (L_headerField != NULL) {
        GEN_LOG_EVENT        (_check_behaviour_mask[P_behave], 
                              "check failed in [" 
                              <<  m_protocol->message_name(L_ref->m_header_id) 
                              << "] " << m_protocol->message_name() 
                              << ", value of field" 
                              << " ["
                              << L_headerField->m_name
                              << "] is incorrect. Expected ["
                              << L_value_ref 
                              << "] but got ["
                              << "L_value" << "]");
      }
    } else {
      L_descr = m_protocol->get_header_body_value_description(L_id);
      if (L_descr != NULL) {
        GEN_LOG_EVENT        (_check_behaviour_mask[P_behave], 
                              "check failed in [" 
                              <<  m_protocol->message_name(L_ref->m_header_id) 
                              << "] " << m_protocol->message_name() 
                              << ", value of " << m_protocol->message_component_name ()
                              << " ["
                              << L_descr->m_name
                              << "]. Expected ["
                              << L_value_ref
                              << "] but got ["
                              << L_value << "]");
      } else {
        GEN_LOG_EVENT        (_check_behaviour_mask[P_behave], 
                              "check failed in [" 
                              <<  m_protocol->message_name(L_ref->m_header_id) 
                              << "] " << m_protocol->message_name() 
                              << ". Expected ["
                              << L_value_ref
                              << "], but got ["
                              << L_value << "]");
      }
    }
  } // if (L_check == false)

  GEN_DEBUG(1, "C_MessageBinary::check_field_value() end ret: " << L_check);

  return (L_check) ;
}

bool C_MessageBinary::check_field_order (int              P_id,
					 T_CheckBehaviour P_behave,
					 int              P_position) {
  bool                                 L_ret   = true  ;

  GEN_DEBUG(1, "C_MessageBinary::check_field_order() start");
  GEN_DEBUG(1, "C_MessageBinary::check_field_order() P_id:       " << P_id);
  GEN_DEBUG(1, "C_MessageBinary::check_field_order() P_behave:   " << P_behave);
  GEN_DEBUG(1, "C_MessageBinary::check_field_order() P_position: " 
		  << P_position);
  GEN_DEBUG(1, "C_MessageBinary::check_field_order() end ret: " << L_ret);

  return (L_ret) ;
}
// field management
bool C_MessageBinary::set_field_value(T_pValueData P_value, 
				      int P_id, int P_occurence,
				      int P_instance,
				      int P_sub_id) {

  bool                          L_found = true ;

  C_ProtocolBinary::T_MsgIdType L_id_type ;
  int                           L_id = m_protocol->retrieve_field_id(P_id, &L_id_type);

  switch (L_id_type) {
  case C_ProtocolBinary::E_MSG_ID_HEADER:
    set_header_value(L_id, P_value);
    break ;
  case C_ProtocolBinary::E_MSG_ID_BODY:
    set_body_value(L_id, P_occurence, P_value);
    break ;
  }

  return (L_found) ;
}

T_pValueData C_MessageBinary::get_field_value (int P_id, int P_occurence,
                                               C_ContextFrame *P_ctxt,
                                               int P_instance,
                                               int P_sub_id) {

  if (m_session_id == NULL) {
    if (P_id == -1) {
      return(getSessionFromOpenId (P_ctxt));
    } else {
      ALLOC_VAR(m_session_id,
                T_pValueData,
                sizeof(T_ValueData));
      
      if (get_field_value(P_id,  P_occurence,
                          P_instance,
                          P_sub_id,
                          m_session_id) == false ) {
        FREE_VAR(m_session_id);
      }
    }
  }
  return (m_session_id);
}

bool C_MessageBinary::get_field_value(int P_id,  int P_occurence,
				      int P_instance,
				      int P_sub_id,
				      T_pValueData P_value) {
  bool                          L_found = true ;
  C_ProtocolBinary::T_MsgIdType L_id_type ;
  int                           L_id = m_protocol->retrieve_field_id(P_id, &L_id_type);

  GEN_DEBUG(1 , "C_MessageBinary::get_field_value() start ");
  GEN_DEBUG(1 , "C_MessageBinary::get_field_value() L_id_type = " 
		  << L_id_type << " (0 = hd, 1 = bd)");

  switch (L_id_type) {
  case C_ProtocolBinary::E_MSG_ID_HEADER:
    get_header_value (P_value, L_id);
    break ;
  case C_ProtocolBinary::E_MSG_ID_BODY:
    L_found = get_body_value (P_value, L_id, P_occurence);
    break ;
  }

  GEN_DEBUG(1 , "C_MessageBinary::get_field_value() end ret = " << L_found);
  return (L_found);
}

T_TypeType C_MessageBinary::get_field_type (int P_id,
					    int P_sub_id) {
  return (m_protocol->get_field_type(P_id,P_sub_id));
}

void C_MessageBinary::dump(iostream_output& P_stream) {
  GEN_DEBUG(1, "C_MessageBinary::dump() start");

  P_stream << *this << iostream_endl ;

  GEN_DEBUG(1, "C_MessageBinary::dump() end");
}

char* C_MessageBinary::name() {
  return( m_protocol -> message_name(m_header_id));
}

int C_MessageBinary::get_id_message(){
  return (m_header_id) ;
}



bool C_MessageBinary::get_field_value(int P_id,  int P_occurence,
                                    C_RegExp *P_reg,
                                    T_pValueData P_value) {
  return (true) ;
}


void   C_MessageBinary::update_message_stats  () {
}

int    C_MessageBinary::get_buffer (T_pValueData      P_dest,
                                    T_MessagePartType P_header_body_type) {

  int                              L_ret          =  0                        ;
  size_t                           L_size         =  4096                     ;
  size_t                           L_size_header  =  0                        ;
  unsigned char                   *L_data         = NULL                      ;
  C_ProtocolFrame::T_MsgError      L_error        = C_ProtocolFrame::E_MSG_OK ;

  T_ValueData                      L_value                                    ;

  if (P_header_body_type == E_NOTHING_TYPE ) {
    return (L_ret);
  } else {
    ALLOC_TABLE(L_data,
                unsigned char *,
                sizeof(unsigned char),
                L_size);

    encode_without_stat(L_data, &L_size, &L_size_header, &L_error);
    
    if (L_error != C_ProtocolFrame::E_MSG_OK) {
      L_ret = -1;
    } else {
      L_value.m_type = E_TYPE_STRING ;
      L_value.m_value.m_val_binary.m_size = L_size;
      ALLOC_TABLE(L_value.m_value.m_val_binary.m_value,
                  unsigned char*,
                  sizeof(unsigned char),
                  L_value.m_value.m_val_binary.m_size);
      
      memcpy(L_value.m_value.m_val_binary.m_value,
             L_data,
             L_value.m_value.m_val_binary.m_size);


      P_dest->m_type = E_TYPE_STRING ;
      switch (P_header_body_type) {
      case E_HEADER_TYPE :

      ALLOC_TABLE(P_dest->m_value.m_val_binary.m_value,
                  unsigned char*,
                  sizeof(unsigned char),
                  L_size_header);
      P_dest->m_value.m_val_binary.m_size = L_size ;
      extractBinaryVal(*P_dest, 0, L_size_header,
                       L_value);

        break;
      case E_BODY_TYPE   :
      ALLOC_TABLE(P_dest->m_value.m_val_binary.m_value,
                  unsigned char*,
                  sizeof(unsigned char),
                  (L_size - L_size_header));
      P_dest->m_value.m_val_binary.m_size = L_size ;
      extractBinaryVal(*P_dest, L_size_header, (L_size - L_size_header),
                       L_value);

        break;
      case E_ALL_TYPE    :
      ALLOC_TABLE(P_dest->m_value.m_val_binary.m_value,
                  unsigned char*,
                  sizeof(unsigned char),
                  L_size);
      P_dest->m_value.m_val_binary.m_size = L_size ;
      extractBinaryVal(*P_dest, 0, L_size,
                       L_value);

        break;
      default:
        L_ret = -1 ;
        break ;
      }
      resetMemory(L_value);
        
    }
  }
  FREE_TABLE(L_data);

  return (L_ret) ;
}


void C_MessageBinary::encode_without_stat (unsigned char*               P_buffer, 
                                           size_t*                      P_size,
                                           size_t*                      P_size_header,
                                           C_ProtocolFrame::T_pMsgError P_error) {
  
  unsigned char              *L_ptr          = P_buffer                  ;
  size_t                      L_size_header  = *P_size                   ;
  size_t                      L_size_body    = 0                         ;
  size_t                      L_size_all     = 0                         ;
  size_t                      L_size_buffer  = *P_size                   ;
  C_ProtocolFrame::T_MsgError L_error        = C_ProtocolFrame::E_MSG_OK ;

  m_protocol->encode_header_without_stat
    (m_header_id, m_header_values, L_ptr, &L_size_header, &L_error);
  if (L_error == C_ProtocolFrame::E_MSG_OK) {
    *P_size_header = L_size_header ;
    L_size_buffer -= L_size_header ;
    L_size_body    = L_size_buffer ;
    L_ptr         += L_size_header ;
    L_size_all    += L_size_header ;
    
    L_error  = m_protocol->encode_body_without_stat(m_nb_body_values,
                                                    m_body_val,
                                                    L_ptr,
                                                    &L_size_body);
    if (L_error == C_ProtocolFrame::E_MSG_OK) {
      
      L_size_all += L_size_body ;
      if (m_protocol->get_msg_length_start_detected()) {
        m_protocol->update_length(P_buffer, 
                                  (L_size_all-(m_protocol->get_msg_length_start())));
      } else {
        if(m_protocol->get_header_length_excluded () ) { 
          m_protocol->update_length(P_buffer, L_size_body);
        } else {
          m_protocol->update_length(P_buffer, L_size_all);
        }
      }
      *P_size = L_size_all ;
    }
  }
  *P_error = L_error ;
}


T_pValueData C_MessageBinary::getSessionFromBody(int P_id) {
  int                                 L_i             ;
  T_pValueData                        L_ret   = NULL  ;
  bool                                L_found = false ;
 

  for (L_i=0 ; L_i < m_nb_body_values ; L_i++) {
    if (m_body_val[L_i].m_id == P_id) { 
      L_found = true; 
      break; 
    }
  }
  if (L_found == true) {
    m_protocol->reset_value_data(&m_id);
    m_protocol->get_body_value(&m_id, &m_body_val[L_i]) ;
    L_ret = &m_id ;
  }

  return (L_ret) ;
}

int C_MessageBinary:: logMsgHeaderInfo(iostream_output *P_OutStream)
{ 

    C_ProtocolBinary::T_pHeaderField L_hdrField;
    unsigned long i;
    static bool hdrNamesTraced = false;
    
    if (!hdrNamesTraced)
    {
      for (i = 0; i < m_protocol->get_nb_field_header(); i++)
      {
          L_hdrField = m_protocol->get_header_field_description(i);
         (*P_OutStream)<<L_hdrField->m_name <<",";
      }    
      (*P_OutStream)<<"Session-Id, Request Timestamp, Response Timestamp, Response Time(ms)"<<iostream_endl;  
      hdrNamesTraced = true;
    }
    
    
    for (i = 0; i < m_protocol->get_nb_field_header(); i++)
    {
      (*P_OutStream)<<m_header_values[i].m_value.m_val_number<<",";
    }

    return 0;
}
