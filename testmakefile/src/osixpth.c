/*
 * Copyright (C) 2006 Aricent Inc . All Rights Reserved  
 *
 * $Id: osixpth.c,v 1.27 2012/07/31 09:31:19 siva Exp $
 *
 * Description: Contains OSIX reference code for PThreads.
 *              All basic OS facilities used by protocol software
 *              from FS, use only these APIs.
 */

#include "osxinc.h"
#include "osix.h"

#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/times.h>





static clock_t      gStartTicks;

/* CMAEOTAG: add by guofeng 2014/2/18, schedule priority range */
static INT4			gi4MinPrio;
static INT4			gi4MaxPrio;

/*CAMEOTAG: Bob ++ at 2012/8/17 */
/*TODO: debug messsage for "Accepted Failed", rmoving it later */
UINT4        gu4MaxSem = 0;


/* The basic structure maintaining the name-to-id mapping */
/* of OSIX resources. 3 arrays - one for tasks, one for   */
/* semaphores and one for queues are maintained by OSIX.  */
/* Each array has this structure as the basic element. We */
/* use this array to store events for tasks also.         */

/* The description of fields of the structures below is as follows */
/*   ThrId    - the thread id returned by the OS                   */
/*   SemId    - SemId used in mutex Creation.                      */
/*   u4RscId  - the id returned by the OS                          */
/*   u2Free   - whether this structure is free or used             */
/*   u4Events - for event simulation; used only for tasks          */
/*   u4Arg    - argument for Entry Point Function                  */
/*   u4Prio   - Task priority used by the CLI display function     */
/*   EvtCond  - Conditional variable to synch. Send/Receive Event  */
/*   EvtMutex - Mutex used to synch. task creatione.               */
/*   pTskStrtAddr - Entry Point function.                          */
/*   TskMutex - Mutex used to ensure that entry point is hit only  */
/*              after task OsixTskCrt has completed.               */
/*   au1Name  - name is always multiple of 4 characters in length  */
typedef struct OsixRscTskStruct
{
    pthread_t           ThrId;
    UINT4               u4Pid;
    UINT4               u4Events;
    INT1               *pArg;
    UINT4               u4Prio;
    UINT4               u4StackSize;
    pthread_mutex_t     TskMutex;
    pthread_cond_t      EvtCond;
    pthread_mutex_t     EvtMutex;
    void                (*pTskStrtAddr) (INT1 *);
    UINT2               u2Free;
    UINT2               u2Pad;
    UINT1               au1Name[OSIX_NAME_LEN + 4];
}
tOsixTsk;
typedef struct OsixRscQueStruct
{
    VOID               *pRscId;
    UINT2               u2Free;
    UINT2               u2Filler;
    UINT1               au1Name[OSIX_NAME_LEN + 4];
}
tOsixQue;

/* A circular queue is simulated using an allocated linear memory */
/* region. Read and write pointers are used to take out and put   */
/* messages in the queue. All messages are the same size only.    */
/* So, a task or thread reads messages from this queue to service */
/* the requests one by ine i.e. one command or activity at a time */

/* The description of fields used in struct below are as follows: */
/*   pQBase - linear memory location holding messages             */
/*   pQEnd - pointer after last byte of the queue                 */
/*   pQRead - pointer where next message can be read from         */
/*   pQWrite - pointer where next message can be written to       */
/*   u4MsgLen - the length of messages on this queue              */
/*   QueCond  - Conditional variable to synch. Send/Receive       */
/*   QueMutex - semaphore for mutual exclusion during writes      */
typedef struct
{
    UINT1              *pQBase;
    UINT1              *pQEnd;
    UINT1              *pQRead;
    UINT1              *pQWrite;
    UINT4               u4MsgLen;
    UINT4               u4OverFlows;
    pthread_cond_t      QueCond;
    pthread_mutex_t    *QueMutex;
}
tPthreadQ;
typedef tPthreadQ  *tPthreadQId;

tOsixTsk            gaOsixTsk[OSIX_MAX_TSKS + 1];
/* 0th element is invalid and not used */

tOsixSem            gaOsixSem[OSIX_MAX_SEMS + 1];
/* 0th element is invalid and not used */

tOsixQue            gaOsixQue[OSIX_MAX_QUES + 1];
/* 0th element is invalid and not used */

pthread_mutex_t     gOsixMutex;
UINT4               gu4OsixTrc;
#define OSIX_TRC_FLAG gu4OsixTrc

extern UINT4 OsixSTUPS2Ticks ARG_LIST ((UINT4));
extern UINT4 OsixTicks2STUPS ARG_LIST ((UINT4));
UINT4        gu4Seconds;
UINT4        gCmu4MicroSeconds;


static UINT4 OsixRscAdd ARG_LIST ((UINT1[], UINT4, UINT4 *));
static VOID OsixRscDel ARG_LIST ((UINT4, VOID *));
static VOID        *OsixTskWrapper ARG_LIST ((VOID *));
static UINT4        FsapShowQueData
ARG_LIST ((tPthreadQ * pPthreadQ, UINT1 *pu1Result));

/* Functions which implement Message Queues. */
static tPthreadQId PTHREAD_Create_MsgQ ARG_LIST ((UINT4, UINT4));
static VOID PTHREAD_Delete_MsgQ ARG_LIST ((tPthreadQId));
static INT4 PTHREAD_Send_MsgQ ARG_LIST ((tPthreadQId, UINT1 *));
static INT4 PTHREAD_Receive_MsgQ ARG_LIST ((tPthreadQId, UINT1 *, INT4));
static UINT4 PTHREAD_MsgQ_NumMsgs ARG_LIST ((tPthreadQId));

UINT4               gu4Tps = OSIX_TPS;
UINT4               gu4Stups = OSIX_STUPS;


VOID
UtlTrcLog (UINT4 u4Flag, UINT4 u4Value, const char *pi1Name,
           const char *pi1Fmt, ...)
{
	return;
}
/************************************************************************
 *  Function Name   : OsixSTUPS2Ticks
 *  Description     : Converts STUPS to Ticks.
 *  Inputs          : u4Time - Duration in STUPS.
 *  Returns         : Duration in Ticks.
 ************************************************************************/
UINT4
OsixSTUPS2Ticks (UINT4 u4Time)
{
    return (u4Time * (1000000 / gu4Stups));
}

/****************************************************************************/
/*                                                                          */
/*    Function Name      : UtlStrCaseCmp                                    */
/*                                                                          */
/*    Description        : Implementation of strcasecmp                     */
/*                                                                          */
/*    Input(s)           : pc1String1 and pc1String2 are the strings to     */
/*                         compare.                                         */
/*                                                                          */
/*    Output(s)          : None.                                            */
/*                                                                          */
/*    Returns            : Return an integer less than, equal to,           */
/*                         or greater than zero  if s1 (or  the  first  n   */
/*                         bytes thereof)  is  found,  respectively, to  be */
/*                         less than, to match, or be greater than s2.      */
/*                                                                          */
/****************************************************************************/
INT4
UtlStrCaseCmp (const CHR1 * pc1String1, const CHR1 * pc1String2)
{
    while (*pc1String1 != '\0'
           && tolower (*pc1String1) == tolower (*pc1String2))
    {
        pc1String1++;
        pc1String2++;
    }

    return (tolower (*pc1String1) - tolower (*pc1String2));
}

/************************************************************************/
/* Routines for task creation, deletion and maintenance                 */
/************************************************************************/
/************************************************************************/
/*  Function Name   : OsixTskCrt                                        */
/*  Description     : Creates task.                                     */
/*  Input(s)        : au1Name[ ] -        Name of the task              */
/*                  : u4TskPrio  -        Task Priority & Scheduling    */
/*                                        policy                        */
/*                  : u4StackSize -       Stack size                    */
/*                  : (*TskStartAddr)() - Entry point function          */
/*                  : u4Arg -             Arguments to above fn.        */
/*  Output(s)       : pTskId -            Task Id.                      */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixTskCrt (UINT1 au1TskName[], UINT4 u4TskPrio, UINT4 u4StackSize,
            VOID (*TskStartAddr) (INT1 *), INT1 *pArg, tOsixTaskId * pTskId)
{
    struct sched_param  SchedParam;
    pthread_attr_t      Attr;
    tOsixTsk           *pTsk = 0;
    VOID               *pId = NULL;
    UINT4               u4Idx;
    INT4                i4OsPrio;
    UINT4               u4SchedPolicy;
    UINT1               au1Name[OSIX_NAME_LEN + 4];

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    MEMCPY (au1Name, au1TskName, OSIX_NAME_LEN);

    pId = (VOID *) pTskId;

    if (OsixRscFind (au1Name, OSIX_TSK, &pId) == OSIX_SUCCESS)
    {
/*CAMEOTAG: Added by Aiyong, 2016-8-31, In the formal release, CAMEO_CLI_DBG_CMD_WANTED will not be defined.*/
#ifdef CAMEO_CLI_DBG_CMD_WANTED
        PRINTF("Repeat task name=%s\n",au1Name);
#endif
        UtlTrcLog (OSIX_TRC_FLAG, OSIX_TSK_TRC, "",
                   "OsixTskCrt, Repeat task name = %s \r\n", au1Name);
DEBUG();
        return (OSIX_FAILURE);    /* Task by this name already exists */
    }
    /* For tasks, the pThreads version of OsixRscAdd does not use */
    /* the last argument. So anything can be passed; we pass 0.   */
    if (OsixRscAdd (au1Name, OSIX_TSK, NULL) == OSIX_FAILURE)
    {DEBUG();
        return (OSIX_FAILURE);
    }

    u4SchedPolicy = u4TskPrio & 0xffff0000;

	/*CAMEOTAG: By Guixue 2014-01-18
	DES: replace by CM_ISS_PRI_TO_OS_PRI() for reusing*/
#if 0
    u4TskPrio = u4TskPrio & 0x0000ffff;

    /* Remap the task priority to Vxworks's range of values. */
    i4OsPrio = OS_LOW_PRIO + (((((INT4) (u4TskPrio) - FSAP_LOW_PRIO) *
                                (OS_HIGH_PRIO -
                                 OS_LOW_PRIO)) / (FSAP_HIGH_PRIO -
                                                  FSAP_LOW_PRIO)));
#endif

	CM_ISS_PRI_TO_OS_PRI(u4TskPrio, i4OsPrio);

    UtlTrcLog (OSIX_TRC_FLAG, OSIX_TSK_TRC, "",
               "OsixTskCrt (%s, %d, %d)\r\n", au1Name, u4TskPrio, u4StackSize);

    if (u4StackSize < OSIX_PTHREAD_MIN_STACK_SIZE)
    {
        u4StackSize = OSIX_PTHREAD_DEF_STACK_SIZE;
    }

    SchedParam.sched_priority = (UINT4) i4OsPrio;
    pthread_attr_init (&Attr);
    pthread_attr_setstacksize (&Attr, u4StackSize);

    switch (u4SchedPolicy)
    {
        case OSIX_SCHED_RR:
            pthread_attr_setinheritsched (&Attr, PTHREAD_EXPLICIT_SCHED);
            pthread_attr_setschedpolicy (&Attr, SCHED_RR);
            u4SchedPolicy = SCHED_RR;
            break;

        case OSIX_SCHED_FIFO:
            pthread_attr_setinheritsched (&Attr, PTHREAD_EXPLICIT_SCHED);
            pthread_attr_setschedpolicy (&Attr, SCHED_FIFO);
            u4SchedPolicy = SCHED_FIFO;
            break;

        case OSIX_SCHED_OTHER:
        default:
            u4SchedPolicy = SCHED_OTHER;
            pthread_attr_setschedpolicy (&Attr, SCHED_OTHER);
            break;

    }

    pthread_attr_setschedparam (&Attr, &SchedParam);

    /* Get the global task array index to return as the task id. */
    /* Return value check not needed because we just added it.   */
    OsixRscFind (au1Name, OSIX_TSK, &pId);
    u4Idx = *pTskId;

    pTsk = &gaOsixTsk[u4Idx];
    pTsk->u4Prio = (UINT4) i4OsPrio;
    pTsk->u4StackSize = u4StackSize;

    /* CondVars and associated mutexes,
     * (1) for task creation synch.
     * (2) for Events mechanism
     */
    /* pTsk replaced by original variable for klocwork false positive */
    pthread_cond_init (&((&gaOsixTsk[u4Idx])->EvtCond), NULL);
    pthread_mutex_init (&((&gaOsixTsk[u4Idx])->TskMutex), NULL);
    pthread_mutex_init (&((&gaOsixTsk[u4Idx])->EvtMutex), NULL);
	
    /* Store the TaskStartAddr and Args in TCB for further reference */
    pTsk->pTskStrtAddr = TskStartAddr;
    pTsk->pArg = pArg;

    /* We need the threadId to be stored in pTsk before pthreads gives
     * control to the entry point. So we use a "stub" function OsixTskWrapper
     * and use condvars to ensure an orderly creation.
     */
    pthread_mutex_lock (&(pTsk->TskMutex));

	int err;
	
    if (err = pthread_create (&(pTsk->ThrId), &Attr, OsixTskWrapper, (void *) pTsk))
    {
        pthread_mutex_unlock (&(pTsk->TskMutex));

        pthread_mutex_destroy (&(pTsk->TskMutex));
        pthread_mutex_destroy (&(pTsk->EvtMutex));
        pthread_cond_destroy (&(pTsk->EvtCond));
        pthread_attr_destroy (&Attr);
        OsixRscDel (OSIX_TSK, (VOID *) &u4Idx);
	    if (err != 0)
			printf("can't create thread: %s\n", strerror(err));
		DEBUG();
        return (OSIX_FAILURE);
    }

    /* In case of SCHED_OTHER, valid priority is not considered by pthread
     * create. In 2.6 kernel, sched other takes the priority of the parent
     * task. Since the parent root task runs with highest priority, sched
     * other tasks also runs with highest priority. So, the priority is set as
     * 0 explicitly
     */
    SchedParam.sched_priority = 0;
    /* Set the thread priority explicity. */
    if (u4SchedPolicy != SCHED_OTHER)
    {
        SchedParam.sched_priority = i4OsPrio;
    }

    pthread_setschedparam (pTsk->ThrId, u4SchedPolicy, &SchedParam);
    /* The OSIX task has been fully created. Now let the thread run. */
    pthread_mutex_unlock (&(pTsk->TskMutex));
    pthread_attr_destroy (&Attr);
	DEBUG();
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixTskDel                                        */
/*  Description     : Deletes a task.                                   */
/*  Input(s)        : TskId          - Task Id                          */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
VOID
OsixTskDel (tOsixTaskId TskId)
{
    UtlTrcLog (OSIX_TRC_FLAG, OSIX_TSK_TRC, "", "OsixTskDel (0x%x)\r\n", TskId);
    pthread_mutex_destroy (&(gaOsixTsk[TskId].TskMutex));
    pthread_mutex_destroy (&(gaOsixTsk[TskId].EvtMutex));
    pthread_cond_destroy (&(gaOsixTsk[TskId].EvtCond));
    if (pthread_self () == gaOsixTsk[TskId].ThrId)
    {
        pthread_detach (gaOsixTsk[TskId].ThrId);
        OsixRscDel (OSIX_TSK, (VOID *) &TskId);
        pthread_exit (NULL);
    }
    else
    {
        pthread_cancel (gaOsixTsk[TskId].ThrId);
        pthread_join (gaOsixTsk[TskId].ThrId, (void **) NULL);
        OsixRscDel (OSIX_TSK, (VOID *) &TskId);
    }
}

/************************************************************************/
/*  Function Name   : OsixTskDelay                                      */
/*  Description     : Suspends a task for a specified duration.         */
/*  Input(s)        : u4Duration - Delay time.                          */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixTskDelay (UINT4 u4Duration)
{
    UINT4               u4Sec;
    struct timespec     timeout;

    u4Duration = OsixSTUPS2Ticks (u4Duration);

    /* Assumption: OsixSTUPS2Ticks converts u4Duration to micro-seconds
     * We convert that into seconds and nano-seconds for the OS call.
     * If the values configured for OSIX_STUPS and OSIX_TPS change, this
     * code should be changed.
     */

    u4Sec = u4Duration / 1000000;
    timeout.tv_sec = u4Sec;
    timeout.tv_nsec = ((u4Duration - u4Sec * 1000000) * 1000);
    nanosleep (&timeout, NULL);
    return (OSIX_SUCCESS);
}


/************************************************************************/
/*  Function Name   : CmOsixTskUsDelay                                    */
/*  Description     : Suspends a task for a specified duration.         */
/*  Input(s)        : u4Duration - Delay time (microsecond).            */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
CmOsixTskUsDelay (UINT4 u4Duration)
{
    usleep(u4Duration);
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixDelay                                         */
/*  Description     : Suspends a task for a specified duration.         */
/*  Input(s)        : u4Duration - Delay time.                          */
/*                  : i4Unit     - Units in which the duration is       */
/*                                 specified                            */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixDelay (UINT4 u4Duration, INT4 i4Unit)
{
    struct timespec     timeout;
    UINT4               u4Sec = 0;
    UINT4               u4NSec = 0;

    switch (i4Unit)
    {
        case OSIX_SECONDS:
            u4Sec = u4Duration;
            break;

        case OSIX_CENTI_SECONDS:
            u4NSec = u4Duration * 10000000;
            break;

        case OSIX_MILLI_SECONDS:
            u4NSec = u4Duration * 1000000;
            break;

        case OSIX_MICRO_SECONDS:
            u4NSec = u4Duration * 1000;
            break;

        case OSIX_NANO_SECONDS:
            u4NSec = u4Duration;
            break;

        default:
            break;
    }

    timeout.tv_sec = u4Sec;
    timeout.tv_nsec = u4NSec;
    nanosleep (&timeout, NULL);
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixTskIdSelf                                     */
/*  Description     : Get Osix Id of current Task                       */
/*  Input(s)        : None                                              */
/*  Output(s)       : pTskId -            Task Id.                      */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixTskIdSelf (tOsixTaskId * pTskId)    /* Get ID of current task */
{
    UINT4               u4Count;
    pthread_t           ThrId;


	
    ThrId = pthread_self ();
	
    for (u4Count = 1; u4Count <= OSIX_MAX_TSKS; u4Count++)
    {
        if ((gaOsixTsk[u4Count].ThrId) == ThrId)
        {
        	if(gaOsixTsk[u4Count].u2Free == OSIX_FALSE)
        	{
            	*pTskId = (tOsixTaskId) u4Count;
    UtlTrcLog (OSIX_TRC_FLAG, OSIX_TSK_TRC, "",
               "OsixTskIdSelf (%s)(%d)\r\n", gaOsixTsk[u4Count].au1Name, *pTskId);
            	return (OSIX_SUCCESS);
            }
            else
            {
            	PRINTF("found but not used taskid=%d\n", u4Count);
            	continue;
            }
        }
    }


    return (OSIX_FAILURE);
}


/************************************************************************/
/*  Function Name   : CmOsixTskIdSelfWithLock                                   */
/*  Description     : This API is used for application to				*/
/*                    get Osix Id of current Task                       */
/*  Input(s)        : None                                              */
/*  Output(s)       : pTskId -            Task Id.                      */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
CmOsixTskIdSelfWithLock (tOsixTaskId * pTskId)   
{
    UINT4               u4Count;
    pthread_t           ThrId;


	/*CAMEOTAG: Bob Yu ++ on 2016/3/30 
      * Add sem4 lock/unclok when compare global gaOsixTsk
	 */
    
	if (pthread_mutex_lock (&gOsixMutex) != 0)
	{
		  return (OSIX_FAILURE);
    }

    ThrId = pthread_self ();

    for (u4Count = 1; u4Count <= OSIX_MAX_TSKS; u4Count++)
    {
        if ((gaOsixTsk[u4Count].ThrId) == ThrId)
        {
        	if(gaOsixTsk[u4Count].u2Free == OSIX_FALSE)
        	{
            *pTskId = (tOsixTaskId) u4Count;
            	 UtlTrcLog (OSIX_TRC_FLAG, OSIX_TSK_TRC, "",
               "OsixTskIdSelf (%s)(%d)\r\n", gaOsixTsk[u4Count].au1Name, *pTskId);     
   				pthread_mutex_unlock (&gOsixMutex);
            return (OSIX_SUCCESS);
        }
            else
            {
            	PRINTF("should not happen: task found but not used taskid=%d\n", u4Count);
            	continue;
            }
    }
    }
   	pthread_mutex_unlock (&gOsixMutex);
    return (OSIX_FAILURE);
}


/************************************************************************/
/* Routines for event management - send / receive event                 */
/************************************************************************/
/************************************************************************/
/*  Function Name   : OsixEvtSend                                       */
/*  Description     : Sends an event to a specified task.               */
/*  Input(s)        : TskId          - Task Id                          */
/*                  : u4Events       - The Events, OR-d                 */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixEvtSend (tOsixTaskId TskId, UINT4 u4Events)
{
    UINT4               u4Idx = (UINT4) TskId;

    UtlTrcLog (OSIX_TRC_FLAG, OSIX_EVT_TRC, "",
               "OsixEvtSend (%d, 0x%x)\r\n", (UINT4) TskId, u4Events);

	/* CAMEOTAG: Bob ++ at 2012/07/06 
	  * Somehow the task input has been del in other tasks
	  * , so add invlaid checking to prevent operation on released task.   
	 */
	if(gaOsixTsk[u4Idx].u2Free == OSIX_TRUE)
	{
		DEBUG();
		return (OSIX_FAILURE);
	}
	
    pthread_mutex_lock (&gaOsixTsk[u4Idx].EvtMutex);
    gaOsixTsk[u4Idx].u4Events |= u4Events;
    /* Noted by Aiyong:
     * Signal the Condition is avaiable now, at least one thread will wakeup from wait and process continue
     */
    pthread_cond_signal (&gaOsixTsk[u4Idx].EvtCond);
    pthread_mutex_unlock (&gaOsixTsk[u4Idx].EvtMutex);

    return (OSIX_SUCCESS);
}

UINT4
OsixEvtSendV1 (tOsixTaskId TskId, UINT4 u4Events)
{

    UINT4               u4Idx = (UINT4) TskId;
    pthread_mutex_lock(&gaOsixTsk[u4Idx].EvtMutex);
    gaOsixTsk[u4Idx].u4Events |= u4Events;
    pthread_cond_signal(&gaOsixTsk[u4Idx].EvtCond);
    pthread_mutex_unlock(&gaOsixTsk[u4Idx].EvtMutex);

    return (OSIX_SUCCESS);

#if 0


    UINT4               u4Idx = (UINT4) TskId;

    UtlTrcLog (OSIX_TRC_FLAG, OSIX_EVT_TRC, "",
               "OsixEvtSend (%d, 0x%x)\r\n", (UINT4) TskId, u4Events);

	/* CAMEOTAG: Bob ++ at 2012/07/06 
	  * Somehow the task input has been del in other tasks
	  * , so add invlaid checking to prevent operation on released task.   
	 */
	if(gaOsixTsk[u4Idx].u2Free == OSIX_TRUE)
	{
		DEBUG();
		return (OSIX_FAILURE);
	}
	
    pthread_mutex_lock (&gaOsixTsk[u4Idx].EvtMutex);
    gaOsixTsk[u4Idx].u4Events |= u4Events;
    /* Noted by Aiyong:
     * Signal the Condition is avaiable now, at least one thread will wakeup from wait and process continue
     */
    pthread_cond_signal (&gaOsixTsk[u4Idx].EvtCond);
    pthread_mutex_unlock (&gaOsixTsk[u4Idx].EvtMutex);

    return (OSIX_SUCCESS);
#endif
}

/************************************************************************/
/*  Function Name   : OsixEvtRecv                                       */
/*  Description     : To receive a event.                               */
/*  Input(s)        : TskId             - Task Id                       */
/*                  : u4Events          - List of interested events.    */
/*  Output(s)       : pu4EventsReceived - Events received.              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixEvtRecv (tOsixTaskId TskId, UINT4 u4Events, UINT4 u4Flg,
             UINT4 *pu4RcvEvents)
{
    UINT4               u4Idx = (UINT4) TskId;

    UtlTrcLog (OSIX_TRC_FLAG, OSIX_EVT_TRC, "",
               "OsixEvtRecv (%d, 0x%x, %d, 0x%x)\r\n", (UINT4) TskId,
               u4Events, u4Flg, pu4RcvEvents);
	/* CAMEOTAG: Bob ++ at 2012/07/06 
	  * Somehow the task input has been del in other tasks
	  * , so add invlaid checking to prevent operation on released task.   
	 */
	if(gaOsixTsk[u4Idx].u2Free == OSIX_TRUE)
		return (OSIX_SUCCESS);
	
    *pu4RcvEvents = 0;

    pthread_mutex_lock (&gaOsixTsk[u4Idx].EvtMutex);

    if ((u4Flg == OSIX_NO_WAIT) &&
        (((gaOsixTsk[u4Idx].u4Events) & u4Events) == 0))
    {
        pthread_mutex_unlock (&gaOsixTsk[u4Idx].EvtMutex);
        return (OSIX_FAILURE);
    }

    while (1)
    {
    	
        if (((gaOsixTsk[u4Idx].u4Events) & u4Events) != 0)
        {
            /* A required event has happened */
            *pu4RcvEvents = (gaOsixTsk[u4Idx].u4Events) & u4Events;
            gaOsixTsk[u4Idx].u4Events &= ~(*pu4RcvEvents);
            pthread_mutex_unlock (&gaOsixTsk[u4Idx].EvtMutex);
            return (OSIX_SUCCESS);
        }
        /* Noted by Aiyong:
         * 1. pthread_cond_wait at first check the ".EvnCond" whether it is avaiable, now ".EvtMutex" is locked 
         * 2. after check the condition and put it in the wait queue, then unlock the mutex ".EvtMutx" 
         * 3. if the condition is avaiable, will return from pthread_cond_wait, and lock mutex again 
         */
        pthread_cond_wait (&gaOsixTsk[u4Idx].EvtCond,
                           &gaOsixTsk[u4Idx].EvtMutex);
    }
}

UINT4
OsixEvtRecvV1 (tOsixTaskId TskId, UINT4 u4Events, UINT4 u4Flg,
             UINT4 *pu4RcvEvents)
{

    UINT4 u4Idx = (UINT4) TskId;
    *pu4RcvEvents = 0;

    pthread_mutex_lock(&gaOsixTsk[u4Idx].EvtMutex);
    while(1)
    {
        if(((gaOsixTsk[u4Idx].u4Events) & u4Events) != 0)
        {
            *pu4RcvEvents = (gaOsixTsk[u4Idx].u4Events) & u4Events;
            gaOsixTsk[u4Idx].u4Events &= ~(*pu4RcvEvents); 
            pthread_mutex_unlock(&gaOsixTsk[u4Idx].EvtMutex);
            return (OSIX_SUCCESS);
        }
        pthread_cond_wait(&gaOsixTsk[u4Idx].EvtCond, &gaOsixTsk[u4Idx].EvtMutex);
    }
        
#if 0
    UINT4               u4Idx = (UINT4) TskId;

    UtlTrcLog (OSIX_TRC_FLAG, OSIX_EVT_TRC, "",
               "OsixEvtRecv (%d, 0x%x, %d, 0x%x)\r\n", (UINT4) TskId,
               u4Events, u4Flg, pu4RcvEvents);
	/* CAMEOTAG: Bob ++ at 2012/07/06 
	  * Somehow the task input has been del in other tasks
	  * , so add invlaid checking to prevent operation on released task.   
	 */
	if(gaOsixTsk[u4Idx].u2Free == OSIX_TRUE)
		return (OSIX_SUCCESS);
	
    *pu4RcvEvents = 0;

    pthread_mutex_lock (&gaOsixTsk[u4Idx].EvtMutex);

    if ((u4Flg == OSIX_NO_WAIT) &&
        (((gaOsixTsk[u4Idx].u4Events) & u4Events) == 0))
    {
        pthread_mutex_unlock (&gaOsixTsk[u4Idx].EvtMutex);
        return (OSIX_FAILURE);
    }

    while (1)
    {
    	
        if (((gaOsixTsk[u4Idx].u4Events) & u4Events) != 0)
        {
            /* A required event has happened */
            *pu4RcvEvents = (gaOsixTsk[u4Idx].u4Events) & u4Events;
            gaOsixTsk[u4Idx].u4Events &= ~(*pu4RcvEvents);
            pthread_mutex_unlock (&gaOsixTsk[u4Idx].EvtMutex);
            return (OSIX_SUCCESS);
        }
        /* Noted by Aiyong:
         * 1. pthread_cond_wait at first check the ".EvnCond" whether it is avaiable, now ".EvtMutex" is locked 
         * 2. after check the condition and put it in the wait queue, then unlock the mutex ".EvtMutx" 
         * 3. if the condition is avaiable, will return from pthread_cond_wait, and lock mutex again 
         */
        pthread_cond_wait (&gaOsixTsk[u4Idx].EvtCond,
                           &gaOsixTsk[u4Idx].EvtMutex);
    }
#endif

}


/************************************************************************/
/*  Function Name   : OsixInitialize                                    */
/*  Description     : The OSIX Initialization routine. To be called     */
/*                    before any other OSIX functions are used.         */
/*  Input(s)        : pOsixCfg - Pointer to OSIX config info.           */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixInitialize ()
{
    UINT4               u4Idx;
    struct tms          buf;

    /* Mutual exclusion semaphore to add or delete elements from */
    /* name-id-mapping list. This semaphore itself will not be   */
    /* added to the name-to-id mapping list. This is a pThreads */
    /* specific call and must be mapped to relevant call for OS  */

    if (pthread_mutex_init (&gOsixMutex, NULL))
    {
        return (OSIX_FAILURE);
    }

    /* Initialize all arrays. */
    for (u4Idx = 0; u4Idx <= OSIX_MAX_TSKS; u4Idx++)
    {
        gaOsixTsk[u4Idx].u2Free = OSIX_TRUE;
        gaOsixTsk[u4Idx].u4Events = 0;
        MEMSET (gaOsixTsk[u4Idx].au1Name, '\0', (OSIX_NAME_LEN + 4));
    }
    for (u4Idx = 0; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
    {
        gaOsixSem[u4Idx].u2Free = OSIX_TRUE;
        gaOsixSem[u4Idx].u2Filler = 0;
        MEMSET (gaOsixSem[u4Idx].au1Name, '\0', (OSIX_NAME_LEN + 4));
    }
    for (u4Idx = 0; u4Idx <= OSIX_MAX_QUES; u4Idx++)
    {
        gaOsixQue[u4Idx].u2Free = OSIX_TRUE;
        gaOsixQue[u4Idx].u2Filler = 0;
        MEMSET (gaOsixQue[u4Idx].au1Name, '\0', (OSIX_NAME_LEN + 4));
    }

    gStartTicks = times (&buf);

	/* CMAEOTAG: add by guofeng 2014/2/18, get os schedule priority range.
	    Only SCHED_RR and SCHED_FIFO have priority value, 
	    below just use SCHED_RR to get the range. */
	gi4MinPrio = sched_get_priority_min(SCHED_RR);
	gi4MaxPrio = sched_get_priority_max(SCHED_RR);

	/* if fail, -1 is returned */
	if( gi4MinPrio == -1 || gi4MaxPrio == -1 )
	{
		pthread_mutex_destroy(&gOsixMutex);
		return (OSIX_FAILURE);
	}
	
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixShutDown                                      */
/*  Description     : Stops OSIX.                                       */
/*  Input(s)        : None.                                             */
/*  Output(s)       : None.                                             */
/*  Returns         : OSIX_SUCCESS                                      */
/************************************************************************/
UINT4
OsixShutDown ()
{
    /* Killing the current process for shutdown */
    kill (getpid (), SIGKILL);
    return (OSIX_SUCCESS);
}

/************************************************************************/
/* Routines for managing semaphores                                     */
/* Keep it simple - support 1 type of semaphore - binary, blocking.     */
/* After creating, the semaphore must be given before it can be taken.  */
/************************************************************************/


/************************************************************************
 *  Function Name   : OsixCreateSem
 *  Description     : This creates a sema4 of a given name.
 *  Input           : au1SemName  - Name of sema4.
 *                    u4InitialCount - Initial value of sema4.
 *                    u4Flags        - Unused.
 *  Output          : pSemId - Pointer to memory which contains SEM-ID
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE.
 ************************************************************************/
UINT4
OsixCreateSem (const UINT1 au1SemName[4], UINT4 u4InitialCount,
               UINT4 u4Flags, tOsixSemId * pSemId)
{
    UINT1               au1Name[OSIX_NAME_LEN + 4], u1Index;

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    for (u1Index = 0;
         ((u1Index < OSIX_NAME_LEN) && (au1SemName[u1Index] != '\0'));
         u1Index++)
    {
        au1Name[u1Index] = au1SemName[u1Index];
    }

    ((VOID) u4Flags);

    if (OsixRscFind (au1Name, OSIX_SEM, (VOID **) pSemId) == OSIX_SUCCESS)
    {
    	/*CAMEOTAG: Bob ++ at 2012/8/17 */
        *(tOsixSemId *)pSemId = NULL;
        /* Semaphore by this name already exists. */
        return (OSIX_FAILURE);
    }
    if (OsixSemCrt (au1Name, pSemId) == OSIX_FAILURE)
    {
    	/*CAMEOTAG: Bob ++ at 2012/8/17 */
        *(tOsixSemId *)pSemId = NULL;
        return (OSIX_FAILURE);
    }
    /* Implementation assumes that mutex is created when u4InitialCount */
    /* is 1, otherwise sem is for task sync. OsixSemCrt creates binary  */
    /* sem in blocked state. So, if u4InitialCount is 1, give the sem.  */
    if (u4InitialCount == 1)
    {
        OsixSemGive (*pSemId);
    }
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixSemCrt                                        */
/*  Description     : Creates a sema4.                                  */
/*  Input(s)        : au1Name [ ] - Name of the sema4.                  */
/*  Output(s)       : pSemId         - The sema4 Id.                    */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixSemCrt (UINT1 au1SemName[], tOsixSemId * pSemId)
{
    UINT1               au1Name[OSIX_NAME_LEN + 4];
    VOID               *pId = NULL;
    tOsixTaskId         tskId;
    UINT4               *pu4RscId = NULL;

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    MEMCPY (au1Name, au1SemName, OSIX_NAME_LEN);

    /* For sem, the pThreads version of OsixRscAdd does not use */
    /* the last argument. So anything can be passed; we pass 0. */
    UtlTrcLog (OSIX_TRC_FLAG, OSIX_SEM_TRC, "", "OsixSemCrt (%s)\r\n", au1Name);

    pId = pSemId;
    if (OsixRscFind (au1Name, OSIX_SEM, pId) == OSIX_SUCCESS)
    {
    	/* CMAMEOTAG: Bob ++ 
    	 * When sema is found, init pSemId to NULL
    	 */
    	*(tOsixSemId *)pSemId = NULL; 
        /* Semaphore by this name already exists. */
        return (OSIX_FAILURE);
    }
    
    /*some semaphore is created by main pthread which didnt record in gaOsixTsk*/
    if(OsixTskIdSelf(&tskId) == OSIX_SUCCESS)
    {
        pu4RscId = (UINT4*) &tskId;
    }

    if (OsixRscAdd (au1Name, OSIX_SEM, pu4RscId) == OSIX_FAILURE)
    {
        /* CMAMEOTAG: Bob ++ 
        * init pSemId to NULL
        */
        *(tOsixSemId *)pSemId = NULL; 
        return (OSIX_FAILURE);
    }

    /* the pThread version of OsixRscFind returns pointer to void pointer,
     * hence assigned the semId pointer to void pointer to remove dereferncing 
     * type-punned warning */
    pId = pSemId;
    OsixRscFind (au1Name, OSIX_SEM, pId);
    if (sem_init (*pSemId, 0, 0))
    {
        OsixRscDel (OSIX_SEM, (VOID *) *pSemId);
    	/* CMAMEOTAG: Bob ++ 
    	 * init pSemId to NULL
    	 */
        *(tOsixSemId *)pSemId = NULL;
        return (OSIX_FAILURE);
    }
   

    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixSemDel                                        */
/*  Description     : Deletes a sema4.                                  */
/*  Input(s)        : pSemId         - The sema4 Id.                    */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
VOID
OsixSemDel (tOsixSemId SemId)
{
    UtlTrcLog (OSIX_TRC_FLAG, OSIX_SEM_TRC, "", "OsixSemDel (0x%x)\r\n", SemId);
    OsixRscDel (OSIX_SEM, (VOID *) SemId);
    sem_destroy (SemId);
}

/************************************************************************/
/*  Function Name   : OsixSemGive                                       */
/*  Description     : Used to release a sem4.                           */
/*  Input(s)        : pSemId         - The sema4 Id.                    */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixSemGive (tOsixSemId SemId)
{
    
	if(SemId == 0)
	{
		return OSIX_FAILURE;
	}
    if (sem_post ((sem_t *) SemId) < 0)
    {
        UtlTrcLog (OSIX_TRC_FLAG, OSIX_TSK_TRC, "", "OsixSemGive (0x%x) failed\r\n", SemId);
        return (OSIX_FAILURE);
    }
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixSemTake                                       */
/*  Description     : Used to acquire a sema4.                          */
/*  Input(s)        : pSemId         - The sema4 Id.                    */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixSemTake (tOsixSemId SemId)
{
    UINT4               u4RetVal = OSIX_SUCCESS;
   
	if(SemId == 0)
	{
		UtlTrcLog (OSIX_TRC_FLAG, OSIX_TSK_TRC, "", "OsixSemTake (0x%x) failed\r\n", SemId);
		return OSIX_FAILURE;
	}
    while (sem_wait ((sem_t *) SemId) != 0)
    {
        if (errno != EINTR)
        {
            u4RetVal = OSIX_FAILURE;
            assert (0);
        }
    }
    return (u4RetVal);
}
/*CAMOTAG:Add by toney 2019/04/30
For all locked semaphore dedug*/
/************************************************************************/
/*  Function Name   : CmOsixSemDebug                                    */
/*  Description     : Used to debug all sema4.                          */
/*  Input(s)        : None.                                             */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
VOID CmOsixSemDebug(VOID)
{
    UINT4 u4Idx = 0;
    INT4  val = 0;
    
    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return;
    }

    /* scan global semaphore array to find the semaphore */
    for (u4Idx = 1; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
    {
        if((gaOsixSem[u4Idx].u2Free) == OSIX_FALSE)
        {
            /*If sem is locked, then the value returned by sem_getvalue() is either zero or a negative number 
            whose absolute value represents the number of processes waiting for the semaphore at some unspecified 
            time during the call*/
            sem_getvalue((sem_t *) &(gaOsixSem[u4Idx].SemId),&val);
            if(val <= 0)
            {
                printf("Task [%d:%s]'s semaphore [%d] locked \n",gaOsixSem[u4Idx].TskId, OsixExGetTaskName(gaOsixSem[u4Idx].TskId), u4Idx);
            }

        }
    }
    if (pthread_mutex_unlock (&gOsixMutex) != 0)
    {
        return;
    }

}

/************************************************************************/
/* Routines for managing message queues                                 */
/************************************************************************/
/************************************************************************/
/*  Function Name   : OsixQueCrt                                        */
/*  Description     : Creates a OSIX Q.                                 */
/*  Input(s)        : au1name[ ] - The Name of the Queue.               */
/*                  : u4MaxMsgs  - Max messages that can be held.       */
/*                  : u4MaxMsgLen- Max length of a messages             */
/*  Output(s)       : pQueId     - The QId returned.                    */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixQueCrt (UINT1 au1QName[], UINT4 u4MaxMsgLen,
            UINT4 u4MaxMsgs, tOsixQId * pQueId)
{
    UINT1               au1Name[OSIX_NAME_LEN + 4];
    VOID               *pId = NULL;

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    MEMCPY (au1Name, au1QName, OSIX_NAME_LEN);

    UtlTrcLog (OSIX_TRC_FLAG, OSIX_QUE_TRC, "", "OsixQueCrt (%s, %d, %d)\r\n",
               au1Name, u4MaxMsgLen, u4MaxMsgs);

    /* the pThread version of OsixRscFind returns pointer to void pointer,
     * hence assigned the QueId pointer to void pointer to remove dereferncing 
     * type-punned warning */
    pId = pQueId;
    if (OsixRscFind (au1Name, OSIX_QUE, pId) == OSIX_SUCCESS)
    {
        return (OSIX_FAILURE);
    }

    *pQueId = (tOsixQId) PTHREAD_Create_MsgQ (u4MaxMsgs, u4MaxMsgLen);
    if (*pQueId == NULL)
    {
        return (OSIX_FAILURE);
    }
    if (OsixRscAdd (au1Name, OSIX_QUE, (UINT4 *) *pQueId) == OSIX_FAILURE)
    {
        PTHREAD_Delete_MsgQ ((tPthreadQId) (*pQueId));
        return (OSIX_FAILURE);
    }
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixQueDel                                        */
/*  Description     : Deletes a Q.                                      */
/*  Input(s)        : QueId     - The QId returned.                     */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS                                      */
/************************************************************************/
void
OsixQueDel (tOsixQId QueId)
{
    UtlTrcLog (OSIX_TRC_FLAG, OSIX_QUE_TRC, "", "OsixQueDel (%d)\r\n", QueId);
    OsixRscDel (OSIX_QUE, QueId);
    PTHREAD_Delete_MsgQ ((tPthreadQId) QueId);
    return;
}

/************************************************************************/
/*  Function Name   : OsixQueSend                                       */
/*  Description     : Sends a message to a Q.                           */
/*  Input(s)        : QueId -    The Q Id.                              */
/*                  : pu1Msg -   Pointer to message to be sent.         */
/*                  : u4MsgLen - length of the messages                 */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixQueSend (tOsixQId QueId, UINT1 *pu1Msg, UINT4 u4MsgLen)
{
    u4MsgLen = 0;

    UtlTrcLog (OSIX_TRC_FLAG, OSIX_QUE_TRC, "",
               "OsixQueSend (%d, 0x%x, %d)\r\n", QueId, pu1Msg, u4MsgLen);
    /* Typically native OS calls take message Length as an argument.
     * In the case of PThreads Queues, the value supplied in the call
     * OsixQueCrt is used implicitly as the message length.
     * Hence the u4MsgLen parameter is not used in this function.
     */
    if (PTHREAD_Send_MsgQ ((tPthreadQId) QueId, pu1Msg) != 0)
    {
        return (OSIX_FAILURE);
    }
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixQueRecv                                       */
/*  Description     : Receives a message from a Q.                      */
/*  Input(s)        : QueId -     The Q Id.                             */
/*                  : u4MsgLen -  length of the messages                */
/*                  : i4Timeout - Time to wait in case of WAIT.         */
/*  Output(s)       : pu1Msg -    Pointer to message to be sent.        */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixQueRecv (tOsixQId QueId, UINT1 *pu1Msg, UINT4 u4MsgLen, INT4 i4Timeout)
{
    u4MsgLen = 0;
    DEBUG();

    PRINTF("[%s][%d] Recv Str:%p.\n", __FUNCTION__, __LINE__, pu1Msg);

    UtlTrcLog (OSIX_TRC_FLAG, OSIX_QUE_TRC, "", "OsixQueRecv (%d)\r\n", QueId);
    /* Typically native OS calls take message Length as an argument.
     * In the case of Pthreads Queues, the value supplied in the call
     * OsixQueCrt is used implicitly as the message length.
     * Hence the u4MsgLen parameter is not used in this function.
     */
    if (i4Timeout > 0)
    {
        i4Timeout = (INT4) OsixSTUPS2Ticks ((UINT4) i4Timeout);
    }
    DEBUG();
    if (PTHREAD_Receive_MsgQ ((tPthreadQId) QueId, pu1Msg, i4Timeout) != 0)
    {
        return (OSIX_FAILURE);
    }
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixQueNumMsgs                                    */
/*  Description     : Returns No. of messages currently in a Q.         */
/*  Input(s)        : QueId -     The Q Id.                             */
/*  Output(s)       : pu4NumberOfMsgs - Contains count upon return.     */
/*  Returns         : OSIX_SUCCESS                                      */
/************************************************************************/
UINT4
OsixQueNumMsg (tOsixQId QueId, UINT4 *pu4NumMsg)
{
    tPthreadQ          *pPthreadQ = ((tPthreadQ *) QueId);
    UtlTrcLog (OSIX_TRC_FLAG, OSIX_QUE_TRC, "", "OsixQueNumMsg (%d)\r\n",
               QueId);
    if (pthread_mutex_lock (pPthreadQ->QueMutex))
    {
        return (OSIX_FAILURE);
    }

    *pu4NumMsg = (UINT4) (PTHREAD_MsgQ_NumMsgs ((tPthreadQId) QueId));
    pthread_mutex_unlock (pPthreadQ->QueMutex);

    return (OSIX_SUCCESS);
}

/************************************************************************/
/* Routines for managing resources based on names                       */
/************************************************************************/
/************************************************************************/
/*  Function Name   : OsixRscAdd                                        */
/*  Description     : Gets a free resouce, stores Resource-Id & Name    */
/*                  : This helps in locating Resource-Id by Name        */
/*  Input(s)        : au1Name -   Name of the resource                  */
/*                  : u4RscType - Type of resource (Task/Queue/Sema4)   */
/*                  : u4RscId -   Resource-Id returned by OS            */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixRscAdd (UINT1 au1Name[], UINT4 u4RscType, UINT4 *pu4RscId)
{
    UINT4               u4Idx;
    static UINT4        u4SemIdx = 1;

    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return (OSIX_FAILURE);
    }

    switch (u4RscType)
    {
        case OSIX_TSK:
            /* scan global task array to find a free slot */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_TSKS; u4Idx++)
            {
                if ((gaOsixTsk[u4Idx].u2Free) == OSIX_TRUE)
                {
                    gaOsixTsk[u4Idx].u2Free = OSIX_FALSE;
                    gaOsixTsk[u4Idx].u4Events = 0;
                    MEMCPY (gaOsixTsk[u4Idx].au1Name, au1Name,
                            (OSIX_NAME_LEN + 4));
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        case OSIX_SEM:
            
            /* CAMEOTAG : Bob Yu + u4SemIdx on 2016/8/12 to avoid duplication of sem4 generation.
             * By Dude TCP attacking, the duplicated sem4, which previous one is deleted by deallocate 
             * but high layer will not know and intend to taking or taken.
             * in such case, it will let the new tcp connection with the same sem4
             * get lock at TcpProcessQMsg or TcpTmrHandleExpiry.    
             * And then TCP main thread will not process events anymore to 
             * see "Unable to allocate TCP_HL_Parms" at IptaskTcpInput. 
             */
             /* scan global semaphore array to find a free slot */
            for (u4Idx = u4SemIdx; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
            {
                if ((gaOsixSem[u4Idx].u2Free) == OSIX_TRUE)
                {
                    gaOsixSem[u4Idx].u2Free = OSIX_FALSE;
                    gaOsixSem[u4Idx].u2Filler = 0;
                    
                    /*CAMOTAG:Add by toney 2019/04/30
                    To record task id that create this semaphore 
                    */
                    if(pu4RscId)
                    {
                        gaOsixSem[u4Idx].TskId = *((tOsixTaskId*)pu4RscId);
                    }
                    MEMCPY (gaOsixSem[u4Idx].au1Name, au1Name,
                            (OSIX_NAME_LEN + 4));
                    pthread_mutex_unlock (&gOsixMutex);
                    u4SemIdx = u4Idx;
                    return (OSIX_SUCCESS);
                }
            }
            /* CAMEOTAG : Bob Yu ++ on 2016/8/12 
             * if u4SemIdx reachs to the end, let index to be 1 and scan.
             */
            for (u4Idx = 1; u4Idx < u4SemIdx; u4Idx++)
            {
                if ((gaOsixSem[u4Idx].u2Free) == OSIX_TRUE)
                {
                    gaOsixSem[u4Idx].u2Free = OSIX_FALSE;
                    gaOsixSem[u4Idx].u2Filler = 0;
                    MEMCPY (gaOsixSem[u4Idx].au1Name, au1Name,
                            (OSIX_NAME_LEN + 4));
                    pthread_mutex_unlock (&gOsixMutex);
                    u4SemIdx = u4Idx;
                    return (OSIX_SUCCESS);
                }
            }
            break;

        case OSIX_QUE:
            /* scan global queue array to find a free slot */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_QUES; u4Idx++)
            {
                if ((gaOsixQue[u4Idx].u2Free) == OSIX_TRUE)
                {
                    gaOsixQue[u4Idx].pRscId = pu4RscId;
                    gaOsixQue[u4Idx].u2Free = OSIX_FALSE;
                    gaOsixQue[u4Idx].u2Filler = 0;
                    MEMCPY (gaOsixQue[u4Idx].au1Name, au1Name,
                            (OSIX_NAME_LEN + 4));
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        default:
            break;
    }

    pthread_mutex_unlock (&gOsixMutex);
    return (OSIX_FAILURE);
}

/************************************************************************/
/*  Function Name   : OsixRscDel                                        */
/*  Description     : Free an allocated resouce                         */
/*  Input(s)        : u4RscType - Type of resource (Task/Queue/Sema4)   */
/*                  : u4RscId -   Resource-Id returned by OS            */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
VOID
OsixRscDel (UINT4 u4RscType, VOID *pRscId)
{
    UINT4               u4Idx;

    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return;
    }

    switch (u4RscType)
    {
        case OSIX_TSK:
            u4Idx = *((UINT4 *) pRscId);
            gaOsixTsk[u4Idx].u2Free = OSIX_TRUE;
            MEMSET (gaOsixTsk[u4Idx].au1Name, '\0', (OSIX_NAME_LEN + 4));
            break;

        case OSIX_SEM:
            /* scan global semaphore array to find the semaphore */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
            {
                if (&(gaOsixSem[u4Idx].SemId) == (sem_t *) pRscId)
                {
                    gaOsixSem[u4Idx].u2Free = OSIX_TRUE;
                    MEMSET (gaOsixSem[u4Idx].au1Name, '\0',
                            (OSIX_NAME_LEN + 4));
                    break;
                }
            }
            break;

        case OSIX_QUE:
            /* scan global queue array to find the queue */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_QUES; u4Idx++)
            {
                if ((gaOsixQue[u4Idx].pRscId) == pRscId)
                {
                    gaOsixQue[u4Idx].u2Free = OSIX_TRUE;
                    MEMSET (gaOsixQue[u4Idx].au1Name, '\0',
                            (OSIX_NAME_LEN + 4));
                    break;
                }
            }
            break;

        default:
            break;
    }
    pthread_mutex_unlock (&gOsixMutex);
}

/************************************************************************/
/*  Function Name   : OsixRscFind                                       */
/*  Description     : This locates Resource-Id by Name                  */
/*  Input(s)        : au1Name -   Name of the resource                  */
/*                  : u4RscType - Type of resource (Task/Queue/Sema4)   */
/*  Output(s)       : pu4RscId - Osix Resource-Id                       */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixRscFind (UINT1 au1Name[], UINT4 u4RscType, VOID **pRscId)
{
    UINT4               u4Idx;
    UINT4              *pu4Id = NULL;

    if (STRCMP (au1Name, "") == 0)
    {
        return (OSIX_FAILURE);
    }
    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return (OSIX_FAILURE);
    }

    switch (u4RscType)
    {
        case OSIX_TSK:
            /* scan global task array to find the task */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_TSKS; u4Idx++)
            {
                if (MEMCMP
                    (au1Name, gaOsixTsk[u4Idx].au1Name,
                     (OSIX_NAME_LEN + 4)) == 0)
                {
                	
                	/* CAMEOTAG : Bob Yu Added on 2016/5/12, it needs to 
                	 * ensure task's status is 'used' 
                	 */
                    if (gaOsixTsk[u4Idx].u2Free == OSIX_TRUE)
                    {
                    	 continue;
                   	}
                    /***
                     * For the case of tasks, applications know only our array
                     * index.  This helps us to simulate events.             
                     ***/
                    pu4Id = *pRscId;
                    *pu4Id = u4Idx;
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        case OSIX_SEM:
            /* scan global semaphore array to find the semaphore */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
            {
                if (MEMCMP
                    (au1Name, gaOsixSem[u4Idx].au1Name,
                     (OSIX_NAME_LEN + 4)) == 0)
                {
                    /* pThread version of OsixRscFind returns pointer to semId */
                    *pRscId = (void *) &gaOsixSem[u4Idx].SemId;
                    pthread_mutex_unlock (&gOsixMutex);
                    /*CAMEOTAG: Bob ++ at 2012/8/17 */
        			/*TODO: debug messsage for "Accepted Failed", rmoving it later */
                    /* To find max value of u4Idx to check sem's macro tuning */
                    if (gu4MaxSem < u4Idx)
                    {
                        /* update the Max Sem number */
                        gu4MaxSem = u4Idx;
                    }
                    return (OSIX_SUCCESS);
                }
            }
            break;

        case OSIX_QUE:
            /* scan global queue array to find the queue */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_QUES; u4Idx++)
            {
                if (MEMCMP
                    (au1Name, gaOsixQue[u4Idx].au1Name,
                     (OSIX_NAME_LEN + 4)) == 0)
                {
                    *pRscId = (void *) gaOsixQue[u4Idx].pRscId;
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        default:
            break;
    }
    pthread_mutex_unlock (&gOsixMutex);
    return (OSIX_FAILURE);
}

/************************************************************************/
/*  Function Name   : Fsap2Start                                        */
/*  Description     : Function to be called to get any fsap2 application*/
/*                  : to work                                           */
/*  Input(s)        : None                                              */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
VOID
Fsap2Start ()
{
    while (1)
    {
        sleep (1);
    }
}

/************************************************************************/
/*  Function Name   : OsixGetSysTime                                    */
/*  Description     : Returns the system time in STUPS.                 */
/*  Input(s)        : None                                              */
/*  Output(s)       : pSysTime - The System time.                       */
/*  Returns         : None                                              */
/************************************************************************/
UINT4
OsixGetSysTime (tOsixSysTime * pSysTime)
{

    clock_t             CurTicks;
    UINT4               u4TicksPerStup;
    struct tms          buf;

    u4TicksPerStup = sysconf (_SC_CLK_TCK) / gu4Stups;

    CurTicks = times (&buf);
    *pSysTime = (CurTicks - gStartTicks) / u4TicksPerStup;

    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixGetSysTimeIn64                                */
/*  Description     : Returns the system time in STUPS.                 */
/*  Input(s)        : None                                              */
/*  Output(s)       : pSysTime - The System time.                       */
/*  Returns         : None                                              */
/************************************************************************/
UINT4
OsixGetSysTimeIn64Bit (FS_UINT8 * pSysTime)
{
    /* Currently we have support for U4 Only */
    pSysTime->u4Hi = 0;
    OsixGetSysTime (&pSysTime->u4Lo);

    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixGetSysUpTime                                  */
/*  Description     : Returns the system time in Seconds.               */
/*  Input(s)        : None                                              */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
UINT4
OsixGetSysUpTime (VOID)
{
    return (gu4Seconds);
}

/************************************************************************/
/*  Function Name   : CmOsixGetNanoTimeTick                                    */
/*  Description     : Returns the time tick by Nanosec                 */
/*  Input(s)        : None                                              */
/*  Output(s)       : nanotime.                       */
/*  Returns         : None                                              */
/************************************************************************/
UINT4
CmOsixGetMicroTimeTick (VOID)
{

    return (gCmu4MicroSeconds);
}


/************************************************************************/
/*  Function Name   : OsixExGetTaskName                                 */
/*  Description     : Get the OSIX task Name given the Osix Task-Id.    */
/*  Input(s)        : TaskId - The Osix taskId.                         */
/*  Output(s)       : None.                                             */
/*  Returns         : Pointer to OSIX task name                         */
/************************************************************************/
const UINT1        *
OsixExGetTaskName (tOsixTaskId TskId)
{
    /* To facilitate direct use of this call in a STRCPY,
     * we return a null string instead of a NULL pointer.
     * A null pointer is returned for all non-OSIX tasks
     * e.g., TMO's idle task.
     */
    if (TskId)
    {
        return ((UINT1 *) (gaOsixTsk[(UINT4) TskId].au1Name));
    }
    return ((const UINT1 *) "");
}

/************************************************************************/
/*  Function Name   : OsixTaskWrapper                                   */
/*  Description     : Intermediate function between OsixTskCrt and      */
/*                  : application entry point function, which serves    */
/*                  : to prevent the application from kicking off even  */
/*                  : before CreateTask has completed. The synch is     */
/*                  : accomplished through the use of a per task mutex. */
/*                  : The mutex is 'taken' just before pthread_create   */
/*                  : and is given up just before close of CreateTask.  */
/*  Input(s)        : pArg - task arguments passed here.                */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
static VOID        *
OsixTskWrapper (void *pArg)
{
    void                (*TaskPtr) (INT1 *);
    tOsixTsk           *pTsk = (tOsixTsk *) pArg;
    tOsixTaskId         TskId;
    INT1               *pFuncArg = NULL;

    /* Waits till OsixCreateTask releases the lock */
    pthread_mutex_lock (&(pTsk->TskMutex));

    /* OsixCreateTask is complete, now releases the lock */
    pthread_mutex_unlock (&(pTsk->TskMutex));

    TaskPtr = pTsk->pTskStrtAddr;
    pFuncArg = pTsk->pArg;

    /*
     * DEBUG TIP:
     * Use this to see taskname to PID mapping as reported by
     * UNIX 'ps' listing. The threadID is not if much use.
     *
     * printf ("Starting %s with thrid %d and PID %d\n", pTsk->au1Name,
     *        pTsk->ThrId, getpid());
     */
    pTsk->u4Pid = (UINT4) getpid ();
    /* Call the actual application Entry Point function. */
    (*TaskPtr) (pFuncArg);

    OsixTskIdSelf (&TskId);
    OsixTskDel (TskId);

    return ((void *) NULL);
}

/************************************************************************/
/*  Function Name   : PTHREAD_Create_MsgQ                               */
/*  Description     : Creates a queue using a linear block of memory.   */
/*  Input(s)        : u4MaxMsgs  - Max messages that can be held.       */
/*                  : u4MaxMsgLen- Max length of a messages             */
/*  Output(s)       : None                                              */
/*  Returns         : Queue-Id, NULL if creation fails                  */
/************************************************************************/
tPthreadQId
PTHREAD_Create_MsgQ (UINT4 u4MaxMsgs, UINT4 u4MsgLen)
{
    pthread_mutex_t    *qSem;
    tPthreadQ          *pPthreadQ;

    /* Allocate memory for holding messages. Create a semaphore for      */
    /* protection between multiple simultaneous calls to write or read   */
    /* Initialize the read and write pointers to the Q start location    */
    /* Initia the pointer marking the end of the queue's memory location */
    u4MsgLen = sizeof (void *);

    pPthreadQ = MEM_MALLOC ((((u4MaxMsgs + 1) * u4MsgLen) +
                              sizeof (tPthreadQ)), tPthreadQ);
    if (pPthreadQ == NULL)
    {
        return (NULL);
    }
    pPthreadQ->pQBase = (UINT1 *) ((UINT1 *) pPthreadQ + sizeof (tPthreadQ));

    qSem = MEM_MALLOC (sizeof (pthread_mutex_t), pthread_mutex_t);
    if (qSem == NULL)
    {
        MEM_FREE (pPthreadQ);
        return (NULL);
    }
    pthread_mutex_init (qSem, 0);
    pPthreadQ->QueMutex = qSem;

    pthread_cond_init (&(pPthreadQ->QueCond), NULL);

    pPthreadQ->pQEnd = (pPthreadQ->pQBase) + ((u4MaxMsgs + 1) * u4MsgLen);
    pPthreadQ->pQRead = pPthreadQ->pQBase;
    pPthreadQ->pQWrite = pPthreadQ->pQBase;
    pPthreadQ->u4MsgLen = u4MsgLen;

    pPthreadQ->u4OverFlows = 0;

    return (pPthreadQ);
}

/************************************************************************/
/*  Function Name   : PTHREAD_Delete_MsgQ                               */
/*  Description     : Deletes a Q.                                      */
/*  Input(s)        : QueId     - The QId returned.                     */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
VOID
PTHREAD_Delete_MsgQ (tPthreadQId QId)
{
    tPthreadQ          *pPthreadQ = (tPthreadQ *) QId;
    /* Wait for semaphore to ensure that when the queue is deleted */
    /* no one is reading from or writing into it. Then delete the  */
    /* semaphore, free the queue memory and initialize queue start */
    if (pthread_mutex_lock (pPthreadQ->QueMutex))
    {
        return;
    }
    pthread_cond_destroy (&(pPthreadQ->QueCond));
    pthread_mutex_destroy (pPthreadQ->QueMutex);
    MEM_FREE (pPthreadQ->QueMutex);
    MEM_FREE ((VOID *) pPthreadQ);
}

/************************************************************************/
/*  Function Name   : PTHREAD_Send_MsgQ                                 */
/*  Description     : Sends a message to a Q.                           */
/*  Input(s)        : QueId -    The Q Id.                              */
/*                  : pu1Msg -   Pointer to message to be sent.         */
/*  Output(s)       : None                                              */
/*  Returns         : 0 on SUCCESS and (-1) on FAILURE                  */
/************************************************************************/
INT4
PTHREAD_Send_MsgQ (tPthreadQId QId, UINT1 *pMsg)
{
    tPthreadQ          *pPthreadQ = (tPthreadQ *) QId;
    UINT1              *pWrite, *pRead, *pBase, *pEnd;
    UINT4               u4MsgLen;

    /* Ensure mutual exclusion. Wait and take the mutual exclusion        */
    /* semaphore. A write is possible if the queue is not full. Queue is  */
    /* recognized as full if by writing one more message, write and read  */
    /* pointers become equal. Actually, this means that the queue holds   */
    /* only u4MaxMsgs-1 messages to be safe. When checking the pointers   */
    /* or when advancing the write pointer after the write operation,     */
    /* take care of the wrap-around since this is a circular queue. When  */
    /* the message is written, advance the write pointer by u4MsgLen.     */
    if (pthread_mutex_lock (pPthreadQ->QueMutex))
    {
        return (-1);
    }

    pWrite = pPthreadQ->pQWrite;
    pRead = pPthreadQ->pQRead;
    pBase = pPthreadQ->pQBase;
    pEnd = pPthreadQ->pQEnd;
    u4MsgLen = pPthreadQ->u4MsgLen;

    if (((pWrite + u4MsgLen) == pEnd) && (pRead == pBase))
    {
        pthread_mutex_unlock (pPthreadQ->QueMutex);
        pPthreadQ->u4OverFlows++;
        return (-1);
    }
    if ((pWrite + u4MsgLen) == pRead)
    {
        pthread_mutex_unlock (pPthreadQ->QueMutex);
        pPthreadQ->u4OverFlows++;
        return (-1);
    }
    memcpy (pWrite, pMsg, u4MsgLen);
    (pPthreadQ->pQWrite) += u4MsgLen;

    if ((pPthreadQ->pQWrite) == pEnd)
    {
        (pPthreadQ->pQWrite) = pBase;
    }

    /* unblock anyone waiting to read    */
    pthread_cond_signal (&pPthreadQ->QueCond);

    /* allow others to read/write/delete */
    pthread_mutex_unlock (pPthreadQ->QueMutex);

    return (0);
}

/************************************************************************/
/*  Function Name   : PTHREAD_Receive_MsgQ                              */
/*  Description     : Receives a message from a Q.                      */
/*  Input(s)        : QueId -     The Q Id.                             */
/*                  : i4Timeout - Time to wait in case of WAIT.         */
/*  Output(s)       : pu1Msg -    Pointer to message to be sent.        */
/*  Returns         : 0 on SUCCESS and (-1) on FAILURE                  */
/************************************************************************/
INT4
PTHREAD_Receive_MsgQ (tPthreadQId QId, UINT1 *pMsg, INT4 i4Timeout)
{
    tPthreadQ          *pPthreadQ = (tPthreadQ *) QId;
    struct timespec     ts;
    struct timeval      now;
    UINT4               u4Sec;
    INT4                i4rc;
    UINT4               u4MicroSec;
    DEBUG();
    if (pthread_mutex_lock (pPthreadQ->QueMutex))
    {
        return (-1);
    }
    DEBUG();
    if (i4Timeout == OSIX_NO_WAIT)
    {
        if (pPthreadQ->pQWrite == pPthreadQ->pQRead)
        {
            pthread_mutex_unlock (pPthreadQ->QueMutex);
            return -1;
        }
    }
    else if (i4Timeout == (INT4) OSIX_WAIT)
    {DEBUG();
        while ((pPthreadQ->pQWrite) == (pPthreadQ->pQRead))
        {DEBUG();
            pthread_cond_wait (&pPthreadQ->QueCond, pPthreadQ->QueMutex);
        }
    }
    else
    {
        gettimeofday (&now, NULL);

        u4Sec = 0;
        u4MicroSec = now.tv_usec + i4Timeout;
        if (u4MicroSec > 1000000)
        {
            u4Sec = u4MicroSec / 1000000;
            u4MicroSec = u4MicroSec % 1000000;
        }

        ts.tv_sec = now.tv_sec + u4Sec;
        ts.tv_nsec = u4MicroSec * 1000;

        i4rc = pthread_cond_timedwait (&pPthreadQ->QueCond, pPthreadQ->QueMutex,
                                       &ts);
        if (i4rc == ETIMEDOUT)
        {
            pthread_mutex_unlock (pPthreadQ->QueMutex);
            return OSIX_FAILURE;
        }
    }
    DEBUG();
    /* There is at least 1 message in the queue and we have locked the */
    /* mutual exclusion semaphore so nobody else changes the state.    */
    
    PRINTF("[%s][%d] Recv Str:%p.\n", __FUNCTION__, __LINE__, pMsg);
    
    memcpy (pMsg, pPthreadQ->pQRead, pPthreadQ->u4MsgLen);

    PRINTF("[%s][%d] Recv Str:%p, pRead:%p, msg:%x.\n", __FUNCTION__, __LINE__, pMsg, pPthreadQ->pQRead, *pMsg);

    (pPthreadQ->pQRead) += (pPthreadQ->u4MsgLen);
    DEBUG();
    if ((pPthreadQ->pQRead) == (pPthreadQ->pQEnd))
    {
        (pPthreadQ->pQRead) = (pPthreadQ->pQBase);
    }
    pthread_mutex_unlock (pPthreadQ->QueMutex);
    return (0);
}

/************************************************************************/
/*  Function Name   : PTHREAD_MsgQ_NumMsgs                              */
/*  Description     : Returns No. of messages currently in a Q.         */
/*  Input(s)        : QueId -     The Q Id.                             */
/*  Output(s)       : None                                              */
/*  Returns         : pu4NumberOfMsgs - Contains count upon return.     */
/************************************************************************/
UINT4
PTHREAD_MsgQ_NumMsgs (tPthreadQId QId)
{
    tPthreadQ           PthreadQ = *((tPthreadQ *) QId);
    UINT4               u4Msgs;

    if ((PthreadQ.pQWrite) < (PthreadQ.pQRead))
    {
        u4Msgs =
            (PthreadQ.pQWrite) - (PthreadQ.pQBase) + (PthreadQ.pQEnd) -
            (PthreadQ.pQRead);
        return (u4Msgs / (PthreadQ.u4MsgLen));
    }
    else
    {
        return (((PthreadQ.pQWrite) - (PthreadQ.pQRead)) / (PthreadQ.u4MsgLen));
    }
}


/************************************************************************
 *  Function Name   : FileOpen
 *  Description     : Function to Open a file.
 *  Input           : pu1FileName - Name of file to open.
 *                    i4Mode      - whether r/w/rw.
 *  Returns         : FileDescriptor if successful
 *                    -1             otherwise.
 ************************************************************************/
INT4
FileOpen (const UINT1 *pu1FileName, INT4 i4InMode)
{
    INT4                i4Mode = 0;

    if (i4InMode & OSIX_FILE_CR)
    {
        i4Mode |= O_CREAT;
    }
    if (i4InMode & OSIX_FILE_TR)
    {
        i4Mode |= O_TRUNC;
    }
    if (i4InMode & OSIX_FILE_SY)
    {
        i4Mode |= O_SYNC;
    }
    if (i4InMode & OSIX_FILE_RO)
    {
        i4Mode |= O_RDONLY;
    }
    else if (i4InMode & OSIX_FILE_WO)
    {
        i4Mode |= O_WRONLY;

        if (i4InMode & OSIX_FILE_AP)
        {
            i4Mode |= O_APPEND;
        }
    }
    else if (i4InMode & OSIX_FILE_RW)
    {
        i4Mode |= O_RDWR;

        if (i4InMode & OSIX_FILE_AP)
        {
            i4Mode |= O_APPEND;
        }
    }

    return open ((const CHR1 *) pu1FileName, i4Mode, 0644);
}

/************************************************************************
 *  Function Name   : FileClose
 *  Description     : Function to close a file.
 *  Input           : i4Fd - File Descriptor to be closed.
 *  Returns         : 0 on SUCCESS
 *                    -1   otherwise.
 ************************************************************************/
INT4
FileClose (INT4 i4Fd)
{
    INT4                i4RetVal;
    i4RetVal = close (i4Fd);
    sync ();
    return i4RetVal;
}

/************************************************************************
 *  Function Name   : FileRead
 *  Description     : Function to read a file.
 *  Input           : i4Fd - File Descriptor.
 *                    pBuf - Buffer into which to read
 *                    i4Count - Number of bytes to read
 *  Returns         : Number of bytes read on SUCCESS
 *                    -1   otherwise.
 ************************************************************************/
UINT4
FileRead (INT4 i4Fd, CHR1 * pBuf, UINT4 i4Count)
{
    return read (i4Fd, pBuf, i4Count);
}

/************************************************************************
 *  Function Name   : FileWrite
 *  Description     : Function to write to a file.
 *  Input           : i4Fd - File Descriptor.
 *                    pBuf - Buffer from which to write
 *                    i4Count - Number of bytes to write
 *  Returns         : Number of bytes written on SUCCESS
 *                    -1   otherwise.
 ************************************************************************/
UINT4
FileWrite (INT4 i4Fd, const CHR1 * pBuf, UINT4 i4Count)
{
    return write (i4Fd, pBuf, i4Count);
}

/************************************************************************
 *  Function Name   : FileDelete
 *  Description     : Function to delete a file.
 *  Input           : pu1FileName - Name of file to be deleted.
 *  Returns         : 0 on successful deletion.
 *                    -1   otherwise.
 ************************************************************************/
INT4
FileDelete (const UINT1 *pu1FileName)
{
    INT4                i4RetVal;
    i4RetVal = unlink ((const CHR1 *) pu1FileName);
    sync ();
    return i4RetVal;
}

/************************************************************************
 *  Function Name   : FileSize  
 *  Description     : Function to return file size
 *  Input           : i4Fd - File Descriptor                    
 *  Returns         : Length on successful deletion.
 *                    -1   otherwise.
 ************************************************************************/
INT4
FileSize (INT4 i4Fd)
{
    struct stat         buffer;
    if (fstat (i4Fd, &buffer) < 0)
    {
        return (-1);
    }
    return buffer.st_size;

}


/************************************************************************
 *  Function Name   : CmFileModifyTime  
 *  Description     : Function to return file last update time
 *  Input           : i4Fd - File Descriptor                    
 *  Returns         : time seconds on successful detection.
 *                    -1   otherwise.
 ************************************************************************/
INT4
CmFileModifyTime (INT4 i4Fd)
{
    struct stat         buffer;
    if (fstat (i4Fd, &buffer) < 0)
    {
        return (-1);
    }
    return buffer.st_mtime;

}

/************************************************************************
 *  Function Name   : FileTruncate  
 *  Description     : Function to truncate a file
 *  Input           : i4Fd - File Descriptor                    
 *                    i4Size - File size. Bytes after this size will be
 *                    truncated.                       
 *  Returns         : Length on successful deletion.
 *                    -1   otherwise.
 ************************************************************************/
INT4
FileTruncate (INT4 i4Fd, INT4 i4Size)
{
    return ftruncate (i4Fd, i4Size);
}

/****************************************************************************
 *  Function Name   : FileSeek
 *  Description     : Function to move the file descriptor for the 
 *                    given offset in accordance with the whence.
 *  Input           : i4Fd     - File Descriptor.
 *                    i4Offset - bytes for how many bytes the file descriptor
 *                               needs to be moved.
 *                    i4Whence - it takes one of the following 3 values.
 *                    SEEK_SET - offset is set to offset bytes.
 *                    SEEK_CUR - offset is set to its current location 
 *                               plus offset bytes.
 *                    SEEK_END - offset is set to the size of the file 
 *                               plus offset bytes.
 *  Returns         : final offset location in SUCCESS case, -1 otherwise.
 ****************************************************************************/
INT4
FileSeek (INT4 i4Fd, INT4 i4Offset, INT4 i4Whence)
{
    return (lseek (i4Fd, i4Offset, i4Whence));
}

/************************************************************************
 *  Function Name   : FileStat  
 *  Description     : Function to check if a file is present.
 *  Input           : pc1FileName - Name of the File to be checked. 
 *  Returns         : OSIX_SUCCESS - If the file exists.
 *                    OSIX_FAILURE - If the file does not exist.
 ************************************************************************/
INT4
FileStat (const CHR1 * pc1FileName)
{
    struct stat         FileInfo;
    INT4                i4FileExists = -1;

    if (pc1FileName != NULL)
    {
        /* Attempt to get the file attributes */
        i4FileExists = stat (pc1FileName, &FileInfo);
        if (i4FileExists == 0)
        {
            return OSIX_SUCCESS;
        }
    }

    return OSIX_FAILURE;
}

/************************************************************************
 *  Function Name   : OsixCreateDir 
 *  Description     : This function used to create a directory.
 *  Inputs          : pc1DirName - Name of the Directory.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE
 ************************************************************************/
INT4
OsixCreateDir (const CHR1 * pc1DirName)
{
    if (mkdir (pc1DirName, S_IRWXU) == -1)
    {
        return OSIX_FAILURE;
    }

    return OSIX_SUCCESS;
}



/************************************************************************
 *  Function Name   : FsapShowTask
 *  Description     : Implements the show task part of CLI interface.
 *  Input           : au1Name - TaskName if specified on command line,
 *                              NULL otherwise.
 *  Output          : pu1Result - Buffer containing formatted output.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILRE
 ************************************************************************/
UINT4
FsapShowTask (UINT1 au1TskName[], UINT1 *pu1Result)
{
    UINT4               u4Idx;
    UINT4               u4Pos = 0;
    UINT4               u4NumMatches = 0;
    UINT1              *pu1ResultStart = pu1Result;
    const CHR1         *pc1Heading =
        "  Name     Pending    Prio    Stack       		\r\n"
        "           Events             Size [KB]         \r\n"
        "  --------------------------------------------  \r\n";

    u4Pos = SPRINTF ((CHR1 *) pu1Result, "%s", pc1Heading);
    pu1Result += u4Pos;
    u4Pos = 0;

    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return (OSIX_FAILURE);
    }

    for (u4Idx = 1; u4Idx <= OSIX_MAX_TSKS; u4Idx++)
    {
        if ((gaOsixTsk[u4Idx].u2Free) == OSIX_FALSE)
        {
            if (au1TskName &&
                (UtlStrCaseCmp ((CHR1 *) au1TskName,
                                (CHR1 *) gaOsixTsk[u4Idx].au1Name)))
                continue;

            u4NumMatches++;
            u4Pos +=
                SPRINTF ((CHR1 *) pu1Result + u4Pos,
                         "%6s%9d%10d%10d\r\n",
                         gaOsixTsk[u4Idx].au1Name,
                         gaOsixTsk[u4Idx].u4Events,
                         gaOsixTsk[u4Idx].u4Prio,
                         gaOsixTsk[u4Idx].u4StackSize / 1024);
        }
    }

    if (!u4NumMatches)
    {
        /* SPRINTF adds a trailing \0 ensuring that the header (pc1Heading)
         * does not show up in the output. */
        pu1Result = pu1ResultStart;
        SPRINTF ((CHR1 *) pu1Result, "No such task.\r\n");
    }
    else
    {
        pu1Result = pu1ResultStart;
    }

    pthread_mutex_unlock (&gOsixMutex);
    return (OSIX_SUCCESS);
}


/************************************************************************
 *  Function Name   : CmFsapSetTaskPRI
 *  Description     : Implements change task PRI.
 *  Input           : None
 *  Output          : None
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE
 ************************************************************************/
INT4
CmFsapSetTaskPRI (UINT4  u4Id, UINT4  u4Pri)
{
	INT4 i4Policy =0;
	INT4 i4Ret, i4Result;
	struct sched_param param;
	do{
		i4Result = OSIX_FAILURE;
		if ((gaOsixTsk[u4Id].u2Free) == OSIX_TRUE)
		{
            PRINTF("Task is not exist.\r\n");
			break;
		}
	   	i4Ret = pthread_getschedparam(gaOsixTsk[u4Id].ThrId, &i4Policy, &param);
		if (i4Ret != 0)
		{
	       PRINTF("FAILED while get\r\n");
		   break;
		}

		param.sched_priority = (INT4)u4Pri;
		i4Ret = pthread_setschedparam(gaOsixTsk[u4Id].ThrId, i4Policy, &param);
		if (i4Ret != 0)
		{
           PRINTF("FAILED while set\r\n");
	       break;
		}
        gaOsixTsk[u4Id].u4Prio = u4Pri;
		i4Result = OSIX_SUCCESS;
		
	}while(0);

	return i4Result;
}

/************************************************************************
 *  Function Name   : CmFsapSetTaskPRIByName
 *  Description     : Implements change task PRI by Name.
 *  Input           : au1Name Task name
 *                    u4Pri   ISS task Priority.
 *  Output          : None
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE
 ************************************************************************/
INT4
CmFsapSetTaskPRIByName (UINT1 *pu1Name, UINT4  u4Pri)
{
	UINT4  u4Id = 0;
	UINT4  u4OSPrio = 0;
	VOID * pId = NULL;
	pId    = &u4Id;
	
	UINT1	au1Name[OSIX_NAME_LEN + 4];


	if(pu1Name == NULL)
		return OSIX_FAILURE;

    MEMSET ((UINT1 *) au1Name, 0, (OSIX_NAME_LEN + 4));
    MEMCPY (au1Name, pu1Name, OSIX_NAME_LEN);

	if (OsixRscFind (au1Name, OSIX_TSK, &pId) != OSIX_SUCCESS)
    {
        PRINTF("Can not find the task name=<%s>\n",au1Name);
        return OSIX_FAILURE;
    }

	CM_ISS_PRI_TO_OS_PRI(u4Pri, u4OSPrio);

	return CmFsapSetTaskPRI(u4Id, u4OSPrio);
}
/************************************************************************
 *  Function Name   : CmFsapDumpTask
 *  Description     : Implements the dump task.
 *  Input           : None
 *  Output          : None
 *  Returns         : OSIX_SUCCESS/OSIX_FAILRE
 ************************************************************************/
INT4 CmFsapDumpTask (UINT4  u4Id)
{
    UINT4               u4Idx;
    UINT4               u4NumMatches = 0;
	
    const CHR1         *pc1Heading =
        "  ID    TrId     Name     Events    Prio    Stack[KB]   \r\n"
        "  ------------------------------------------------------\r\n";

	PRINTF("%s", pc1Heading);

    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return (OSIX_FAILURE);
    }


    for (u4Idx = 1; u4Idx <= OSIX_MAX_TSKS; u4Idx++)
    {
    	
        if ((gaOsixTsk[u4Idx].u2Free) == OSIX_FALSE)
        {
			if(u4Id != 0)
				u4Idx = u4Id;
			
            u4NumMatches++;

            PRINTF ("%4u%4d%6s%9d%10d%10d\r\n",u4Idx,
            			 (INT4)gaOsixTsk[u4Idx].ThrId,
                         gaOsixTsk[u4Idx].au1Name,
                         gaOsixTsk[u4Idx].u4Events,
                         gaOsixTsk[u4Idx].u4Prio,
                         gaOsixTsk[u4Idx].u4StackSize / 1024);
			
			if(u4Id != 0)
				break;
        }
    }

	PRINTF("Tatal %u\r\n",u4NumMatches);
    pthread_mutex_unlock (&gOsixMutex);
    return (OSIX_SUCCESS);
}
/************************************************************************
 *  Function Name   : FsapShowQueData
 *  Description     : Called from FsapShowQue to display the data present
 *                    in a given (pTmoQ) queue.
 *  Input           : pTmoQ - Pointer to TMO's que data structure.
 *  Output          : pu1Result - Buffer containing formatted output.
 *  Returns         : No. of bytes written to output buffer.
 ************************************************************************/
UINT4
FsapShowQueData (tPthreadQ * pPthreadQ, UINT1 *pu1Result)
{
    UINT4               u4Pos = 0;
    UINT4               u4Queued;
    UINT4               u4Count;
    UINT4               u4MsgLen;
    UINT4               u4Byte;
    UINT1              *pMsg;
    UINT1              *pDatum;

    u4MsgLen = pPthreadQ->u4MsgLen;
    u4Queued = (pPthreadQ->pQWrite - pPthreadQ->pQRead) / u4MsgLen;
    pMsg = pPthreadQ->pQRead;
    pDatum = MEM_MALLOC (u4MsgLen, UINT1);

    if (pDatum == NULL)
    {
        return u4Pos;
    }

    u4Pos += SPRINTF ((CHR1 *) pu1Result + u4Pos, "\r\nMessages in Q:\r\n");

    for (u4Count = 0; u4Count < u4Queued; u4Count++)
    {
        MEMCPY (pDatum, pMsg, u4MsgLen);

        /* If msglength is 4, assume it is a pointer and display it
         * else do a byte by byte dump of the message.
         */
        if (u4MsgLen == 4)
        {
            u4Pos +=
                SPRINTF ((CHR1 *) pu1Result + u4Pos, " 0x%lx\r\n",
                         (FS_ULONG) pMsg);
        }
        else
        {
            for (u4Byte = 0; u4Byte < u4MsgLen; u4Byte++)
            {
                u4Pos +=
                    SPRINTF ((CHR1 *) pu1Result + u4Pos, " %x",
                             *(CHR1 *) (pMsg + u4Byte));
            }
            u4Pos += SPRINTF ((CHR1 *) pu1Result + u4Pos, "\r\n");
        }
        pMsg += u4MsgLen;
    }

    MEM_FREE (pDatum);

    return (u4Pos);
}

/************************************************************************
 *  Function Name   : FsapShowQue
 *  Description     : Implements the show que part of CLI interface.
 *  Input           : au1Name - QueueName if specified on command line,
 *                              NULL otherwise.
 *  Output          : pu1Result - Buffer containing formatted output.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILRE
 ************************************************************************/
UINT4
FsapShowQue (UINT1 au1QName[], UINT1 *pu1Result)
{
    tOsixQId            QueId;
    tPthreadQ          *pPthreadQ;
    UINT4               u4Idx;
    UINT4               u4Queued;
    UINT4               u4MsgLen;
    UINT4               u4Iter = 0;
    UINT4               u4Pos = 0;
    UINT4               u4TmpPos = 0;
    UINT4               u4NumMatches = 0;
    UINT4               u4SpecificQ = (au1QName ? 1 : 0);
    UINT1              *pu1ResultStart = pu1Result;
    const CHR1         *pc1Heading =
        "   Name      ID        Q Depth  MaxMsgLen    Queued   OverFlows\r\n"
        "  --------------------------------------------------------------\r\n";

    u4Pos = SPRINTF ((CHR1 *) pu1Result, "%s", pc1Heading);
    pu1Result += u4Pos;
    u4Pos = 0;

    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return (OSIX_FAILURE);
    }

    /*
     * If the CLI cmd is "show que" we iterate over all queues once.
     * If the CLI cmd is "show que qname" first iteration of do-while
     * composes the tabular form, the second iteration, then writes
     * the messages in each que. Since the code is similar we use
     * a while loop around the foo loop and distinguish the iterations
     * by means of the two variables u4Iter and u4SpecificQ.
     */
    do
    {
        u4Iter++;
        for (u4Idx = 1; u4Idx <= OSIX_MAX_QUES; u4Idx++)
        {
            if ((gaOsixQue[u4Idx].u2Free) == OSIX_FALSE)
            {
                if (au1QName &&
                    (UtlStrCaseCmp ((CHR1 *) au1QName,
                                    (CHR1 *) gaOsixQue[u4Idx].au1Name)))
                    continue;

                u4NumMatches++;

                QueId = &gaOsixQue[u4Idx];
                pPthreadQ = (tPthreadQ *) gaOsixQue[u4Idx].pRscId;

                u4MsgLen = pPthreadQ->u4MsgLen;
                u4Queued = (pPthreadQ->pQWrite - pPthreadQ->pQRead) / u4MsgLen;
                if (u4Iter == 1)
                {
                    u4Pos += SPRINTF ((CHR1 *) pu1Result + u4Pos,
                                      "%6s %10lx %10ld %10d %10d %10d\r\n",
                                      gaOsixQue[u4Idx].au1Name,
                                      (FS_ULONG) QueId,
                                      (FS_ULONG) ((pPthreadQ->pQEnd -
                                                   pPthreadQ->pQBase - 1) /
                                                  (pPthreadQ->u4MsgLen)),
                                      u4MsgLen, u4Queued,
                                      pPthreadQ->u4OverFlows);
                }
                else
                {
                    if (u4SpecificQ)
                    {
                        u4TmpPos =
                            FsapShowQueData (pPthreadQ, pu1Result + u4Pos);
                        if (u4TmpPos == 0)
                        {
                            return OSIX_FAILURE;
                        }
                        u4Pos += u4TmpPos;
                        break;
                    }
                }
            }
        }
    }
    while (u4Iter < 2);

    if (!u4NumMatches)
    {
        /* SPRINTF adds a trailing \0 ensuring that the header (pc1Heading)
         * does not show up in the output. */
        pu1Result = pu1ResultStart;
        SPRINTF ((CHR1 *) pu1Result, "No such queue.\r\n");
    }
    else
    {
        pu1Result = pu1ResultStart;
    }
    pthread_mutex_unlock (&gOsixMutex);

    return (0);
}


/************************************************************************
 *  Function Name   : CmFsapShowMsgQue
 *  Description     : Implements the show que part of CLI interface.
 *  Input           : au1Name - QueueName if specified on command line,
 *                              NULL otherwise.
 *  Output          : N/A
 *  Returns         : OSIX_SUCCESS/OSIX_FAILRE
 ************************************************************************/
UINT4
CmFsapShowMsgQue (UINT1 au1QName[])
{
    tOsixQId            QueId;
    tPthreadQ          *pPthreadQ;
    UINT4               u4Idx;
    UINT4               u4Queued;
    UINT4               u4MsgLen;
    UINT4               u4NumMatches = 0;
    UINT4               u4UsedQueNumb = 0, u4FreeQueNumb = 0;
        
    const CHR1         *pc1Heading =
        "   Name      ID        Q Depth  MaxMsgLen    Queued   OverFlows\r\n"
        "  --------------------------------------------------------------\r\n";

    PRINTF ("%s", pc1Heading);
    
    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return (OSIX_FAILURE);
    }


    for (u4Idx = 1; u4Idx <= OSIX_MAX_QUES; u4Idx++)
    {
        if ((gaOsixQue[u4Idx].u2Free) == OSIX_FALSE)
        {
            if (au1QName &&
                (UtlStrCaseCmp ((CHR1 *) au1QName,
                                (CHR1 *) gaOsixQue[u4Idx].au1Name)))
                continue;

            u4NumMatches++;

            QueId = &gaOsixQue[u4Idx];
            pPthreadQ = (tPthreadQ *) gaOsixQue[u4Idx].pRscId;

            u4MsgLen = pPthreadQ->u4MsgLen;
            u4Queued = (pPthreadQ->pQWrite - pPthreadQ->pQRead) / u4MsgLen;

            PRINTF ("%6s %10lx %10ld %10d %10d %10d\r\n",
                    gaOsixQue[u4Idx].au1Name,
                    (FS_ULONG) QueId,
                    (FS_ULONG) ((pPthreadQ->pQEnd -
                                pPthreadQ->pQBase - 1) /
                                (pPthreadQ->u4MsgLen)),
                    u4MsgLen, u4Queued,
                    pPthreadQ->u4OverFlows);
        }
    }

    for (u4Idx = 1; u4Idx <= OSIX_MAX_QUES; u4Idx++)
    {
        if ((gaOsixQue[u4Idx].u2Free) == OSIX_FALSE)
        {
            u4UsedQueNumb++;
        }
        else
        {
            u4FreeQueNumb++;
        }
    }
    
    PRINTF("[%s][%d]UsedQueNumb:%d, FreeQueNumb:%d,Total:%d.\r\n", __FUNCTION__, __LINE__, u4UsedQueNumb, u4FreeQueNumb, (u4UsedQueNumb+u4FreeQueNumb));

    if (!u4NumMatches)
    {
        /* SPRINTF adds a trailing \0 ensuring that the header (pc1Heading)
         * does not show up in the output. */
        PRINTF ("No such queue.\r\n");
    }

    pthread_mutex_unlock (&gOsixMutex);

    return (0);
}


/************************************************************************
 *  Function Name   : FsapShowSem
 *  Description     : Implements the show sem part of CLI interface.
 *  Input           : au1Name - SemName if specified on command line,
 *                              NULL otherwise.
 *  Output          : pu1Result - Buffer containing formatted output.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILRE
 ************************************************************************/
UINT4
FsapShowSem (UINT1 au1SemName[], UINT1 *pu1Result, UINT4 *pu4NextIdx)
{
    au1SemName = au1SemName;    /* Unused variable */
    pu4NextIdx = pu4NextIdx;    /* Unused variable */
    SPRINTF ((CHR1 *) pu1Result, "Not supported\r\n");
    return (OSIX_FAILURE);
}

/************************************************************************
 *  Function Name   : FsapTrace
 *  Description     : Implements the trace part of CLI interface.
 *  Input           : u4Flag =1 for 'trace' command, =0 for the 'no trace' cmd
 *                    u4Value - the value of the particular trace that is
 *                              being set/unset.
 *  Output          : pu4TrcLvl - Returns the current/new trace level.
 *  Returns         : None.
 ************************************************************************/
void
FsapTrace (UINT4 u4Flag, UINT4 u4Value, UINT4 *pu4TrcLvl)
{
    if (u4Value == 0)
    {
        *pu4TrcLvl = OSIX_TRC_FLAG;
        return;
    }

    if (u4Flag)
    {
        OSIX_TRC_FLAG |= u4Value;
    }
    else
    {
        OSIX_TRC_FLAG &= ~u4Value;
    }
    *pu4TrcLvl = OSIX_TRC_FLAG;

    return;
}

/************************************************************************/
/*  Function Name   : OsixGetTps                                        */
/*  Description     : Returns the no of system ticks in a second        */
/*  Input(s)        : None                                              */
/*  Output(s)       : None                                              */
/*  Returns         : Number of system ticks in a second                */
/************************************************************************/
UINT4
OsixGetTps ()
{
    return (sysconf (_SC_CLK_TCK));
}

/************************************************************************/
/*  Function Name   : OsixSetLocalTime                                  */
/*  Description     : Obtains the system time and sets it in FSAP.      */
/*  Input(s)        : None                                              */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
OsixSetLocalTime (void)
{
    time_t              t;
    struct tm          *tm;

    time (&t);
    tm = localtime (&t);
    if (tm == NULL)
    {
        return (OSIX_FAILURE);
    }
    tm->tm_year += (1900);
    /*UtlSetTime ((tUtlTm *) tm);*/
    /* CAMEOTAG:Delete by Jiane on 2012.3.28
    * function TmrSetSysTime has been called by the above function UtlSetTime;
    * so delete it.
    */
    /*TmrSetSysTime ((tUtlTm *) tm);*/
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : FsapShowTCB                                       */
/*  Description     : Used to Give the Task Info to the Calling Function*/
/*  Input(s)        : None                                              */
/*  Output(s)       : pu1Result - Output Buffer                         */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT4
FsapShowTCB (UINT1 *pu1Result)
{
    UINT4               u4Idx;
    UINT4               u4Pos = 0;

    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        SPRINTF ((CHR1 *) pu1Result, "Cannot Access the TCB \n");
        return (OSIX_FAILURE);
    }
    for (u4Idx = 1; (u4Idx <= OSIX_MAX_TSKS) &&
         (gaOsixTsk[u4Idx].u2Free == OSIX_FALSE); u4Idx++)
    {
        u4Pos += SPRINTF ((CHR1 *) pu1Result + u4Pos,
                          "Task Name : %s Process Id : %d \n",
                          gaOsixTsk[u4Idx].au1Name, gaOsixTsk[u4Idx].u4Pid);
    }
    pthread_mutex_unlock (&gOsixMutex);
    return (OSIX_SUCCESS);
}



