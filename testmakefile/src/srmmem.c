/*
 * Copyright (C) 2006 Aricent Inc . All Rights Reserved
 *
 * $Id: srmmem.c,v 1.9 2012/12/20 11:14:59 siva Exp $
 *
 * Description: SRM Mem Module.
 *
 */

/************************************************************************
*                                                                       *
*                          Header  Files                                *
*                                                                       *
*************************************************************************/
/*#include "osxinc.h"
#include "osxstd.h"
#include "srmbuf.h"
#include "osxsys.h"
#include "osxprot.h"
#include "srmmem.h"
#include "srmmemi.h"
#include "utltrc.h"
#include "utlmacro.h"
#include "memport.h"
#include "buftdfs.h"
#include "bufglob.h"*/

#include "osxinc.h"
#include "osix.h"
#include "osxstd.h"
#include "srmmem.h"
#include "srmmemi.h"


/*extern tMemPoolId SLIMemPoolIds[ ];
extern tMemPoolId TCPMemPoolIds[ ];*/

/*extern VOID         mmi_printf (const char *fmt, ...);*/

#define mmi_printf printf

/*extern tMemPoolId gTcbTblPoolId;*/
#define   PARAM_UNUSED(x)   ((VOID)x)
/************************************************************************
*                                                                       *
*                          MACROS                                       *
*                                                                       *
*************************************************************************/

/************************************************************************
*                                                                       *
*               Internal Function Prototypes                            *
*                                                                       *
*************************************************************************/
PRIVATE INT4        MemGetFreePoolId (void);
PRIVATE UINT4       MemPoolInitVar (tMemPoolCfg * pMemPoolCfg);
PRIVATE UINT4       MemIsValidBlock (tMemPoolId PoolId, UINT1 *pu1Block);
PRIVATE UINT4       MemPoolInitializeFreePoolList (void);
PRIVATE UINT4       MemPoolValidateCreateParam (UINT4 u4BlockSize,
                                                UINT4 u4NumberOfBlocks,
                                                UINT4 u4TypeOfMemory);
/************************************************************************
*                                                                       *
*               Internal Static Global Variables                        *
*                                                                       *
*************************************************************************/
static UINT4        gu4MemPoolInitialized = 0;
tMemPoolCfg         gtMemPoolCfg;
static tMemTypeCfg *gptMemTypeArray = NULL;
tMemFreePoolRec    *pMemFreePoolRecList = NULL;
static tOsixSemId   MemMutex;    /* Mutex used within Mem */
#if (DEBUG_MEM == FSAP_ON)
UINT4               gu4MemDbg = MEM_DBG_MAJOR | MEM_DBG_CRITICAL |
    MEM_DBG_FATAL | MEM_DBG_ALWAYS;
#endif

/************************************************************************
*                                                                       *
*               Function  Definitions                                   *
*                                                                       *
*************************************************************************/

/************************************************************************/
/*  Function Name   : MemInitMemPool                                    */
/*  Description     : Initialises the Memory Pool Manager with details  */
/*                    about the number of maximum number of memory pools*/
/*                    type of memory supported and the chunk details etc*/
/*  Input(s)        : pMemPoolCfg -  pointer to tMemPoolCfg structure   */
/*  Output(s)       : None                                              */
/*  Returns         : MEM_SUCCESS on successful Initialisation          */
/*                    MEM_FAILURE on error                              */
/************************************************************************/
UINT4
MemInitMemPool (tMemPoolCfg * pMemPoolCfg)
{
    UINT4               u4rc;
    UINT1               au1Name[OSIX_NAME_LEN + 4];

    if (gu4MemPoolInitialized)
        return MEM_FAILURE;

    if (MemPoolInitVar (pMemPoolCfg) == MEM_FAILURE)
        return (MEM_FAILURE);

    if ((u4rc = MemPoolInitializeFreePoolList ()) == MEM_FAILURE)
    {
        POOL_MEM_FREE (gptMemTypeArray);
        return MEM_FAILURE;
    }

    /* Create a Semaphore for MUTEX operations within MEM. */
    MEMSET (au1Name, '\0', OSIX_NAME_LEN + 4);
    STRCPY (au1Name, MEM_MUTEX_NAME);
    u4rc = OsixSemCrt (au1Name, &MemMutex);
    if (u4rc)
    {
        POOL_MEM_FREE (gptMemTypeArray);
        return MEM_FAILURE;
    }
    OsixSemGive (MemMutex);
    gu4MemPoolInitialized = 1;
    return MEM_SUCCESS;
}

/************************************************************************/
/*  Function Name   : MemCreateMemPoolDbg / MemCreateMemPool            */
/*  Description     : Allocates a memory pool for the requested number  */
/*                    of requested sized blocks from the specified      */
/*                    type.                                             */
/*  Input(s)        : u4BlockSize -  Size of memory blocks              */
/*                  : u4NumberOfBlocks -  Maximum number of blocks      */
/*                  : u4TypeOfMemory -  Memory from which Pool should   */
/*                    be allocated. Valid types are:                    */
/*                    MEM_DEFAULT_MEMORY_TYPE - Allocates only from Pool*/
/*                    MEM_HEAP_MEMORY_TYPE - Allocates from heap        */
/*                    when MemPool drain out.                           */
/*                  : pu1FilePath - contain the path of the file that   */
/*                    called this function                              */
/*                  : pu1Funcname - contain the name of the function    */
/*                    which called this function                        */
/*                  : u4LineNo - contain the line numberfrom where this */
/*                    this function was invoked                         */
/*                  : pu1Variable - contain the variable name that      */
/*                    passed the poolid to  this function               */
/*  Output(s)       : pPoolId - ID of memory which has been allocated   */
/*  Returns         : MEM_SUCCESS / MEM_FAILURE                         */
/************************************************************************/
#if DEBUG_MEM == FSAP_ON
/* CAMEOTAG: delete by linyu on 2014-01-22, func and line number decide the location */
UINT4
MemCreateMemPoolDbg (UINT4 u4BlockSize,
                     UINT4 u4NumberOfBlocks,
                     UINT4 u4TypeOfMemory, tMemPoolId * pPoolId,
                     /*const CHR1 * pu1FilePath,*/ const CHR1 * pu1FuncName,
                     UINT4 u4LineNo/*, const CHR1 * pu1Variable*/)
#else
UINT4
MemCreateMemPool (UINT4 u4BlockSize,
                  UINT4 u4NumberOfBlocks,
                  UINT4 u4TypeOfMemory, tMemPoolId * pPoolId)
#endif
{
    UINT1              *pu1PoolBase;
    UINT1              *pu1BufStart;
    UINT4               u4CurrObj;
    INT4                i4PoolId;
    UINT1               au1Name[OSIX_NAME_LEN + 4];
    UINT4               u4InBlockSize = u4BlockSize;
#if DEBUG_MEM == FSAP_ON
/*
    const CHR1         *pu1File;
*/
#endif
    if ((u4BlockSize == 0) || (u4NumberOfBlocks == 0) || (pPoolId == NULL))
        return (MEM_FAILURE);

    /* Align size of each Mem Block to 4 bytes. */
    u4BlockSize = (u4BlockSize + MEM_ALIGN_BYTE) & MEM_ALIGN;
	/* CAMEOTAG: add by linyu on 2014-01-15, correct size for debug info start */
	u4InBlockSize = u4BlockSize;

    /* If Memory type is not default then validate the memory type */
    if ((u4TypeOfMemory != MEM_DEFAULT_MEMORY_TYPE) &&
        ((u4TypeOfMemory & MEM_HEAP_MEMORY_TYPE) != MEM_HEAP_MEMORY_TYPE))
    {
        if (MemPoolValidateCreateParam (u4BlockSize, u4NumberOfBlocks,
                                        u4TypeOfMemory) == MEM_FAILURE)
            return (MEM_FAILURE);
    }

    if ((i4PoolId = MemGetFreePoolId ()) == (INT4) MEM_FAILURE)
    {
        MEM_DBG ((MEM_DBG_FLAG, MEM_DBG_MAJOR, MEM_MODNAME,
                  "\n InSufficient Buffer Records Allocated\n"));
        return MEM_FAILURE;
    }
    /****** Allocating The Buffer Pool ******/

    /*
     * calloc for one extra block than what user wants,
     * to allow for alignment of the first byte to 4-byte boundary.
     */

#if DEBUG_MEM == FSAP_ON
    u4BlockSize += ((sizeof (tMemDebugInfo) + MEM_ALIGN_BYTE) & MEM_ALIGN);
#endif
    /* Allocate one more block so that first block can be 4 byte aligned.
     */
    if (!(pu1PoolBase = POOL_MEM_CALLOC (u4BlockSize, u4NumberOfBlocks + 1, UINT1)))
    {
        MEM_DBG ((MEM_DBG_FLAG, MEM_DBG_MAJOR, MEM_MODNAME,
                  "\n InSufficient Memory\n"));
        return MEM_FAILURE;
    }

    /* Set the MemType to indicate whether the alloc is from Pool/Heap */
    pMemFreePoolRecList[i4PoolId].u4MemType = u4TypeOfMemory;

    /* Save the pointer to be used for free() */
    pMemFreePoolRecList[i4PoolId].pu1ActualBase = pu1PoolBase;

    /* Align first block to beginning of next 4-byte boundary. */
    pu1PoolBase =
        (UINT1 *) (((FS_ULONG) pu1PoolBase + MEM_ALIGN_BYTE) & MEM_ALIGN);

    pu1BufStart = pu1PoolBase;
    pMemFreePoolRecList[i4PoolId].BufList.pu1Base = pu1PoolBase;
    pMemFreePoolRecList[i4PoolId].BufList.pu1Head = pu1PoolBase;

    pMemFreePoolRecList[i4PoolId].u4Size = u4InBlockSize;

    /*
     * Chain the free pool objects in a single linked list
     * For debugging, add free/available status of the block
     * and
     * a signature unique to memblocks of this pool.
     *
     * This will enable detection of multiple free of the same block
     * and free-ing of a block belonging to one pool into another.
     */
    for (u4CurrObj = 1; u4CurrObj < u4NumberOfBlocks; u4CurrObj++)
    {

#if DEBUG_MEM == FSAP_ON
#ifndef CAMEO_MEMTRACE_MINMEM_WANTED
        {
            UINT2               u2QueId = (UINT2) i4PoolId;

            MEM_DEBUG_SYSTEM_DATA (pu1PoolBase) = mem_debug_block_free;
            MEM_DEBUG_SIGNATURE (pu1PoolBase) =
                MEM_DEBUG_SIGNATURE_VAL + u2QueId;
        }
#endif
#endif
        ((tCRU_SLL_NODE *) (VOID *) pu1PoolBase)->pNext =
            (pu1PoolBase + u4BlockSize);
        pu1PoolBase += u4BlockSize;

    }
    ((tCRU_SLL_NODE *) (VOID *) pu1PoolBase)->pNext = NULL;

#if DEBUG_MEM == FSAP_ON
#ifndef CAMEO_MEMTRACE_MINMEM_WANTED
    /* Set debug fields in the last block. */
    {
        UINT2               u2QueId = (UINT2) i4PoolId;

        MEM_DEBUG_SYSTEM_DATA (pu1PoolBase) = mem_debug_block_free;
        MEM_DEBUG_SIGNATURE (pu1PoolBase) = MEM_DEBUG_SIGNATURE_VAL + u2QueId;
    }
#endif
#endif

    /*****************************************************
     *** Perform Other Initialisations and Updations to ***
     *** the buffer pool record                         ***
     ******************************************************/
    pMemFreePoolRecList[i4PoolId].u4UnitsCount = u4NumberOfBlocks;
    pMemFreePoolRecList[i4PoolId].u4FreeUnitsCount = u4NumberOfBlocks;
    pMemFreePoolRecList[i4PoolId].u4Size = u4InBlockSize;
    pMemFreePoolRecList[i4PoolId].pu1StartAddr = (UINT1 *) pu1BufStart;
    pMemFreePoolRecList[i4PoolId].pu1EndAddr =
        (UINT1 *) (pu1BufStart + ((u4NumberOfBlocks * u4BlockSize) - 1));

#if DEBUG_MEM == FSAP_ON
    pMemFreePoolRecList[i4PoolId].u4AllocCount = 0;
    pMemFreePoolRecList[i4PoolId].u4ReleaseCount = 0;
    pMemFreePoolRecList[i4PoolId].u4AllocFailCount = 0;
    pMemFreePoolRecList[i4PoolId].u4HeapAllocFailCount = 0;
    pMemFreePoolRecList[i4PoolId].u4ReleaseFailCount = 0;
    pMemFreePoolRecList[i4PoolId].u4PeakUsageCount = 0;
    pMemFreePoolRecList[i4PoolId].u4LineNo = u4LineNo;
/*
    pu1File = pu1FilePath;
    while (*pu1FilePath != '\0')
    {
        if (*pu1FilePath == '/')
        {
            pu1File = ++pu1FilePath;
        }
        else
        {
            pu1FilePath++;
        }
    }
    STRCPY (pMemFreePoolRecList[i4PoolId].au1File, pu1File);
*/
/*
    STRNCPY (pMemFreePoolRecList[i4PoolId].au1Func, pu1FuncName,
             MEM_DBG_INFO_LEN);
*/
	pMemFreePoolRecList[i4PoolId].pu1Func = pu1FuncName;
/*
    STRNCPY (pMemFreePoolRecList[i4PoolId].au1Variable, pu1Variable,
             MEM_DBG_INFO_LEN);
*/
#endif

    MEMSET (au1Name, '\0', OSIX_NAME_LEN + 4);
    au1Name[0] = (UINT1) ((UINT1) (i4PoolId >> 16) + '0');
    au1Name[1] = (UINT1) ((UINT1) (i4PoolId >> 8) + '0');
    au1Name[2] = (UINT1) ((UINT1) (i4PoolId) + '0');
    au1Name[3] = 'm';

    if (OsixSemCrt (au1Name, &(pMemFreePoolRecList[i4PoolId].SemId)) !=
        OSIX_SUCCESS)
    {
        pMemFreePoolRecList[i4PoolId].u4FreeUnitsCount = 0;
        pMemFreePoolRecList[i4PoolId].u4MemType = MEM_DEFAULT_MEMORY_TYPE;
        POOL_MEM_FREE (pMemFreePoolRecList[i4PoolId].pu1ActualBase);
        pMemFreePoolRecList[i4PoolId].BufList.pu1Base = NULL;
        pMemFreePoolRecList[i4PoolId].BufList.pu1Head = NULL;
        pMemFreePoolRecList[i4PoolId].u4Size = 0;

        pMemFreePoolRecList[i4PoolId].pu1StartAddr = NULL;
        pMemFreePoolRecList[i4PoolId].pu1EndAddr = NULL;
        pMemFreePoolRecList[i4PoolId].u4UnitsCount = 0;

        MEM_DBG ((MEM_DBG_FLAG, MEM_DBG_MAJOR, MEM_MODNAME,
                  "\nSema4 creation failed in MemCreateMemPool.\n"));
        return MEM_FAILURE;
    }

    OsixSemGive (pMemFreePoolRecList[i4PoolId].SemId);
    *pPoolId = (i4PoolId + 1);

    return MEM_SUCCESS;
}

/************************************************************************/
/*  Function Name   : MemDeleteMemPool                                  */
/*  Description     : Delete the memPool.                               */
/*  Input(s)        :                                                   */
/*                  : PoolId - Pool ID to be deleted.                   */
/*  Output(s)       : None.                                             */
/*  Returns         : MEM_SUCCESS/MEM_FAILURE                           */
/************************************************************************/
UINT4
MemDeleteMemPool (tMemPoolId PoolId)
{
    UINT2               u2PoolId = 0;

    if ((PoolId == 0) || (PoolId > gtMemPoolCfg.u4MaxMemPools))
        return MEM_FAILURE;

    u2PoolId = (UINT2) (PoolId - 1);

    if (u2PoolId >= (gtMemPoolCfg.u4MaxMemPools))
        return MEM_FAILURE;

    if (pMemFreePoolRecList[u2PoolId].u4UnitsCount == 0)
        return MEM_FAILURE;

    OsixSemDel (pMemFreePoolRecList[u2PoolId].SemId);

    pMemFreePoolRecList[u2PoolId].u4FreeUnitsCount = 0;
    POOL_MEM_FREE (pMemFreePoolRecList[u2PoolId].pu1ActualBase);
    pMemFreePoolRecList[u2PoolId].BufList.pu1Base = NULL;
    pMemFreePoolRecList[u2PoolId].BufList.pu1Head = NULL;
    pMemFreePoolRecList[u2PoolId].u4Size = 0;

#if DEBUG_MEM == FSAP_ON
    pMemFreePoolRecList[u2PoolId].u4AllocCount = 0;
    pMemFreePoolRecList[u2PoolId].u4ReleaseCount = 0;
    pMemFreePoolRecList[u2PoolId].u4AllocFailCount = 0;
    pMemFreePoolRecList[u2PoolId].u4ReleaseFailCount = 0;
    pMemFreePoolRecList[u2PoolId].u4PeakUsageCount = 0;
	pMemFreePoolRecList[u2PoolId].u4LineNo = 0;
	pMemFreePoolRecList[u2PoolId].pu1Func = NULL;
#endif

    pMemFreePoolRecList[u2PoolId].pu1StartAddr = NULL;
    pMemFreePoolRecList[u2PoolId].pu1EndAddr = NULL;

    /* Since the value u4UnitsCount is also used as flag to
     * signify the used/unused status of a pool, we flag it
     * as unused as the last step. Otherwise we land up in a
     * race condition.
     */
    pMemFreePoolRecList[u2PoolId].u4UnitsCount = 0;
    return MEM_SUCCESS;
}

/************************************************************************/
/*  Function Name   : MemAllocateMemBlock                               */
/*  Description     : Allocates a block from a specficied Pool, if free */
/*                    Block is available in the Pool. If the Pool drain */
/*                    out, and if the pool is created with memory type  */
/*                    MEM_HEAP_MEMORY_TYPE, allocates the Block         */
/*                    from Heap.                                        */
/*  Input(s)        :                                                   */
/*                  : PoolId    - Pool From which to allocate.          */
/*  Output(s)       : ppu1Block - Pointer to block.                     */
/*  Returns         : MEM_SUCCESS/MEM_FAILURE                           */
/************************************************************************/
#if DEBUG_MEM == FSAP_ON
/* CAMEOTAG: delete by linyu on 2014-01-22, func and line number decide the location */
UINT4
MemAllocateMemBlockLeak (tMemPoolId PoolId, UINT1 **ppu1Block,
                         /*const CHR1 * pc1FilePath,*/ UINT4 u4LineNo,
                         const CHR1 * pFunc)
#else
UINT4
MemAllocateMemBlock (tMemPoolId PoolId, UINT1 **ppu1Block)
#endif
{
    UINT2               u2QueId;
    UINT1              *pNode;
    UINT1              *pu1Blk;
    tCRU_SLL           *pPool;
    tMemFreePoolRec    *pPoolRecPtr;
#if DEBUG_MEM == FSAP_ON
/*
    const CHR1         *pc1File;
*/
#endif

#if DEBUG_MEM == FSAP_ON
    if ((PoolId == 0) || (PoolId > gtMemPoolCfg.u4MaxMemPools))
        return MEM_FAILURE;

    if (ppu1Block == NULL)
        return MEM_FAILURE;

    if (pMemFreePoolRecList[PoolId - 1].u4UnitsCount == 0)
        return MEM_FAILURE;
#endif

    /* PoolIds returned by MemCreateMemPool is QueId + 1. So decrement. */
    u2QueId = (UINT2) (PoolId - 1);

    MEM_ENTER_CS (u2QueId);

    pPoolRecPtr = &pMemFreePoolRecList[u2QueId];
    pPool = &pMemFreePoolRecList[u2QueId].BufList;
    pNode = pPool->pu1Head;

    if (pNode)
    {
        *ppu1Block = pNode;
        pPoolRecPtr->u4FreeUnitsCount--;
        pPool->pu1Head = ((tCRU_SLL_NODE *) (VOID *) pNode)->pNext;

#if DEBUG_MEM == FSAP_ON
        pPoolRecPtr->u4AllocCount++;
        pPoolRecPtr->u4PeakUsageCount =
            (pPoolRecPtr->u4PeakUsageCount < pPoolRecPtr->u4AllocCount) ?
            pPoolRecPtr->u4PeakUsageCount : pPoolRecPtr->u4AllocCount;
/*
        pc1File = pc1FilePath;
        while (*pc1FilePath != '\0')
        {
            if (*pc1FilePath == '/')
            {
                pc1File = ++pc1FilePath;
            }
            else
            {
                pc1FilePath++;
            }
        }

        STRCPY (MEM_DEBUG_FILE (*ppu1Block), pc1File);
        STRNCPY (MEM_DEBUG_FUNC (*ppu1Block), pFunc, MEM_DBG_INFO_LEN);
*/
		MEM_DEBUG_FUNC (*ppu1Block) = pFunc;
#ifdef CAMEO_MEMTRACE_MINMEM_WANTED
		PARAM_UNUSED (u4LineNo);
#else
        MEM_DEBUG_LINE (*ppu1Block) = u4LineNo;
        MEM_DEBUG_SYSTEM_DATA (*ppu1Block) = mem_debug_block_allocated;
        OsixGetSysTime (&MEM_DEBUG_TIMESTAMP (*ppu1Block));
#endif /* CAMEO_MEMTRACE_MINMEM_WANTED */
#ifndef MEMTRACE_WANTED
        /*
         * Warn if memPool is draining out.
         * Threshold is set to 1% of maximum Pool size.
         */
        if (1.0 * pPoolRecPtr->u4FreeUnitsCount / pPoolRecPtr->u4UnitsCount
            < MEM_THRESHOLD_VAL)
        {
            MEM_DBG ((MEM_DBG_FLAG, MEM_DBG_CRITICAL, MEM_MODNAME,
                      "-I- MemPool with Pool Id %d draining out at...\n",
                      PoolId));

        }
#endif
        /* Does this block belong to *this* pool. */
        if (MemIsValidBlock (PoolId, *ppu1Block) == MEM_FAILURE)
        {
            *ppu1Block = NULL;
            MEM_PRNT ("-E-: Spotted Corruption during MemAllocate.\n");
            MEM_LEAVE_CS (u2QueId);
            return MEM_FAILURE;
        }
#endif
        MEM_LEAVE_CS (u2QueId);
        return MEM_SUCCESS;
    }
    else
    {
#if DEBUG_MEM == FSAP_ON
        pPoolRecPtr->u4AllocFailCount++;
#endif
        if ((pPoolRecPtr->u4MemType) & MEM_HEAP_MEMORY_TYPE)
        {
            pu1Blk = POOL_MEM_CALLOC (sizeof (UINT1), pPoolRecPtr->u4Size, UINT1);
            if (pu1Blk)
            {
                *ppu1Block = pu1Blk;
#if DEBUG_MEM == FSAP_ON
                pPoolRecPtr->u4AllocCount++;
#endif
                MEM_LEAVE_CS (u2QueId);
                return MEM_SUCCESS;
            }
            else
            {
#if DEBUG_MEM == FSAP_ON
                pPoolRecPtr->u4HeapAllocFailCount++;
#endif
                MEM_LEAVE_CS (u2QueId);
                return MEM_FAILURE;
            }
        }
        else
        {
            MEM_LEAVE_CS (u2QueId);
            return MEM_FAILURE;
        }
    }
}

/************************************************************************/
/*  Function Name   : MemAllocMemBlk                                  */
/*  Description     : Allocates a block from a specficied Pool, if free */
/*                    Block is available in the Pool. If the Pool drain */
/*                    out, and if the pool is created with memory type  */
/*                    MEM_HEAP_MEMORY_TYPE, allocates the Block         */
/*                    from Heap.                                        */
/*  Input(s)        :                                                   */
/*                  : PoolId    - Pool From which to allocate.          */
/*  Output(s)       : None                                              */
/*  Returns         : Pointer to the Block / NULL                       */
/************************************************************************/
#if DEBUG_MEM == FSAP_ON
/* CAMEOTAG: delete by linyu on 2014-01-22, func and line number decide the location */
VOID               *
MemAllocMemBlkLeak (tMemPoolId PoolId,
                    /*const CHR1 * pc1FilePath,*/ UINT4 u4LineNo,
                    const CHR1 * pFunc)
#else
VOID               *
MemAllocMemBlk (tMemPoolId PoolId)
#endif
{
    UINT1              *pu1Block = NULL;
#if DEBUG_MEM == FSAP_ON
    if (MemAllocateMemBlockLeak
        (PoolId, &pu1Block, /*pc1FilePath,*/ u4LineNo, pFunc) != MEM_SUCCESS)
#else
    if (MemAllocateMemBlock (PoolId, &pu1Block) != MEM_SUCCESS)
#endif
    {
        return NULL;
    }
    return (VOID *) pu1Block;
}

/************************************************************************/
/*  Function Name   : MemReleaseMemBlock                                */
/*  Description     : Releases a block back to a specified Pool/Heap    */
/*  Input(s)        :                                                   */
/*                  : PoolId   - Pool to which to be released.          */
/*  Output(s)       : pu1Block - Pointer to block being released.       */
/*  Returns         : MEM_SUCCESS/MEM_FAILURE                           */
/************************************************************************/
/* CAMEOTAG: modify by linyu on 2014-04-01, func and line number decide the location */
#if DEBUG_MEM == FSAP_ON
UINT4
MemReleaseMemBlockDbg (tMemPoolId PoolId, UINT1 *pu1Block, 
                    const CHR1 * pFunc, UINT4 u4LineNo)
#else
UINT4
MemReleaseMemBlock (tMemPoolId PoolId, UINT1 *pu1Block)
#endif
{
    UINT2               u2QueId;
    tMemFreePoolRec    *pPoolRecPtr;
    CHR1 au1Str [64];

    u2QueId = (UINT2) (PoolId - 1);

    MEM_ENTER_CS (u2QueId);
    pPoolRecPtr = &pMemFreePoolRecList[u2QueId];

#if DEBUG_MEM == FSAP_ON
    if ((PoolId == 0) || (PoolId > gtMemPoolCfg.u4MaxMemPools))
    {
        MEM_LEAVE_CS (u2QueId);
        return MEM_FAILURE;
    }
    if (!pu1Block)
    {
        pPoolRecPtr->u4ReleaseFailCount++;
        MEM_LEAVE_CS (u2QueId);
        return MEM_FAILURE;
    }
#endif

    /* Does this block belong to *this* pool. */
    if (MemIsValidBlock (PoolId, pu1Block) == MEM_FAILURE)
    {
        /* No. The Block is not from Pool. It's from the Heap. So call free */
        if ((pPoolRecPtr->u4MemType) & MEM_HEAP_MEMORY_TYPE)
        {
            POOL_MEM_FREE (pu1Block);

#if DEBUG_MEM == FSAP_ON
            pPoolRecPtr->u4ReleaseCount++;
#endif
            MEM_LEAVE_CS (u2QueId);
            return MEM_SUCCESS;
        }
        else
        {
#if DEBUG_MEM == FSAP_ON
			PRINTF ("[%s-%d]\n", pFunc, u4LineNo);
#endif
			/* CAMEOTAG: modify by liny on 2014-01-09, pool debug by id and name */
            SPRINTF(au1Str, "[%d]%s\n", PoolId, "-E-: Invalid MemRelease.");
            MEM_PRNT ((const CHR1 *)au1Str);
            /*MEM_PRNT ("-E-: Invalid MemRelease.\n");*/

            MEM_LEAVE_CS (u2QueId);
            return MEM_FAILURE;
        }
    }

#if DEBUG_MEM == FSAP_ON
#ifndef CAMEO_MEMTRACE_MINMEM_WANTED
    /* Detect duplicate frees */
    if (MEM_DEBUG_SYSTEM_DATA (pu1Block) == mem_debug_block_free)
    {
        char               *p = 0;
		PRINTF ("[%s-%d]release pool %d block %p(allocated by %s-%d) failed\n", 
				pFunc, u4LineNo, PoolId, pu1Block, 
				MEM_DEBUG_FUNC (pu1Block), MEM_DEBUG_LINE (pu1Block));
        *p = 0;
    }
    /* Detect mem. corruption. */
    {
        UINT4               u4Sig;
        char               *p = 0;

        u4Sig = MEM_DEBUG_SIGNATURE (pu1Block);
        if (MEM_DEBUG_SIGNATURE (pu1Block) !=
            (UINT4) MEM_DEBUG_SIGNATURE_VAL + u2QueId)
		{
			PRINTF ("[%s-%d]release pool %d block %p(allocated by %s-%d) failed\n", 
					pFunc, u4LineNo, PoolId, pu1Block, 
					MEM_DEBUG_FUNC (pu1Block), MEM_DEBUG_LINE (pu1Block));
            *p = 0;
		}
    }
    /* Time Info. */
    {
        UINT4               u4Time;

        OsixGetSysTime (&u4Time);
        u4Time -= MEM_DEBUG_TIMESTAMP (pu1Block);

    }
    MEMSET (pu1Block, '\0', pPoolRecPtr->u4Size);
	MEM_DEBUG_LINE (pu1Block) = 0;
#endif
#endif

    ((tCRU_SLL_NODE *) (VOID *) pu1Block)->pNext = pPoolRecPtr->BufList.pu1Head;
    pPoolRecPtr->BufList.pu1Head = pu1Block;
    pPoolRecPtr->u4FreeUnitsCount++;

#if DEBUG_MEM == FSAP_ON
    pPoolRecPtr->u4ReleaseCount++;
#endif

#if DEBUG_MEM == FSAP_ON
	MEM_DEBUG_FUNC (pu1Block) = NULL;
#ifndef CAMEO_MEMTRACE_MINMEM_WANTED
    MEM_DEBUG_SYSTEM_DATA (pu1Block) = mem_debug_block_free;
#endif
#endif
    MEM_LEAVE_CS (u2QueId);
    return (MEM_SUCCESS);
}

/************************************************************************/
/*  Function Name   : MemPrintMemPoolStatistics                         */
/*  Description     : Function to print MemPool Statistics.             */
/*                  : If invoked with arg. 0 prints stats for all pools.*/
/*  Input(s)        :                                                   */
/*                  : PoolId - Pool Id for which stats is sought.       */
/*  Output(s)       :                                                   */
/*  Returns         : MEM_SUCCESS/MEM_FAILURE.                          */
/************************************************************************/
#if DEBUG_MEM == FSAP_ON
void
MemLeak (tMemPoolId PoolId)
{
    tCRU_SLL_NODE      *pNode;
    tMemFreePoolRec    *pPoolRecPtr;
    UINT2               u2QueId;
    UINT4               u4NumberOfBlocks;
    UINT4               u4BlockSize;
    UINT4               u4CurrObj;
    UINT1               au1Buf[200];

    for (u2QueId = PoolId ? PoolId - 1 : 0;
         (u2QueId < gtMemPoolCfg.u4MaxMemPools) &&
         (PoolId ? u2QueId != PoolId : 1); u2QueId++)
    {
        /*CAMEOTAG:Add by tongliang 2016/11/18
                MemLeak only for debug,gTcbTblPoolId use was especial,it would lead to system crash when debug gTcbTblPoolId pool
                so skip gTcbTblPoolId;
                */
        /*if ((gTcbTblPoolId -1) == u2QueId)
        {
            continue;
        }*/
        u4NumberOfBlocks = pMemFreePoolRecList[u2QueId].u4UnitsCount;
        u4BlockSize = pMemFreePoolRecList[u2QueId].u4Size;
        u4BlockSize += ((sizeof (tMemDebugInfo) + MEM_ALIGN_BYTE) & MEM_ALIGN);
        pNode = (tCRU_SLL_NODE *)(VOID *) pMemFreePoolRecList[u2QueId].BufList.pu1Base;
        pPoolRecPtr = &pMemFreePoolRecList[u2QueId];

        SPRINTF ((char *) au1Buf, "\npool %d, size %d, num %d, ",
                 u2QueId + 1, u4BlockSize, u4NumberOfBlocks);
        mmi_printf ("%s", (const char *) au1Buf);
        SPRINTF ((char *) au1Buf, /*%s*/"%s:%d\n",
                 /*pPoolRecPtr->au1File,*/ pPoolRecPtr->pu1Func,
                 pPoolRecPtr->u4LineNo/*, pPoolRecPtr->au1Variable*/);
        mmi_printf ("%s", (const char *) au1Buf);
        for (u4CurrObj = 1; u4CurrObj <= u4NumberOfBlocks; u4CurrObj++)
        {
/*
            if ((STRLEN (MEM_DEBUG_FUNC (pNode)) != 0))
            {
                SPRINTF ((char *) au1Buf, "%x %s\n",
                         (UINT4) pNode, MEM_DEBUG_FUNC (pNode));
                mmi_printf ("%s", (const char *) au1Buf);
            }
*/
			/* Function name is constant string, no need memory allocate */
            if (/*(STRLEN (MEM_DEBUG_FILE (pNode)) != 0) &&
                (STRLEN (MEM_DEBUG_FUNC (pNode)) != 0) &&
                (MEM_DEBUG_LINE (pNode) != 0)*/
				(MEM_DEBUG_FUNC (pNode) != NULL))
            {
#ifdef CAMEO_MEMTRACE_MINMEM_WANTED
                SPRINTF ((char *) au1Buf, "%x %s\n",
                         (UINT4) pNode, MEM_DEBUG_FUNC (pNode));
#else
                SPRINTF ((char *) au1Buf, "%p %s %d\n",
                         pNode,
                         /*MEM_DEBUG_FILE (pNode),*/
                         MEM_DEBUG_FUNC (pNode), MEM_DEBUG_LINE (pNode));
				/*PRINTF ("%x %x\n", (UINT4)pNode, (UINT4)MEM_DEBUG_FUNC (pNode));*/
#endif
                mmi_printf ("%s", (const char *) au1Buf);
            }

            pNode = (tCRU_SLL_NODE *)(VOID *) ((UINT1 *) pNode + u4BlockSize);
        }
    }
}

UINT4
MemPrintMemPoolStatistics (tMemPoolId PoolId)
{
    UINT4               u4PoolId = 0;
    UINT1               au1Buf[200];
	UINT4				u4Total = 0, u4TotalUnits = 0;

    if (PoolId > gtMemPoolCfg.u4MaxMemPools)
        return (MEM_FAILURE);

    mmi_printf (" |*******|*******|*******|*******|*****|***************\n");
    mmi_printf (" |Pool   |TOTAL  |FREE   |UNIT   |Line |Func           \n");
    mmi_printf (" | ID    |UNITS  |UNITS  |SIZES  |Num  |Name           \n");

    if (PoolId == 0)
    {
        for (u4PoolId = 0; u4PoolId < (gtMemPoolCfg.u4MaxMemPools); u4PoolId++)
        {
			/* CAMEOTAG: modify by liny on 2014-01-09, pool debug by id and name */
            if (pMemFreePoolRecList[u4PoolId].u4Size)
            {                    /* UnAlloced pool */
				u4Total += (pMemFreePoolRecList[u4PoolId].u4UnitsCount * 
							pMemFreePoolRecList[u4PoolId].u4Size);
				u4TotalUnits += pMemFreePoolRecList[u4PoolId].u4UnitsCount;

                mmi_printf
                    (" |-------|-------|-------|-------|-----|---------------\n");
                SPRINTF ((char *) au1Buf, " |%6d |%6d |%6d |%6d |%4d |%s\n",
                         (u4PoolId + 1),
                         pMemFreePoolRecList[u4PoolId].u4UnitsCount,
                         pMemFreePoolRecList[u4PoolId].u4FreeUnitsCount, 
						 pMemFreePoolRecList[u4PoolId].u4Size, 
						 /*pMemFreePoolRecList[u4PoolId].au1File,*/
                         pMemFreePoolRecList[u4PoolId].u4LineNo,
                         pMemFreePoolRecList[u4PoolId].pu1Func);

                mmi_printf ("%s", (const char *) au1Buf);
            }
        }
    }
    else
    {
		u4Total += (pMemFreePoolRecList[PoolId - 1].u4UnitsCount * 
							pMemFreePoolRecList[PoolId - 1].u4Size);
		u4TotalUnits += pMemFreePoolRecList[PoolId - 1].u4UnitsCount;

        mmi_printf (" |-------|-------|-------|-------|-----|---------------\n");

        SPRINTF ((char *) au1Buf,
                 " |%6d |%6d |%6d |%6d |%4d |%s\n",
                 PoolId, pMemFreePoolRecList[PoolId - 1].u4UnitsCount,
                 pMemFreePoolRecList[PoolId - 1].u4FreeUnitsCount,
				 pMemFreePoolRecList[PoolId - 1].u4Size, 
				 /*pMemFreePoolRecList[PoolId - 1].au1File,*/
                 pMemFreePoolRecList[PoolId - 1].u4LineNo,
                 pMemFreePoolRecList[PoolId - 1].pu1Func);

        mmi_printf ("%s", (const char *) au1Buf);
    }

    mmi_printf
        (" |******************************************************|\n\n");
	mmi_printf
        ("Total: UINTS %u, %u BYTES\n\n", u4TotalUnits, u4Total);

    return MEM_SUCCESS;
}
#else /* DEBUG_MEM == FSAP_OFF */
/* CAMEOTAG: add by linyu on 2014-04-18, 
 * support basic statistic for memory pool, include id, total/free units 
 * run x86, compare result whill help find the pool name and line to decide module
 */
UINT4
MemPrintMemPoolStatistics (tMemPoolId PoolId)
{
    UINT4               u4PoolId = 0;
    UINT1               au1Buf[200];
	UINT4				u4Total = 0, u4TotalUnits = 0;

    if (PoolId > gtMemPoolCfg.u4MaxMemPools)
        return (MEM_FAILURE);

    mmi_printf (" |*******|*******|*******|*******\n");
    mmi_printf (" |Pool   |TOTAL  |FREE   |UNIT   \n");
    mmi_printf (" | ID    |UNITS  |UNITS  |SIZES  \n");

    if (PoolId == 0)
    {
        for (u4PoolId = 0; u4PoolId < (gtMemPoolCfg.u4MaxMemPools); u4PoolId++)
        {
            if (pMemFreePoolRecList[u4PoolId].u4Size)
            {
				u4Total += (pMemFreePoolRecList[u4PoolId].u4UnitsCount * 
							pMemFreePoolRecList[u4PoolId].u4Size);
				u4TotalUnits += pMemFreePoolRecList[u4PoolId].u4UnitsCount;

                mmi_printf
                    (" |-------|-------|-------|-------\n");
                SPRINTF ((char *) au1Buf, " |%6d |%6d |%6d |%6d \n",
                         (u4PoolId + 1),
                         pMemFreePoolRecList[u4PoolId].u4UnitsCount,
                         pMemFreePoolRecList[u4PoolId].u4FreeUnitsCount, 
						 pMemFreePoolRecList[u4PoolId].u4Size);

                mmi_printf ("%s", (const char *) au1Buf);
            }
        }
    }
    else
    {
		u4Total += (pMemFreePoolRecList[PoolId - 1].u4UnitsCount * 
							pMemFreePoolRecList[PoolId - 1].u4Size);
		u4TotalUnits += pMemFreePoolRecList[PoolId - 1].u4UnitsCount;

        mmi_printf (" |-------|-------|-------|-------\n");

        SPRINTF ((char *) au1Buf,
                 " |%6d |%6d |%6d |%6d \n",
                 PoolId, pMemFreePoolRecList[PoolId - 1].u4UnitsCount,
                 pMemFreePoolRecList[PoolId - 1].u4FreeUnitsCount,
				 pMemFreePoolRecList[PoolId - 1].u4Size);

        mmi_printf ("%s", (const char *) au1Buf);
    }

    mmi_printf
        (" |******************************|\n\n");
	mmi_printf
        ("Total: UINTS %u, %u BYTES\n\n", u4TotalUnits, u4Total);

    return MEM_SUCCESS;
}
#endif

/* Just print the block data which in use */
UINT4 MemPrintMemPoolContent (tMemPoolId PoolId)
{
    tCRU_SLL_NODE      *pNode;
    UINT2               u2QueId;
    UINT4               u4NumberOfBlocks;
    UINT4               u4BlockSize, u4BlockDSize;
    UINT4               u4CurrObj, u4DataIndex;
    UINT1               au1Buf[200];
	UINT1			   *pu1PoolHead;
    UINT1              *pu1Start;
    UINT1              *pu1End;
	UINT4				*pu4Data;
	UINT1				au1DataBuf[128 + 4];

	if (PoolId < 1 || PoolId > gtMemPoolCfg.u4MaxMemPools)
		return (MEM_FAILURE);

	u2QueId = PoolId - 1;

    pu1Start = pMemFreePoolRecList[u2QueId].pu1StartAddr;
    pu1End = pMemFreePoolRecList[u2QueId].pu1EndAddr;
	u4NumberOfBlocks = pMemFreePoolRecList[u2QueId].u4UnitsCount;
	u4BlockDSize = u4BlockSize = pMemFreePoolRecList[u2QueId].u4Size;
	/* just show first 64Bytes */
	if (u4BlockDSize > 64) u4BlockDSize = 64;
#if DEBUG_MEM == FSAP_ON
	u4BlockSize += ((sizeof (tMemDebugInfo) + MEM_ALIGN_BYTE) & MEM_ALIGN);
#endif
	pNode = (tCRU_SLL_NODE *)(VOID *) pMemFreePoolRecList[u2QueId].BufList.pu1Base;

	SPRINTF ((char *) au1Buf, "\npool %d, size %d, num %d\n",
		     PoolId, u4BlockSize, u4NumberOfBlocks);
	mmi_printf ("%s", (const char *) au1Buf);

	for (u4CurrObj = 1; u4CurrObj <= u4NumberOfBlocks; u4CurrObj++)
	{
		pu1PoolHead = * ((UINT1 **)pNode);
		pu4Data = (UINT4 *)pNode;
		/* if block not in use, it's first 4 byte acutual is an pointer to next block, 
		 * assert data content can't match the range check
		 */
		if (!(pu1PoolHead >= pu1Start && pu1PoolHead <= pu1End))
		{
		    SPRINTF ((char *) au1Buf, "block %d @ %p\n", u4CurrObj, pNode);
		    mmi_printf ("%s", (const char *) au1Buf);
			MEMSET (au1DataBuf, 0, sizeof (au1DataBuf));
			for (u4DataIndex = 0; u4DataIndex < (u4BlockDSize / 4); u4DataIndex ++)
			{
				SPRINTF ((char *) au1Buf, "%08x", *(pu4Data + u4DataIndex));
				STRCAT (au1DataBuf, au1Buf);
			}
			mmi_printf ("[%s]\n\n", (const char *)au1DataBuf);
		}

		pNode = (tCRU_SLL_NODE *)(VOID *) ((UINT1 *) pNode + u4BlockSize);
	}

	return MEM_SUCCESS;
}

VOID
DebugShowMemPoolDetails(VOID)
{
    UINT4               u4PoolId = 0;
    UINT4               u4InUseCount = 0;
    UINT1               au1Buf[200];
    const char*         sliDisplayIds[] ={
        "SLI_ACCEPT_LIST_BLOCKS",
        "SLI_BUFF_BLOCKS",
        "SLI_DESC_TBL_BLOCKS",
        "SLI_FD_ARR_BLOCKS",
        "SLI_IP_OPT_BLOCKS",
        "SLI_MD5_SLL_NODES",
        "SLI_RAW_HASH_NODES",
        "SLI_RAW_RCV_Q_NODES",
        "SLI_SDT_BLOCKS",
        "SLI_SLL_NODES",
        "SLI_UDP4_BLOCKS",
        "SLI_UDP6_BLOCKS"
    };

    const char*         tcpDisplayIds[] ={
        "TCP_CXT_STRUCTS",
        "TCP_ICMP_PARAMS",
        "TCP_IP_HL_PARMS",
        "TCP_NUM_CONTEXT",
        "TCP_NUM_OF_FRAG",
        "TCP_NUM_OF_RXMT",
        "TCP_NUM_OF_SACK",
        "TCP_NUM_PARAMS",
        "TCP_RCV_BUFF_BLOCKS",
        "TCP_SND_BUFF_BLOCKS",
        "TCP_TCB_TBL_BLOCKS",
        "TCP_TO_APP_BLOCKS",
    };

    mmi_printf (" |***********CRU MemPool Details************|\n");

    mmi_printf (" |***********************|*****|*****|*****|\n");
    mmi_printf (" |Pool                   |TOTAL|FREE |Used |\n");
    mmi_printf (" |Name                   |UNITS|UNITS|UNITS|\n");

    u4InUseCount = (pMemFreePoolRecList[0].u4UnitsCount -
                    pMemFreePoolRecList[0].u4FreeUnitsCount);
    mmi_printf (" |-----------------------|-----|-----|-----|\n");
    snprintf ((char *) au1Buf, sizeof(au1Buf), " |%22s |%4d |%4d |%4d |\n",
              ("CRU"),
              pMemFreePoolRecList[0].u4UnitsCount,
              pMemFreePoolRecList[0].u4FreeUnitsCount,
              u4InUseCount);
    mmi_printf ("%s", (const char *) au1Buf);
    mmi_printf (" |*****************************************|\n\n\n");


/*Show for TCP Mempool Details */
    mmi_printf (" |**********TCPMemPool Details*************|\n");

    mmi_printf (" |***********************|*****|*****|*****|\n");
    mmi_printf (" |Pool                   |TOTAL|FREE |Used |\n");
    mmi_printf (" |Name                   |UNITS|UNITS|UNITS|\n");

    /*for (u4PoolId = 0; u4PoolId < 12; u4PoolId++)
    {
        if ((pMemFreePoolRecList[TCPMemPoolIds[u4PoolId]-1].u4Size != 0))
        {
            u4InUseCount = 
                (pMemFreePoolRecList[TCPMemPoolIds[u4PoolId]-1].u4UnitsCount -
                 pMemFreePoolRecList[TCPMemPoolIds[u4PoolId]-1].u4FreeUnitsCount);
            mmi_printf (" |-----------------------|-----|-----|-----|\n");
            SNPRINTF ((char *) au1Buf, sizeof(au1Buf), " |%22s |%4d |%4d |%4d |\n",
                      (tcpDisplayIds[u4PoolId]),
                      pMemFreePoolRecList[TCPMemPoolIds[u4PoolId]-1].u4UnitsCount,
                      pMemFreePoolRecList[TCPMemPoolIds[u4PoolId]-1].u4FreeUnitsCount,
                      u4InUseCount);
            mmi_printf ("%s", (const char *) au1Buf);
        }
    }*/

    mmi_printf (" |*****************************************|\n\n");
}


/************************************************************************/
/*  Function Name   : MemShutDownMemPool                                */
/*  Description     : Function to shutdown MemManager.                  */
/*  Input(s)        : -                                                 */
/*  Output(s)       : -                                                 */
/*  Returns         : MEM_SUCCESS/MEM_FAILURE                           */
/************************************************************************/
UINT4
MemShutDownMemPool ()
{
    UINT4               u4_PoolId;

    if (!gu4MemPoolInitialized)
    {
        return (MEM_FAILURE);
    }

    OsixSemDel (MemMutex);

    for (u4_PoolId = 0; u4_PoolId < (gtMemPoolCfg.u4MaxMemPools); u4_PoolId++)
    {
        pMemFreePoolRecList[u4_PoolId].u4UnitsCount = 0;
        pMemFreePoolRecList[u4_PoolId].u4FreeUnitsCount = 0;
        POOL_MEM_FREE (pMemFreePoolRecList[u4_PoolId].BufList.pu1Base);
        pMemFreePoolRecList[u4_PoolId].BufList.pu1Base = NULL;
        pMemFreePoolRecList[u4_PoolId].BufList.pu1Head = NULL;
        pMemFreePoolRecList[u4_PoolId].u4Size = 0;

#if DEBUG_MEM == FSAP_ON
        pMemFreePoolRecList[u4_PoolId].u4AllocCount = 0;
        pMemFreePoolRecList[u4_PoolId].u4ReleaseCount = 0;
        pMemFreePoolRecList[u4_PoolId].u4AllocFailCount = 0;
        pMemFreePoolRecList[u4_PoolId].u4HeapAllocFailCount = 0;
        pMemFreePoolRecList[u4_PoolId].u4ReleaseFailCount = 0;
        pMemFreePoolRecList[u4_PoolId].u4PeakUsageCount = 0;
#endif

        pMemFreePoolRecList[u4_PoolId].pu1StartAddr = NULL;
        pMemFreePoolRecList[u4_PoolId].pu1EndAddr = NULL;
        pMemFreePoolRecList[u4_PoolId].u4MemType = MEM_DEFAULT_MEMORY_TYPE;

    }
    POOL_MEM_FREE (pMemFreePoolRecList);
    POOL_MEM_FREE (gptMemTypeArray);
    gu4MemPoolInitialized = 0;
    MemMutex = 0;
    return MEM_SUCCESS;
}

/**************   PRIVATE Functions   ***********************/

/*****************************************************************************/
/*  Function Name   : MemPoolInitVar                                         */
/*  Description     : Function to initialize global vars.                    */
/*  Input(s)        : u4BlockSize - Requested Block Size to Allocate         */
/*                    u4NumberOfBlocks  - Requested #of blocks to Allocate   */
/*                    u4TypeOfMemory  - Specific type of memory from which   */
/*                                allocatio of pool is expected              */
/*  Output(s)       : None                                                   */
/*  Returns         : MEM_SUCCESS / MEM_FAILURE                              */
/*****************************************************************************/
PRIVATE UINT4
MemPoolInitVar (tMemPoolCfg * pMemPoolCfg)
{
    UINT4               u4MemTypeIndex = 0;
    UINT4               u4BytesToAllocate = 0;

    gtMemPoolCfg.u4MaxMemPools = pMemPoolCfg->u4MaxMemPools;
    gtMemPoolCfg.u4NumberOfMemTypes = pMemPoolCfg->u4NumberOfMemTypes;

    if (gtMemPoolCfg.u4MaxMemPools == 0)
        return MEM_FAILURE;

    if (gtMemPoolCfg.u4NumberOfMemTypes == 0)
        return MEM_SUCCESS;

    /* Compute bytes to allocate for MemoryType and Chunk Array */
    for (u4MemTypeIndex = 0; u4MemTypeIndex < gtMemPoolCfg.u4NumberOfMemTypes;
         u4MemTypeIndex++)
    {

        u4BytesToAllocate += ((gtMemPoolCfg.MemTypes[u4MemTypeIndex].u4NumberOfChunks * sizeof (tMemChunkCfg)) + sizeof (UINT4) +    /* for memory type */
                              sizeof (UINT4));    /* for # of chunks */

    }

    if (u4BytesToAllocate)
    {
        gptMemTypeArray = MEM_MALLOC (u4BytesToAllocate, tMemTypeCfg);
        if (gptMemTypeArray == NULL)
            return MEM_FAILURE;

        MEMCPY (gptMemTypeArray, pMemPoolCfg->MemTypes, u4BytesToAllocate);
    }

    return MEM_SUCCESS;
}

/*****************************************************************************/
/*  Function Name   : MemPoolValidateCreateParam                             */
/*  Description     : Validate params for creating a mem pool.               */
/*  Input(s)        : u4BlockSize - Requested Block Size to Allocate         */
/*                    u4NumberOfBlocks  - Requested #of blocks to Allocate   */
/*                    u4TypeOfMemory  - Specific type of memory from which   */
/*                                allocatio of pool is expected              */
/*  Output(s)       : None                                                   */
/*  Returns         : MEM_SUCCESS / MEM_FAILURE                              */
/*****************************************************************************/
PRIVATE UINT4
MemPoolValidateCreateParam (UINT4 u4BlockSize,
                            UINT4 u4NumberOfBlocks, UINT4 u4TypeOfMemory)
{
    UINT4               u4MemTypeIndex = 0;

    /* Unused Params. */
    u4BlockSize = u4BlockSize;
    u4NumberOfBlocks = u4NumberOfBlocks;

    for (u4MemTypeIndex = 0; u4MemTypeIndex < gtMemPoolCfg.u4NumberOfMemTypes;
         u4MemTypeIndex++)
    {
        /* If Memory Type Matches  */
        if (u4TypeOfMemory ==
            (gtMemPoolCfg.MemTypes[u4MemTypeIndex]).u4MemoryType)
        {
            break;
        }
    }

    /*  u4TypeOfMemory doesn't match with the Initialized list of memory types */
    if (u4MemTypeIndex == gtMemPoolCfg.u4NumberOfMemTypes)
        return (MEM_FAILURE);
    return MEM_SUCCESS;
}

/*****************************************************************************/
/*  Function Name   : MemGetFreePoolId                                       */
/*  Description     : This Procedure Returns The Index To A Free Buffer Pool */
/*                    Record After Initialising The tTMO_SLL Structure Of    */
/*                    That Record.                                           */
/*  Input(s)        : none                                                   */
/*  Output(s)       : pool index                                             */
/*  Returns         : u4_PoolId / TMO_NOT_OK                                 */
/*****************************************************************************/
PRIVATE INT4
MemGetFreePoolId (void)
{
    UINT4               u4_PoolId;

    OsixSemTake (MemMutex);

    for (u4_PoolId = 0; u4_PoolId < (gtMemPoolCfg.u4MaxMemPools); u4_PoolId++)
        if (!pMemFreePoolRecList[u4_PoolId].u4UnitsCount)
        {
            pMemFreePoolRecList[u4_PoolId].u4UnitsCount = 1;
            OsixSemGive (MemMutex);
            return u4_PoolId;
        }

    OsixSemGive (MemMutex);
    return (INT4) (MEM_FAILURE);
}

/*****************************************************************************/
/*  Function Name   : MemPoolInitializeFreePoolList                          */
/*  Description     : This Procedure Initialises The Elements In             */
/*                    pMemPoolFreePoolRecList Array To NULL                  */
/*  Input(s)        : none                                                   */
/*  Output(s)       : Initialized pMemPoolFreePoolRecList                    */
/*  Returns         : MEM_FAILURE / MEM_SUCCESS                              */
/*****************************************************************************/
PRIVATE UINT4
MemPoolInitializeFreePoolList (void)
{
    UINT4               u4PoolId;

    pMemFreePoolRecList = POOL_MEM_CALLOC (sizeof (tMemFreePoolRec),
                                      (gtMemPoolCfg.u4MaxMemPools),
                                      tMemFreePoolRec);
    if (!pMemFreePoolRecList)
        return MEM_FAILURE;

    for (u4PoolId = 0; u4PoolId < (gtMemPoolCfg.u4MaxMemPools); u4PoolId++)
    {
        pMemFreePoolRecList[u4PoolId].u4Size = 0;
        pMemFreePoolRecList[u4PoolId].u4UnitsCount = 0;
        pMemFreePoolRecList[u4PoolId].u4FreeUnitsCount = 0;
#if DEBUG_MEM == FSAP_ON
        pMemFreePoolRecList[u4PoolId].u4AllocCount = 0;
        pMemFreePoolRecList[u4PoolId].u4ReleaseCount = 0;
        pMemFreePoolRecList[u4PoolId].u4AllocFailCount = 0;
        pMemFreePoolRecList[u4PoolId].u4HeapAllocFailCount = 0;
        pMemFreePoolRecList[u4PoolId].u4ReleaseFailCount = 0;
        pMemFreePoolRecList[u4PoolId].u4PeakUsageCount = 0;
#endif
        pMemFreePoolRecList[u4PoolId].pu1StartAddr = NULL;
        pMemFreePoolRecList[u4PoolId].pu1EndAddr = NULL;

        pMemFreePoolRecList[u4PoolId].BufList.pu1Base = NULL;
        pMemFreePoolRecList[u4PoolId].BufList.pu1Head = NULL;

        pMemFreePoolRecList[u4PoolId].u4MemType = MEM_DEFAULT_MEMORY_TYPE;

    }
    return MEM_SUCCESS;
}

/*****************************************************************************/
/*  Function Name   : MemIsValidMemoryType                                   */
/*  Description     : Memory type validation routine.                        */
/*                  : Unused in this version.                                */
/*  Input(s)        : u4Type - Memory type.                                  */
/*  Output(s)       : -                                                      */
/*  Returns         : MEM_FAILURE / MEM_SUCCESS                              */
/*****************************************************************************/
UINT4
MemIsValidMemoryType (UINT4 u4Type)
{
    /* Unused Param. */
    u4Type = u4Type;
    return 0;
}

/*****************************************************************************/
/*  Function Name   : MemGetFreeUnits                                        */
/*  Description     : Returns count of available blocks in a given pool.     */
/*  Input(s)        : u4QueID - The poolId.                                  */
/*  Output(s)       : None.                                                  */
/*  Returns         : The free units.                                        */
/*****************************************************************************/
UINT4
MemGetFreeUnits (UINT4 u4QueID)
{
    return (pMemFreePoolRecList[(u4QueID - 1)].u4FreeUnitsCount);
}

/*****************************************************************************/
/*  Function Name   : MemGetTotalUnits                                       */
/*  Description     : Returns Total number of blocks in a given pool.        */
/*  Input(s)        : u4QueID - The poolId.                                  */
/*  Output(s)       : None.                                                  */
/*  Returns         : The Total units.                                       */
/*****************************************************************************/
UINT4
MemGetTotalUnits (UINT4 u4QueID)
{    
	return (pMemFreePoolRecList[(u4QueID - 1)].u4UnitsCount);
}


/*****************************************************************************/
/*  Function Name   : MemSetDbg                                              */
/*  Description     : Function to change current debug level.                */
/*  Input(s)        : u4Val - The new debug level.                           */
/*  Output(s)       :                                                        */
/*  Returns         : None.                                                  */
/*****************************************************************************/
#if DEBUG_MEM == FSAP_ON
VOID
MemSetDbg (UINT4 u4Val)
{
    MEM_DBG_FLAG = u4Val;
}
#endif

/*****************************************************************************/
/*  Function Name   : MemIsValidBlock                                        */
/*  Description     : To test if a block of mem belongs to a specific pool.  */
/*                    Some protocols have the habit of allocating from the   */
/*                    heap if MemAllocateMemBlock fails.                     */
/*                    Such apps, should call this function, b4 calling       */
/*                    MemFreeMemBlock or free.                               */
/*  Input(s)        : PoolId   - Pool to which the block might belong.       */
/*                    pu1Block - Block of memory                             */
/*  Output(s)       : None.                                                  */
/*  Returns         : MEM_SUCCESS/MEM_FAILURE/MEM_OK_BUT_NOT_ALIGNED         */
/*****************************************************************************/
PRIVATE UINT4
MemIsValidBlock (tMemPoolId PoolId, UINT1 *pu1Block)
{
    tMemFreePoolRec    *pPoolRecPtr;
    UINT4               u4BlockSize;
    UINT2               u2QueId;
    UINT1              *pu1Start;
    UINT1              *pu1End;

    if ((PoolId == 0) || (PoolId > gtMemPoolCfg.u4MaxMemPools))
        return MEM_FAILURE;

    if (!pu1Block)
        return MEM_FAILURE;

    u2QueId = (UINT2) (PoolId - 1);
    pPoolRecPtr = &pMemFreePoolRecList[u2QueId];
    pu1Start = pPoolRecPtr->pu1StartAddr;
    pu1End = pPoolRecPtr->pu1EndAddr;

    u4BlockSize = pPoolRecPtr->u4Size;
#if DEBUG_MEM == FSAP_ON
    u4BlockSize += ((sizeof (tMemDebugInfo) + MEM_ALIGN_BYTE) & MEM_ALIGN);
#endif
    /*
     * It is valid if the block falls on a "block-boundary" within this
     * memPool.
     * Else if it is merely not aligned, we try to be good
     * and indicate it through a suitable error code aptly numbered.
     */
    if (pu1Block >= pu1Start && (pu1Block < pu1End))
    {
        if ((pu1Block - pu1Start) % u4BlockSize == 0)
            return MEM_SUCCESS;
        else
            return MEM_OK_BUT_NOT_ALIGNED;
    }
    else
        return MEM_FAILURE;

}

/*CAMEOTAG: Added By Aiyong, 2016-12-10, To judge whether CRU pool over limit*/
/************************************************************************************** 
 * FUNCTION NAME: CmMemIsCruPoolOverLimit
 *
 * DESCRIPTION : This function is used to judge whether CRU pool over limit
 *
 * INPUTS      : u1Percent - Percentage (input 80 represet 80%)
 *
 * OUTPUTS     : None.
 *
 * RETURNS     : TRUE/FALSE.
 *
 ***************************************************************************************/
#if 0
BOOL1
CmMemIsCruPoolOverLimit (UINT1 u1Percent)
{
    UINT2               u2ArrIndex = 0, 
                        u2PoolId = 0;
    tMemFreePoolRec    *pPoolRecPtr = NULL;;
    
    UINT2               u2ArrPoolId[] = {
        pCRU_BUF_Chain_FreeQueDesc->u2_QueId,
        pCRU_BUF_DataDesc_FreeQueDesc->u2_QueId,
        pCRU_BUF_DataBlk_FreeQueDesc[0].u2_QueId
    };

    /* Percentage can't more than 100%, if input more than 100, just return false, 
     * means that pool usage can't move than 100%
     */
    if(u1Percent/100 >= 1)
        return FALSE;
    
    for (u2ArrIndex = 0; u2ArrIndex < (sizeof(u2ArrPoolId)/sizeof(UINT2)); ++u2ArrIndex)
    {

        u2PoolId = u2ArrPoolId[u2ArrIndex] - 1;

        MEM_ENTER_CS (u2PoolId);
        pPoolRecPtr = &pMemFreePoolRecList[u2PoolId];
        if(pPoolRecPtr == NULL)
        {
            MEM_LEAVE_CS (u2PoolId);
            /*PRINTF ("%s,%d, MemPool Id:%d pMemFreePoolRecList is NULL.", 
                    __FUNCTION__, __LINE__, u2ArrPoolId[u2ArrIndex]);*/
            return FALSE;
        }
        
        /*
         * Warn if memPool is draining out.
         * Only (1 - u1Percent/100) Pool size are remained.
         */
        if (1.0 * pPoolRecPtr->u4FreeUnitsCount / pPoolRecPtr->u4UnitsCount
            < (1.0 - 1.0*u1Percent/100))
        {
            MEM_LEAVE_CS (u2PoolId);
            /*PRINTF("%s,%d, MemPool Id:%d, Free:%d, Units:%d.\n", __FUNCTION__, __LINE__, 
                u2ArrPoolId[u2ArrIndex], pPoolRecPtr->u4FreeUnitsCount, pPoolRecPtr->u4UnitsCount);*/

            return TRUE;
        }

        MEM_LEAVE_CS (u2PoolId);
    }

    return FALSE;
}
#endif

