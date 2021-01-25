/*
 * Copyright (C) 2006 Aricent Inc . All Rights Reserved
 *
 * $Id: osix.h,v 1.12 2012/10/04 09:36:12 siva Exp $
 *
 * Description: This is the exported file for fsap 3000.
 *              It contains exported defines and FSAP APIs.
 */
#ifndef _OSIX_H_
#define _OSIX_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
/*#include "fsap.h"*/

enum {
    OSIX_TSK_TRC = 0x01,
    OSIX_QUE_TRC = 0x02,
    OSIX_SEM_TRC = 0x04,
    OSIX_MEM_TRC = 0x08,
    OSIX_BUF_TRC = 0x10,
    OSIX_TMR_TRC = 0x20,
    OSIX_EVT_TRC = 0x40,
    OSIX_ALL_TRC = ~0L
};

#define   OSIX_MAX_SEMS                     11000
#define  OSIX_MAX_TSKS          100

/* If the default stack size is less than 16000, pthread
 * library will allocate stack size of 2044000 */

#define OSIX_PTHREAD_MIN_STACK_SIZE         16000 
#define OSIX_PTHREAD_DEF_STACK_SIZE         2094000


/*** Configurable parameters.              ***/
#define  OSIX_NAME_LEN           4
#define  OSIX_DEF_MSG_LEN        4
#define  OSIX_MAX_Q_MSG_LEN      sizeof(void *)
#define  OSIX_TPS          1000000
#define  OSIX_STUPS              1
#define  OSIX_MAX_QUES         200
#define  OSIX_FILE_LOG           0        /* 0: disable file logging
                                           * 1: enable file logging
                                           */
#define OSIX_LOG_METHOD    CONSOLE_LOG 

/*** Shared between tmo.c and wrap.c       ***/
#define  OSIX_TSK                0
#define  OSIX_SEM                1
#define  OSIX_QUE                2
#define  OSIX_TRUE               1
#define  OSIX_FALSE              0
#define  OSIX_RSC_INV         NULL

/*** OS related constants used by fsap     ***/

/* LOW_PRIO  - Priority of lowest priority task.
 * HIGH_PRIO - Priority of highest priority task.
 */
#define  FSAP_LOW_PRIO         255  /* 31 but set as 255 for bward compat. */
#define  FSAP_HIGH_PRIO          0

/* CMAEOTAG: add by guofeng 2014/2/18, dynamic get schedule priority range */
#define OS_LOW_PRIO				gi4MinPrio
#define OS_HIGH_PRIO			gi4MaxPrio

/*CAMEOTAG: By Guixue 2014-01-18
DES: Convert the ISS PRI to OS PRI.*/

#define CM_ISS_PRI_TO_OS_PRI(issPri, osPri) \
	issPri = issPri & 0x0000ffff; \
    /* Remap the task priority to Vxworks's range of values. */ \
    osPri = OS_LOW_PRIO + (((((INT4) (issPri) - FSAP_LOW_PRIO) * \
                                (OS_HIGH_PRIO -OS_LOW_PRIO)) /  \
                                (FSAP_HIGH_PRIO - FSAP_LOW_PRIO)));

/* Scheduling Algorithm */
#define OSIX_SCHED_RR             (1 << 16)
#define OSIX_SCHED_FIFO           (1 << 17)
#define OSIX_SCHED_OTHER          (1 << 18)
 
    
/*** Wait flags used in OsixEvtRecv()      ***/
#undef   OSIX_WAIT
#undef   OSIX_NO_WAIT
#define  OSIX_WAIT               (UINT4) (~0UL)
#define  OSIX_NO_WAIT            0

/***          FILE modes                   ***/
#define OSIX_FILE_RO          0x01
#define OSIX_FILE_WO          0x02
#define OSIX_FILE_RW          0x04
#define OSIX_FILE_CR          0x08
#define OSIX_FILE_AP          0x10
#define OSIX_FILE_SY          0x20
#define OSIX_FILE_TR          0x40
    
/***          Memory related constants     ***/
#define OSIX_MEM_INFO         10   
#define OSIX_MEM_TOTAL        "MemTotal"
#define OSIX_MEM_FREE         "MemFree"

/***          CPU related constants        ***/
#define OSIX_CPU_USAGE_COUNT  4
#define OSIX_MAX_USAGE_COUNT  4
#define OSIX_CPU_INF0         "cpu"
#define OSIX_CPU_WAIT_TIME    300
    
/***          Time Units                   ***/
#define OSIX_SECONDS          1
#define OSIX_CENTI_SECONDS    2
#define OSIX_MILLI_SECONDS    3
#define OSIX_MICRO_SECONDS    4
#define OSIX_NANO_SECONDS     5

/***         BackTrace related macros     ****/
#define  OSIX_MAX_STACK_FRAMES   30
#define  OSIX_STACK_TRACE_FILE  "stacktrace.txt"
#define  OSIX_MAX_STACK_TRACE_FILE_SIZE  3000
#define  OSIX_STACK_TRACE_MAX_FILE_NAME_LEN 20
/* CAMEOTAG: add by linyu on 2012-09-11, replace system() function 
	this function's actual parameter must be a const or static value to prevent it be cleared or changed 
	before transmit to the new thread*/
#define SYSTEM	CmOsixSystem


#define ARG_LIST(a) a

typedef struct OsixRscSemStruct
{
    sem_t               SemId;
    UINT2               u2Free;
    UINT2               u2Filler;
    UINT1               au1Name[OSIX_NAME_LEN + 4];
    tOsixTaskId         TskId;
}tOsixSem;

/*** Prototype of an Entry Point Function. ***/
typedef VOID (*OsixTskEntry)(INT1 *);


/***           FSAP APIs                   ***/
UINT4      OsixInitialize   ARG_LIST ((VOID));
UINT4      OsixShutDown     ARG_LIST ((VOID));
UINT4      OsixTskCrt       ARG_LIST ((UINT1[], UINT4, UINT4, VOID(*)(INT1 *),
INT1*, tOsixTaskId*));
VOID       OsixTskDel       ARG_LIST ((tOsixTaskId));
UINT4      OsixTskDelay     ARG_LIST ((UINT4));
UINT4      CmOsixTskUsDelay     ARG_LIST ((UINT4));
UINT4      OsixDelay        ARG_LIST ((UINT4, INT4));
UINT4      OsixTskIdSelf    ARG_LIST ((tOsixTaskId*));
UINT4      CmOsixTskIdSelfWithLock    ARG_LIST ((tOsixTaskId*));
UINT4      OsixEvtSend      ARG_LIST ((tOsixTaskId, UINT4));
UINT4      OsixEvtRecv      ARG_LIST ((tOsixTaskId, UINT4, UINT4, UINT4*));
UINT4
OsixCreateSem (CONST UINT1 au1SemName[4], UINT4 u4InitialCount,
               UINT4 u4Flags, tOsixSemId * pSemId);
UINT4      OsixSemCrt       ARG_LIST ((UINT1[], tOsixSemId*));
VOID       OsixSemDel       ARG_LIST ((tOsixSemId));
UINT4      OsixSemGive      ARG_LIST ((tOsixSemId));
UINT4      OsixSemTake      ARG_LIST ((tOsixSemId));
UINT4      OsixQueCrt       ARG_LIST ((UINT1[], UINT4, UINT4, tOsixQId*));
VOID       OsixQueDel       ARG_LIST ((tOsixQId));
UINT4      OsixQueSend      ARG_LIST ((tOsixQId, UINT1*, UINT4));
UINT4      OsixQueRecv      ARG_LIST ((tOsixQId, UINT1*, UINT4, INT4));
UINT4      OsixQueNumMsg    ARG_LIST ((tOsixQId, UINT4*));
UINT4      OsixGetSysUpTime ARG_LIST ((VOID));
UINT4      CmOsixGetMicroTimeTick ARG_LIST ((VOID));

UINT4      OsixGetTps       ARG_LIST ((VOID));

/*** API shared b/w tmo.c and wrap.c  
     This is not an exported API           ***/
UINT4      OsixRscFind      ARG_LIST((UINT1[], UINT4, VOID **));

/* CAMEOTAG: add by linyu on 2012-09-11, replace system() function */
INT4 CmOsixSystem ARG_LIST((INT1 *));

INT4 CmOsixAccess(const CHR1 *pathname, INT4 mode);
INT4 CmOsixTruncate(const CHR1 *path, INT4 length);

INT4       FileOpen        (const UINT1 *pu1FileName, INT4 i4Mode);
INT4       FileClose       (INT4 i4Fd);
UINT4      FileRead        (INT4 i4Fd, CHR1 *pBuf, UINT4 i4Count);
UINT4      FileWrite       (INT4 i4Fd, const CHR1 *pBuf, UINT4 i4Count);
INT4       FileDelete      (const UINT1 *pu1FileName);
INT4       FileSize        (INT4 i4Fd);
INT4       CmFileModifyTime        (INT4 i4Fd);
INT4       FileTruncate    (INT4 i4Fd, INT4 i4Size);
INT4       FileSeek        (INT4 i4Fd, INT4 i4Offset, INT4 i4Whence);
INT4       FileStat        (const CHR1 * pc1FileName);
INT4       OsixCreateDir   (const CHR1 * pc1DirName);
UINT4      OsixSetLocalTime (void);
UINT4      FsapShowTCB     (UINT1 *pu1Result);
VOID       CmOsixSemDebug(VOID);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* _OSIX_H_ */
