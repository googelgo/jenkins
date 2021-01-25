/*
 * Copyright (C) 2006 Aricent Inc . All Rights Reserved
 *
 * $Id: osxsys.h,v 1.2 2007/11/23 14:14:44 allprojects Exp $
 *
 * Description:
 * Contains standard constants and error codes used by fsap applns.
 * Exported file.
 *
 */

#ifndef _OSIX_SYS_H
#define _OSIX_SYS_H

/************************************************************************
*                                                                       *
*                           OSIX Constants                              *
*                                                                       *
*************************************************************************/
#define  OSIX_GLOBAL        0x0
#define  OSIX_LOCAL         0x1
#define  OSIX_WAIT          (UINT4)(~0UL)
#define  OSIX_NO_WAIT         0

/* Use this if you want/must. */
#define SELF_TASKNAME      ((UINT1 *)OsixExGetTaskName(OsixGetCurTaskId()))

/************************************************************************
*                                                                       *
*                           Task Modes                                  *
*                                                                       *
*************************************************************************/
#define OSIX_TASK_SUPERVISORY_MODE      0x0
#define OSIX_TASK_USER_MODE             0x8
#define OSIX_TASK_TIME_SLICING_ENABLED  0x0
#define OSIX_TASK_TIME_SLICING_DISABLED 0x10
#define OSIX_TASK_PREEMPTIVE            0x0
#define OSIX_TASK_NON_PREEMPTIVE        0x20
#define OSIX_TASK_ISR_ENABLED           0x0
#define OSIX_TASK_ISR_DISABLED          0x40

#define OSIX_DEFAULT_STACK_SIZE         10000
/* CAMEOTAG: Add by fenglin.2012-8-15
 * In mainos, Redefine stack size of http task for different platform 
 * In pthreads, do not more large size , so we define it the same as OSIX_DEFAULT_STACK_SIZE
 */
#define CM_OSIX_DEFAULT_LARGE_STACK_SIZE OSIX_DEFAULT_STACK_SIZE

#define OSIX_DEFAULT_TASK_MODE \
                                (OSIX_GLOBAL|\
                                OSIX_TASK_SUPERVISORY_MODE|\
                                OSIX_TASK_TIME_SLICING_ENABLED)
/************************************************************************
*                                                                       *
*                      Message types                                    *
*                                                                       *
*************************************************************************/
#define OSIX_MSG_NORMAL 0x0
#define OSIX_MSG_URGENT 0x80 

/************************************************************************
*                                                                       *
*                     Semaphore types                                   *
*                                                                       *
*************************************************************************/
#define OSIX_SEM_FIFO  0x0
#define OSIX_SEM_PRIOR 0x100 

#define OSIX_DEFAULT_SEM_MODE    (OSIX_SEM_FIFO|OSIX_GLOBAL)
/************************************************************************
*                                                                       *
*                       Event types                                     *
*                                                                       *
*************************************************************************/
#define OSIX_EV_ANY  0x0
#define OSIX_EV_ALL  0x200

/************************************************************************
*                                                                       *
*                       Error Codes                                     *
*                                                                       *
*************************************************************************/
#define OSIX_SUCCESS               0
#define OSIX_FAILURE               1

#define OSIX_ERR_NOTSUSP           2 /* Thetaskisnotsuspended */
#define OSIX_ERR_NO_EVENTS         3 /* Noeventsposted(NO_WAITcase) */
#define OSIX_ERR_NO_MSG            4 /* NomessagesintheQ(NO_WAITcase) */
#define OSIX_ERR_NO_RSRC           5 /* OSresourcesalreadyusedupcompletely. */
#define OSIX_ERR_NO_SEM            6 /* Nosemaphore(NO_WAITcase) */
#define OSIX_ERR_NO_SUCH_Q         7 /* SuchaQdoesnotexist */
#define OSIX_ERR_NO_SUCH_SEM       8 /* Nosuchsemaphoreexists */
#define OSIX_ERR_NO_SUCH_TASK      9 /* Suchataskdoesnotexist */
#define OSIX_ERR_PRIORITY         10 /* Invalidpriorityvalue */
#define OSIX_ERR_QNF              11 /* NosuchnamedQexists. */
#define OSIX_ERR_Q_DELETED        12 /* Qdeletedwhilewaitingformessages. */
#define OSIX_ERR_Q_FULL           13 /* Qalreadycontainsmaxnumberofmessages */
#define OSIX_ERR_SEM_DELETED      14 /* Semaphorehasbeendeletedwhilewaiting. */
#define OSIX_ERR_STK_SIZE         15 /* Stacksizeerror(toosmall/toolarge) */
#define OSIX_ERR_SUSP             16 /* Taskalreadysuspended */
#define OSIX_ERR_TASKNF           17 /* Nosuchnamedtaskexists */
#define OSIX_ERR_TASK_ACTIVATION  18 /* TaskActivationerror */
#define OSIX_ERR_TASK_MODE        19 /* Unsupported/impossibleTaskMode */
#define OSIX_ERR_TIMEOUT          20 /* Timeoutoccurredduringeventwaiting. */
#define OSIX_ERR_UNKNOWN          21 /* Unspecifiederrorcondition */
#define OSIX_ERR_OS_DOES_NOT_SUPP 22
#define OSIX_ERR_NO_SUCH_OBJ      23

/* Error Levels to be used in OsixSetDbg. */
#define OSIX_DBG_MINOR    0x1
#define OSIX_DBG_MAJOR    0x2
#define OSIX_DBG_CRITICAL 0x4
#define OSIX_DBG_FATAL    0x8

/************************************************************************
*                                                                       *
*                            Typedefs                                   *
*                                                                       *
*************************************************************************/

/*
 * The Osix configuration structure passed to OsixInit.
 * u4TicksPerSecond will signify the smallest timer you can start. 
 * Setting it to 1 => you can only have a granularity of 1 sec.
 * Setting it to 100 => a granularity of 10 ms.
 * 
 * If you have set u4TicksPerSecond to 100 but all your timers are
 * in seconds, you can say u4SystemTimingUnitsPerSecond =1
 * If u4SystemTimingUnitsPerSecond = 100 and you start a timer for
 * 3 units, you will get a 30 ms timer !
 */
typedef struct OsixCfg
{
    UINT4 u4MaxTasks;
    UINT4 u4MaxQs;
    UINT4 u4MaxSems;
    UINT4 u4MyNodeId;
    UINT4 u4TicksPerSecond;
    UINT4 u4SystemTimingUnitsPerSecond;
} tOsixCfg;

typedef VOID *              tOsixQId;
typedef sem_t *             tOsixSemId;
typedef UINT4               tOsixTaskId;
typedef UINT4               tOsixSysTime;


#endif /* !_OSIX_SYS_H */
