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

#ifndef _C_CALL_CONTROL_H
#define _C_CALL_CONTROL_H

#include "C_TaskControl.hpp"
#include "C_MessageFrame.hpp"
#include "C_CallContext.hpp"
#include "C_ScenarioControl.hpp"
#include "C_GeneratorConfig.hpp"

#include "TrafficType.h"
#include "C_TrafficModel.hpp"

#include "C_MultiList.hpp"

#include "C_GeneratorStats.hpp"

#include "C_ChannelControl.hpp"

#include "list_t.hpp"
#include "map_t.hpp"


#include "gen_operation_t.hpp"

#include "ProtocolData.hpp"
#include "ReceiveMsgContext.h"


#include "C_TrafficDistribUniform.hpp"
#include "C_TrafficDistribBestEffort.hpp"
#include "C_TrafficDistribPoisson.hpp"


typedef struct _event_recv {
  C_TransportEvent::T_TransportEvent_type m_type ;
  int                                     m_id   ;
} T_EventRecv, *T_pEventRecv ;

class C_ScenarioControl ;

class C_CallControl : public C_TaskControl {

public:


  typedef C_MultiList<C_CallContext> T_CallContextList, 
    *T_pCallContextList ;
  
  typedef map_t<int, T_pCallContext> T_SuspendMap, *T_pSuspendMap ;

   C_CallControl(C_GeneratorConfig    *P_config, 
		 C_ScenarioControl    *P_scenControl,
		 C_ChannelControl     *P_channel_ctrl);
  ~C_CallControl();
  
  //  void set_call_rate (unsigned long P_rate) ;
  void messageReceived      (T_pReceiveMsgContext P_ctxt) ;
  void eventReceived        (T_pEventRecv P_event) ;

  virtual void pause_traffic () ;
  virtual void restart_traffic () ;
  virtual void burst_traffic () ;

          void force_init() ;
  virtual unsigned long get_call_rate();
  virtual void change_call_rate(T_GenChangeOperation P_op,
				unsigned long        P_rate);
  virtual void change_rate_scale(unsigned long P_scale);
  virtual void change_burst (unsigned long P_burst);

  virtual void clean_traffic () ;


  T_GeneratorError close() ;

  C_CallContext::T_pCallMap* get_call_map () ;

  virtual void start_traffic () ;

  
protected:

  typedef list_t<T_EventRecv> T_EventRecvList, *T_pEventRecvList ;


  // message management
  T_pRcvMsgCtxtList    m_msg_rcv_ctxt_list ;

  // statistics
  C_GeneratorStats    *m_stat ;

  unsigned long        m_call_created ;
  unsigned long        m_call_to_simulate ;
  bool                 m_pause ;
  T_TrafficType        m_type ;


  // call context management
  T_pCallContext      *m_call_ctxt_table ;
  size_t               m_call_ctxt_table_size ;
  T_pCallContextList   m_call_ctxt_mlist ;

  T_pSuspendMap        m_call_suspended ;

  C_CallContext::T_pCallMap          *m_call_map_table ;
  int                  m_nb_channel ;
  
  // traffic parameters
  bool                 m_accept_new_call ;

  // open event management
  T_EventRecvList     *m_event_list ;

  // scenario informations
  C_ScenarioControl   *m_scenario_control ;

  // channel management
  C_ChannelControl    *m_channel_control ;

  // configuration
  C_GeneratorConfig   *m_config ;
  int                  m_max_send_loop ;
  int                  m_max_receive_loop ;
  unsigned long        m_call_timeout_ms ;

  unsigned long       *m_wait_values ;
  size_t               m_nb_wait_values ;

  bool                 m_call_timeout_abort ;

  unsigned long        m_open_timeout_ms ;

  unsigned long          m_max_retrans                    ;
  unsigned long          m_retrans_enabled                ;
  C_CallContext::T_pRetransContextList  *m_retrans_context_list           ;

  unsigned long         *m_retrans_delay_values           ;
  size_t                 m_nb_retrans_delay_values        ;

  int                    m_nb_send_per_scene              ;
  int                    m_nb_recv_per_scene              ;

  bool                   m_correlation_section            ;
  

  typedef T_pCallContext (C_CallControl::* T_CorrelationMethod)(T_ReceiveMsgContext P_rcvCtxt,
                                                                T_pValueData        *P_value_id);

  char*  verdict_string(T_exeCode P_code);

  T_CorrelationMethod    m_correlation_method                                 ;
  T_pCallContext         getSessionFromDico(T_ReceiveMsgContext P_rcvCtxt,
                                            T_pValueData        *P_value_id)    ;  
  T_pCallContext         getSessionFromScen(T_ReceiveMsgContext P_rcvCtxt,
                                            T_pValueData        *P_value_id)    ;  

  
  // TaskController related methods
  T_GeneratorError TaskProcedure();
  virtual T_GeneratorError InitProcedure();
  T_GeneratorError EndProcedure();
  T_GeneratorError StoppingProcedure();
  T_GeneratorError ForcedStoppingProcedure();

  // internal methods
  void messageReceivedControl () ;
  void messageTimeoutControl () ;
  void waitControl () ;
  void messageSendControl () ;
  void endTrafficControl () ;
  void eventControl () ;

  void messageOpenTimeoutControl () ;
  
  void stopServer () ;

  T_exeCode execute_scenario_cmd (T_pCallContext P_callContext, bool P_resume=false) ;

  void messageRetransControl () ;
  T_exeCode execute_scenario_cmd_retrans (int P_index, T_pCallContext  P_callContext);
  void insert_retrans_list(T_pCallContext  P_callContext) ;

  void stopRetrans (T_pCallContext P_callContext) ;

  void makeCallContextAvailable (T_pCallContext *P_pCallCxt) ;
  T_pCallContext  makeCallContextUnavailable (C_Scenario *P_scen);
  T_pCallContext   retrieve_call_context (int P_channel_id, T_pValueData P_id);

  void makeCallContextSuspended (T_pCallContext P_callContext);

  virtual void init_done() ;  // to be called when init scenario is executed
                              // => function also defined for the client

  void clean_mlist (long P_id) ;

  virtual void increase_incoming_call() ;

  virtual void log_search_session_from_scen(int P_channel_id) ;

} ;

typedef C_CallControl * T_pC_CallControl ;

class C_CallControlClient : public C_CallControl {
public:
  C_CallControlClient(C_GeneratorConfig *P_config, 
		      C_ScenarioControl *P_scenControl,
		      C_ChannelControl     *P_channel_ctrl) ;
  ~C_CallControlClient() ;
  virtual void start_traffic () ;

protected:
  virtual T_GeneratorError InitProcedure();
  T_GeneratorError TaskProcedure();
  T_GeneratorError StoppingProcedure();

  void             newCallControl() ;
  void             init_done () ;
  void             pause_traffic() ;
  void             restart_traffic() ;
  void             burst_traffic () ;


  virtual void clean_traffic () ;

  virtual void increase_incoming_call() ;
  virtual void log_search_session_from_scen(int P_channel_id) ;

  unsigned long    get_call_rate();
  void             change_call_rate(T_GenChangeOperation P_op,
				    unsigned long        P_rate);
  void             change_rate_scale(unsigned long P_scale);
  void             change_burst (unsigned long P_burst);

  C_TrafficModel*  m_traffic_model ;


  unsigned int            m_model_traffic_select        ;
  typedef void           (C_CallControlClient::*T_Update_Param_Traffic)()    ;
  T_Update_Param_Traffic  m_update_param_traffic        ;
  void                    calculNilParamTraffic()       ;  
  void                    calculUpdateParamTraffic()    ;  


  bool             m_outgoing_traffic ;

  long             m_call_rate, m_burst_limit ;
  long             m_call_rate_scale ;
  C_Scenario      *m_traffic_scen ;

  T_pCallContext  makeCallContextUnavailable ();

} ;

class C_CallControlServer : public C_CallControl {
public:
  C_CallControlServer(C_GeneratorConfig *P_config, 
		      C_ScenarioControl *P_scenControl,
		      C_ChannelControl     *P_channel_ctrl) ;
  ~C_CallControlServer() ;
protected:
  virtual T_GeneratorError InitProcedure();

  virtual void increase_incoming_call() ;
  virtual void log_search_session_from_scen(int P_channel_id) ;

  virtual void clean_traffic () ;
} ;


#endif  // _C_CALL_CONTROL_H
