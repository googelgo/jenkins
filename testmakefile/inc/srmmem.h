/*
 * Copyright (C) 2006 Aricent Inc . All Rights Reserved
 *
 * $Id: srmmem.h,v 1.6 2012/07/31 09:31:10 siva Exp $
 *
 * Description: SRM Mem module's exported file.
 *
 */

#ifndef SRM_MEM_H
#define SRM_MEM_H

VOID
DebugShowMemPoolDetails(VOID);

#define  MEM_FREE_POOL_UNIT_COUNT(_Id)      MemGetFreeUnits(_Id)

/************************************************************************
*                                                                       *
*                      Defines and Typedefs                             *
*                                                                       *
*************************************************************************/
/* Error codes */
#define  MEM_SUCCESS                    0
#define  MEM_OK_BUT_NOT_ALIGNED         1
#define  MEM_FAILURE               (UINT4) (~0UL)

/* Debug Levels to be used in MemSetDbgLevel */
#define  MEM_DBG_MINOR                0x1
#define  MEM_DBG_MAJOR                0x2
#define  MEM_DBG_CRITICAL             0x4
#define  MEM_DBG_FATAL                0x8
#define  MEM_DBG_ALWAYS              0x10

/* Bit map representation of MEM TYPE */
#define MEM_DEFAULT_MEMORY_TYPE      0x00
#define MEM_HEAP_MEMORY_TYPE         0x01

#define  INVALID_POOL_ID                0

#if DEBUG_MEM == FSAP_ON
/* this macro returns the variable name ,that is passed , as a string */
#define VAR_STR(x) #x
/* CAMEOTAG: delete by linyu on 2014-01-22, func and line number decide the location */
#define MemCreateMemPool(size,u4NumNodes,MemType,PoolId) \
        MemCreateMemPoolDbg(size,u4NumNodes,MemType,PoolId,/*__FILE__,*/\
                            __FUNCTION__,__LINE__/*,VAR_STR(PoolId)*/)
#endif

typedef struct MemChunkCfg
{
   UINT4 u4StartAddr;      /* StartAddr of  chunk */
   UINT4 u4Length;         /* Length of each chunk */
} tMemChunkCfg;

typedef struct MemTypeCfg
{
        UINT4 u4MemoryType;
        /* Type of memory, say, DRAM, SRAM, etc */

        UINT4 u4NumberOfChunks;
        /* Number of non-contiguous chunks present in this memory type  */

        tMemChunkCfg MemChunkCfg[1];
        /* Memory Chunk details */

} tMemTypeCfg;

typedef struct MemPoolCfg
{
        UINT4 u4MaxMemPools;
        /* Max number of memory pools in the system */

        UINT4 u4NumberOfMemTypes;
        /* Number of memory types present in the system */

        tMemTypeCfg MemTypes [1];
        /* Memory type details */

} tMemPoolCfg;

typedef UINT4 tMemPoolId;

/************************************************************************
*                                                                       *
*                      Function Prototypes                              *
*                                                                       *
*************************************************************************/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

UINT4
MemInitMemPool (tMemPoolCfg *pMemPoolCfg);

/* CAMEOTAG: add by linyu on 2014-04-18, 
 * support basic statistic for memory pool, include id, total/free units 
 */
UINT4
MemPrintMemPoolStatistics (tMemPoolId PoolId);
UINT4
MemPrintMemPoolContent (tMemPoolId PoolId);

#if DEBUG_MEM == FSAP_ON
/* CAMEOTAG: delete by linyu on 2014-01-22, func and line number decide the location */
UINT4
MemCreateMemPoolDbg (UINT4 u4BlockSize, UINT4 u4NumberOfBlocks,
                     UINT4 u4TypeOfMemory, tMemPoolId *pPoolId,
                     /*const CHR1 *pu1FilePath,*/ const CHR1 *pu1FuncName,
                     UINT4 u4LiineNo/*,const CHR1 *pu1Variable*/);

#define MemAllocateMemBlock(Id,pp) MemAllocateMemBlockLeak(Id, pp, /*__FILE__,*/__LINE__,__PRETTY_FUNCTION__)

#define MemAllocMemBlk(Id) MemAllocMemBlkLeak(Id,/*__FILE__,*/__LINE__,__PRETTY_FUNCTION__)

UINT4
MemAllocateMemBlockLeak (tMemPoolId PoolId, UINT1 **ppu1Block, /*const CHR1 *,*/ UINT4, const CHR1 *);

VOID *
MemAllocMemBlkLeak (tMemPoolId PoolId, /*const CHR1 *,*/ UINT4, const CHR1 *);

VOID
MemLeak (tMemPoolId PoolId);

UINT4
MemReleaseMemBlockDbg (tMemPoolId PoolId, UINT1 *pu1Block, 
                    const CHR1 * pFunc, UINT4 u4LineNo);

#define MemReleaseMemBlock(Id, pBlock) \
	MemReleaseMemBlockDbg(Id, pBlock, __PRETTY_FUNCTION__, __LINE__)
#else
UINT4
MemAllocateMemBlock (tMemPoolId PoolId, UINT1 **ppu1Block);

VOID * MemAllocMemBlk (tMemPoolId PoolId);

UINT4
MemCreateMemPool (UINT4 u4BlockSize, UINT4 u4NumberOfBlocks,
                  UINT4 u4TypeOfMemory, tMemPoolId *pPoolId);

UINT4
MemReleaseMemBlock (tMemPoolId PoolId, UINT1 *pu1Block);
#endif

UINT4
MemDeleteMemPool (tMemPoolId PoolId);

UINT4
MemGetFreeUnits (UINT4 u4QueID);

UINT4
MemGetTotalUnits (UINT4 u4QueID);


UINT4
MemShutDownMemPool (void);

VOID
MemSetDbg(UINT4 u4Val);

/*CAMEOTAG: Added By Aiyong, 2016-12-10, to judge whether CRU pool over limit */
BOOL1 CmMemIsCruPoolOverLimit (UINT1 u1Percent);
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
