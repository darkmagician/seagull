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
#ifndef _C_RESPONSE_TIME_LOG_H
#define _C_RESPONSE_TIME_LOG_H

#include "iostream_t.hpp"
#include "C_CallContext.hpp"
#include "C_MessageFrame.hpp"
#include "string_t.hpp"

class C_ResponseTimeLog {
public:
    C_ResponseTimeLog
    (
        string_t * P_LogFileName, 
        unsigned long L_rsptime_threshold
    );
   
    ~C_ResponseTimeLog
    (
    );

    bool Init
    (
    );

    void LogRspTimeInfo
    (
        T_pCallContext  P_callCtxt,
        C_MessageFrame *P_msg,
        C_MessageFrame *P_ref,
        unsigned long   L_RspTime        
    );

private:
    unsigned long    m_rsptime_threshold;
    string_t         m_rsptime_log_filename;
    iostream_output *m_RspTimeTraceStream; 
} ;

#endif
