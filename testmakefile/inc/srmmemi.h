/*
 * Copyright (C) 2006 Aricent Inc . All Rights Reserved
 *
 * $Id: srmmemi.h,v 1.4 2011/08/04 10:28:22 siva Exp $
 *
 * Description: SRM Mem moudule's internal file.
 *
 */

#ifndef SRM_MEMI_H
#define SRM_MEMI_H

/************************************************************************
*                                                                       *
*                      Defines and Typedefs                             *
*                                                                       *
*************************************************************************/

typedef struct CRU_SLL_NODE {
        UINT1  *pNext;
} tCRU_SLL_NODE;

typedef struct CRU_SLL {
        UINT1  *pu1Base;
        UINT1  *pu1Head;
} tCRU_SLL;

typedef struct MEM_FREE_POOL_REC{
        UINT4  u4Size;
        /** Size of Each Block in the Buffer Pool */

        UINT4  u4UnitsCount;
        /** Number of Units of `u4Size` Blocks   */

        UINT4  u4FreeUnitsCount;
        /** Number of Units Presently Free        */
        
        UINT4 u4MemType;
        /** BitMap to indicate whether the alloc is from MemPool or Heap */

#if DEBUG_MEM == FSAP_ON
#define MEM_DBG_INFO_SZ  (52)
#define MEM_DBG_INFO_LEN (50)
        UINT4  u4AllocCount;
        /** Statistics About Number of Allocs on this Memory Pool  */

        UINT4  u4ReleaseCount;
        /** Statistics About Number of Frees on this Memory Pool */

        UINT4  u4AllocFailCount;
        /** Statistics on Number of Allocs Failed on this Pool */

        UINT4  u4HeapAllocFailCount;
        /** Statistics on Number of Allocs Failed on the Heap */

        UINT4  u4ReleaseFailCount;
        /** Statistics on Number of Frees Failed on this Pool */

        UINT4  u4PeakUsageCount;
        UINT1  au1OwnerTask[OSIX_NAME_LEN + 4];
        UINT4  u4LineNo;
		/* CAMEOTAG: delete by linyu on 2014-01-22, func and line number decide the location */
        /* UINT1  au1File [MEM_DBG_INFO_LEN]; */
        /* UINT1  au1Func [MEM_DBG_INFO_LEN]; */
		const CHR1 *pu1Func;
        /* UINT1  au1Variable [MEM_DBG_INFO_LEN]; */
#endif

        UINT1  *pu1ActualBase;
        /** The Value returned by MALLOC/CALLOC in MemCreateMemPool */

        UINT1  *pu1StartAddr;
        /** Starting Address of the Buffer Pool (aligned to 4 byte boundary)*/

        UINT1  *pu1EndAddr;
        /** Ending Address of the Buffer Pool*/

        tCRU_SLL  BufList;
        /** Singly Linked Circular List associated with the Buffer Pool */

        tOsixSemId SemId;

} tMemFreePoolRec;

#if DEBUG_MEM == FSAP_ON
typedef struct {
	/* CAMEOTAG: delete by linyu on 2014-01-22, func and line number decide the location */
    /* UINT1  au1File [MEM_DBG_INFO_LEN]; */
    /* UINT1  au1Func [MEM_DBG_INFO_LEN];*/
	const CHR1  *pu1Func;
    /*UINT1  au1Variable [MEM_DBG_INFO_LEN];*/
	/* CAMEOTAG: add debug info for each node will cost too much memroy */
#ifndef CAMEO_MEMTRACE_MINMEM_WANTED
    UINT4  u4LineNo;
#endif
} tMemBlkDbg;

typedef struct {
#ifndef CAMEO_MEMTRACE_MINMEM_WANTED
    UINT4 u4Signature;
    UINT4 u4SystemData;
    UINT4 u4TimeStamp;
#endif
    tMemBlkDbg BlkDbg;
} tMemDebugInfo;

#define MEM_DEBUG_SIGNATURE_VAL 0x7db12997
#define MEM_THRESHOLD_VAL       0.01

#endif
/************************************************************************
*                                                                       *
*                          Macro                                        *
*                                                                       *
*************************************************************************/
#if DEBUG_MEM == FSAP_ON
enum {mem_debug_block_free = 0, mem_debug_block_allocated};

#define MEM_DEBUG_SYSTEM_DATA(pBlock)\
(((tMemDebugInfo *)(VOID *)((UINT1 *)(pBlock)+pMemFreePoolRecList[u2QueId].u4Size))->u4SystemData)

#define MEM_DEBUG_TIMESTAMP(pBlock)\
(((tMemDebugInfo *)(VOID *)((UINT1 *)(pBlock)+pMemFreePoolRecList[u2QueId].u4Size))->u4TimeStamp)

#define MEM_DEBUG_SIGNATURE(pBlock)\
(((tMemDebugInfo *)(VOID *)((UINT1 *)(pBlock)+pMemFreePoolRecList[u2QueId].u4Size))->u4Signature)

/* CAMEOTAG: delete by linyu on 2014-01-22, func and line number decide the location */
/*#define MEM_DEBUG_FILE(pBlock)\
(((tMemDebugInfo *)(VOID *)((UINT1 *)(pBlock)+pMemFreePoolRecList[u2QueId].u4Size))->BlkDbg.au1File)*/

#define MEM_DEBUG_FUNC(pBlock)\
(((tMemDebugInfo *)(VOID *)((UINT1 *)(pBlock)+pMemFreePoolRecList[u2QueId].u4Size))->BlkDbg.pu1Func)

#ifndef CAMEO_MEMTRACE_MINMEM_WANTED
#define MEM_DEBUG_LINE(pBlock)\
(((tMemDebugInfo *)(VOID *)((UINT1 *)(pBlock)+pMemFreePoolRecList[u2QueId].u4Size))->BlkDbg.u4LineNo)
#endif /* CAMEO_MEMTRACE_MINMEM_WANTED */
#endif /* DEBUG_MEM == FSAP_ON */

#if DEBUG_MEM == FSAP_ON
/* Size of debug info. - 4 byte normalized. */
#define MEM_DEBUG_INFO_SIZE ((sizeof(tMemDebugInfo) + MEM_ALIGN_BYTE) & MEM_ALIGN)
#else
#define MEM_DEBUG_INFO_SIZE 0
#endif

#define MEM_POOL_GET_BLOCK_SIZE(pMemFreePoolRec) (pMemFreePoolRec->u4Size)
#define MEM_POOL_GET_NUM_BLOCKS(pMemFreePoolRec) (pMemFreePoolRec->u4UnitsCount)
#define MEM_POOL_GET_START_ADDR(pMemFreePoolRec) (pMemFreePoolRec->pu1StartAddr)
#define MEM_POOL_GET_END_ADDR(pMemFreePoolRec)   (pMemFreePoolRec->pu1EndAddr)

#define MEM_ENTER_CS(Id) OsixSemTake(pMemFreePoolRecList[Id].SemId)

#define MEM_LEAVE_CS(Id) OsixSemGive(pMemFreePoolRecList[Id].SemId)



/* The mutex to be used by Mem module. */
#define  MEM_MUTEX_NAME  (const UINT1 *)"MEMU"

#define MEM_DBG_FLAG     gu4MemDbg
#define MEM_MODNAME      "MEM"

#if DEBUG_MEM == FSAP_ON
#define MEM_DBG(x)  UtlTrcLog x
#else
#define MEM_DBG(x)
#endif

#define MEM_PRNT(pi1Msg) printf ("%s", pi1Msg); /*UtlTrcPrint*/
#define MEM_ALIGN  ((~0UL) & ~(FS_ULONG)(sizeof(FS_ULONG) - 1))
#define MEM_ALIGN_BYTE (sizeof(FS_ULONG) - 1)

extern UINT4 MemIsValidMemoryType(UINT4);
extern INT4 MemParseSpec (UINT1 *s, UINT1 *pu1Sign, UINT4 *pu4Duration,
                          UINT1 *pu1HMS);
extern UINT4 MemMatch (UINT4 u4TimeStamp, UINT1 u1Sign, UINT4 u4Duration,
                       UINT1 u1HMS);
#endif
