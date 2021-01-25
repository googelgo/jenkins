/*
 * Copyright (C) 2006 Aricent Inc . All Rights Reserved
 *
 * $Id: osxprot.h,v 1.6 2012/07/31 09:31:19 siva Exp $
 *
 * Description:
 * OSIX's exported functions.
 *
 */

#ifndef OSIX_OSXPROT_H
#define OSIX_OSXPROT_H
#include "utlmacro.h"
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

UINT4
OsixInit (tOsixCfg *pOsixCfg);


UINT4
OsixCreateTask (const UINT1 au1TaskName[4], UINT4 u4TaskPriority,
                UINT4 u4StackSize,
                void (*TaskStartAddr)(INT1 *),
                INT1 ai1TaskStartArgs [1],
                UINT4 u4TaskMode,
                tOsixTaskId *pTaskId);

UINT4
OsixDeleteTask (UINT4 u4NodeId,
                const UINT1 au1TaskName[4]);

#define OsixGetTaskMode(a,b,c)     OSIX_FAILURE;
#define OsixGetTaskPriority(a,b,c) OSIX_FAILURE;
#define OsixSetTaskMode(a)         OSIX_FAILURE;
#define OsixSetTaskPriority(a,b,c) OSIX_FAILURE;
#define OsixSuspendTask(a,b)       OSIX_FAILURE;
#define OsixResumeTask(a,b)        OSIX_FAILURE;

UINT4
OsixDelayTask (UINT4 u4Duration);

UINT4
OsixGetTaskId (UINT4 u4NodeId,
               const UINT1 au1TaskName [4],
               tOsixTaskId *pTaskId);


UINT4
OsixCreateQ (const UINT1 au1Qname[4],
             UINT4 u4QDepth,
             UINT4 u4Qmode,
             tOsixQId *pQId);

UINT4
OsixDeleteQ (UINT4 u4NodeId,
             const UINT1 au1Qname[4]);
/*
UINT4
OsixSendToQ (UINT4 u4NodeId,
             const UINT1 au1Qname [4],
             tOsixMsg *pMessage,
             UINT4 u4MessagePriority);

UINT4
OsixReceiveFromQ (UINT4 u4NodeId,
                  const UINT1 au1Qname[4],
                  UINT4 u4Flags,
                  UINT4 u4Timeout,
                  tOsixMsg **ppu1Message);
*/

UINT4
OsixGetQId (UINT4 u4NodeId,
            const UINT1 au1Qname [4],
            tOsixQId *pQid);

UINT4
OsixGetNumMsgsInQ (UINT4 u4NodeId,
                   const UINT1 au1Qname[4],
                   UINT4* pu4NumberOfMsgs);

UINT4
OsixCreateSem (const UINT1 au1SemName [4],
               UINT4 u4InitialCount,
               UINT4 u4Flags,
               tOsixSemId *pSemId);

UINT4
OsixDeleteSem (UINT4 u4NodeId,
               const UINT1 au1SemName[4]);

UINT4
OsixTakeSem (UINT4 u4NodeId,
             const UINT1 au1SemName[4],
             UINT4 u4Flags,
             UINT4 u4Timeout);

UINT4
OsixGiveSem (UINT4 u4NodeId,
             const UINT1 au1SemName[4]);

UINT4
OsixGetSemId (UINT4 u4NodeId,
              const UINT1 au1SemName[4],
              tOsixSemId *pSemId);

UINT4
OsixSendEvent (UINT4 u4NodeId,
               const UINT1 au1TaskName[4],
               UINT4 u4Events);

UINT4
OsixReceiveEvent (UINT4 u4Events,
                  UINT4 u4Flags,
                  UINT4 u4Timeout,
                  UINT4 *pu4EventsReceived);

UINT4
OsixGetSysTime(tOsixSysTime *pSysTime);

UINT4
OsixGetSysTimeIn64Bit (FS_UINT8 * pSysTime);

UINT4
OsixIntLock(void);

UINT4
OsixIntUnlock(UINT4 u4s);


VOID
OsixSetDbg(UINT4 u4Value);

VOID
Fsap2Start(void);

UINT4
OsixGetCurTaskId (void);

const UINT1 *
OsixExGetTaskName(tOsixTaskId TaskId);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
