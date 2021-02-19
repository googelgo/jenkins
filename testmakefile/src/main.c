/*wdmain.c watch dog entry */
#ifndef ARCH_WATCHDOG_TIMER_C
#define ARCH_WATCHDOG_TIMER_C

#include "macro.h"
#include "proto.h"
#include "osxinc.h"
#include "osix.h"

typedef enum TaskStatus
{
    TSK_BEFORE_INIT = 0,
    TSK_INIT_SUCCESS,
    TSK_INIT_FAILURE
}
tTskStatus;

typedef struct TskInitTable
{
    char               *pu1TskName;
    UINT4               u4TskPriority;
    UINT4               u4SchedPolicy;
    UINT4               u4TskStack;
    VOID                (*pTskEntryFn) (INT1 *);
    VOID                (*pTskShutDnFn) (VOID);
}
tTskInitTable;

tTskInitTable       gaInitTable[] = {
    {"TASKA", 10, OSIX_SCHED_RR, OSIX_DEFAULT_STACK_SIZE, (VOID *)TaskAMain, NULL},
    {NULL, 0, 0, 0, NULL, NULL}
};

#define ENTRY_FN(gu4CurIndex)     (gaInitTable[gu4CurIndex].pTskEntryFn)
#define SHUTDN_FN(gu4CurIndex)    (gaInitTable[gu4CurIndex].pTskShutDnFn)

static UINT4        gu4CurIndex = 0;
tOsixSemId          gSeqSemId;
static tTskStatus   gCurTskStatus = TSK_BEFORE_INIT;
static tMemPoolCfg  LrMemPoolCfg;


VOID
lrInitComplete (UINT4 u4Status)
{
    /* Depending on the status returned, set the Task Init Status
     * for the current task.
     */
    if (u4Status == OSIX_SUCCESS)
    {
		DEBUG();
        gCurTskStatus = TSK_INIT_SUCCESS;
    }
    else
    {
		DEBUG();
        gCurTskStatus = TSK_INIT_FAILURE;
    }

    /* Relinquish the "SEQ" semaphore and return */
    if (OsixSemGive (gSeqSemId) != OSIX_SUCCESS)
    {
		DEBUG();
        return;
    }
	DEBUG();
}

/*****************************************************************************
 *
 *    Function Name        : main
 *
 *    Description        : main task function
 *
 *    Input(s)            :None
 *
 *    Output(s)            : None.
 *
 *    Returns            : None
 *
 *****************************************************************************/
int main()
{
    INT4                i4RetVal = 0;
    tOsixTaskId         TskId = 0;
	INT4                (*pModEntFn) (INT1 *) = NULL;
    INT1                i1Param = 0;
	
    printf("main start v2:%d \n", SUCCESS);
    UtilityTest();
    OsixInitialize ();

    DEBUG();

    /* The "SEQ" semaphore is used to ensure that each protocol / module
     * is initialized before the next protocol / module initialization 
     * is invoked.
     */
    if (OsixCreateSem ((const UINT1 *) "SEQ", 0, 0, &gSeqSemId) != OSIX_SUCCESS)
    {
        return (1);
    }
	DEBUG();
	
	/* Initialize MemPool manager */
	LrMemPoolCfg.u4MaxMemPools = 100;
    LrMemPoolCfg.u4NumberOfMemTypes = 0;
    LrMemPoolCfg.MemTypes[0].u4MemoryType = MEM_DEFAULT_MEMORY_TYPE;
    LrMemPoolCfg.MemTypes[0].u4NumberOfChunks = 0;
    LrMemPoolCfg.MemTypes[0].MemChunkCfg[0].u4StartAddr = 0;
    LrMemPoolCfg.MemTypes[0].MemChunkCfg[0].u4Length = 0;

    if (MemInitMemPool (&LrMemPoolCfg) != MEM_SUCCESS)
    {
        PRINTF ("MemInitMemPool Fails !!!!!!!!!!!!!!!!!!!!!!");
        return (1);
    }	
	
    while (ENTRY_FN (gu4CurIndex) != NULL)
    {
		gCurTskStatus = TSK_BEFORE_INIT;
		
        /* When a module is to be initialized, the Task Name for the
         * module will be set to NULL. This is to ensure that the 
         * module initialization function is invoked directly.
         */
        if (gaInitTable[gu4CurIndex].pu1TskName == NULL)
        {
            pModEntFn = (INT4 (*)(INT1 *)) (ENTRY_FN (gu4CurIndex));
            pModEntFn (&i1Param);
            /* Module reports failure by lrInitComplete
               Since, every module has different return type */
            i4RetVal = OSIX_SUCCESS;
        }
        else
		{
			DEBUG();
			i4RetVal =
				(INT4) OsixTskCrt ((UINT1 *) gaInitTable[gu4CurIndex].pu1TskName,
						(gaInitTable[gu4CurIndex].u4TskPriority |
						 gaInitTable[gu4CurIndex].u4SchedPolicy),
						gaInitTable[gu4CurIndex].u4TskStack,
						(OsixTskEntry) ENTRY_FN (gu4CurIndex),
						0, &TskId);
			DEBUG();			
		}
		
		DEBUG();
		
        if (i4RetVal == OSIX_FAILURE)
        {
            PRINTF ("ERROR: %s - Task failed, exiting!!\n",
                    gaInitTable[gu4CurIndex].pu1TskName);
            break;
        }
		
        /* After the module init has been invoked or protocol task has been
         * spawned, wait on the "SEQ" semaphore. When the semaphore is given
         * back by the protocol, check if the initialization was successful.
         */
		DEBUG(); 
        if ((OsixSemTake (gSeqSemId) != OSIX_SUCCESS) ||
            (gCurTskStatus != TSK_INIT_SUCCESS))
        {
			DEBUG();
            PRINTF ("Task Init Failed for <%s> \n",
                    gaInitTable[gu4CurIndex].pu1TskName);
            i4RetVal = OSIX_FAILURE;
            break;
        }

        printf("main start task:%s \n", gaInitTable[gu4CurIndex].pu1TskName);
        
		gu4CurIndex++;
    }

    Event2TaskASend();

    DEBUG();
	
	/* Run main process always */
	Fsap2Start ();
}


#endif  /* ARCH_WATCHDOG_TIMER_C */
