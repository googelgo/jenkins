/*wdmain.c watch dog entry */
#ifndef TASKA_C 
#define TASKA_C 

#include "macro.h"
#include "proto.h"
#include "osxinc.h"
#include "osix.h"

tOsixQId gTestQId = 0;
tOsixQId gPoolQId = 0;

static tOsixTaskId gTaskAId = 0;
tMemPoolId      gQMsgPoolId  =0;         /*mempool id for Message */

#define TASKA_EVENTA 0x1
#define TASKA_EVENT_POOL 0x2

VOID TestQueMsgHandle()
{
    CHR1 *pchStrTmp = NULL;

    DEBUG();
    
	/*OSIX_NO_WAIT: when have no meesage in queue ,will return OSIX_FAILURE; if exist message, will return OSIX_SUCCESS 
	  OSIX_WAIT: when have no meesage in queue ,will wait (condition wait) until have new messages
	 */
    while(OsixQueRecv(gTestQId, (UINT1*) &pchStrTmp, OSIX_DEF_MSG_LEN, OSIX_NO_WAIT) == OSIX_SUCCESS)
    {
        DEBUG();
        PRINTF("[%s][%d] Recv Str:%s, Str Point:%p.\n", __FUNCTION__, __LINE__, pchStrTmp, pchStrTmp);
    }

}

VOID TestQueMsgFromPoolHandle()
{
    CHR1 *pQMsg = NULL;

    DEBUG();
    
	/*OSIX_NO_WAIT: when have no meesage in queue ,will return OSIX_FAILURE; if exist message, will return OSIX_SUCCESS 
	  OSIX_WAIT: when have no meesage in queue ,will wait (condition wait) until have new messages
	 */	
    while(OsixQueRecv(gPoolQId, (UINT1*) &pQMsg, OSIX_DEF_MSG_LEN, OSIX_NO_WAIT) == OSIX_SUCCESS)
    {
        DEBUG();
        PRINTF("[%s][%d] Recv Str:%s, Str Point:%p.\n", __FUNCTION__, __LINE__, pQMsg, pQMsg);
		
	
        /* Release the buffer to pool */
        if (MemReleaseMemBlock (gQMsgPoolId, (UINT1 *) pQMsg)
            					!= MEM_SUCCESS)
        {
            PRINTF("TestQueMsgFromPoolHandle"
                      "Free MemBlock AppMsg FAILED!!!\r\n");
            return;
        }
    }
}

/*****************************************************************************
 *
 *    Function Name         : TaskAMain
 *
 *    Description           : TaskA task function
 *
 *    Input(s)              : None
 *
 *    Output(s)             : None.
 *
 *    Returns               : None
 *
 *****************************************************************************/
VOID TaskAMain(VOID)
{
    UINT4 u4Event = 0;

    TaskAInit();

    /* Indicate the status of initialization to the main routine */
    lrInitComplete (OSIX_SUCCESS);

    while(1)
    {
        DEBUG();
        if(OsixEvtRecvV1(gTaskAId, TASKA_EVENTA | TASKA_EVENT_POOL, OSIX_WAIT, &u4Event) == OSIX_SUCCESS)
        {
            DEBUG();
			PRINTF("Receive TASKA_EVENT:%d. \n", u4Event);
            if (u4Event & TASKA_EVENTA)
            {
                DEBUG();
                PRINTF("Receive TASKA_EVENTA \n");
                TestQueMsgHandle();
            }
			
			DEBUG();
            if (u4Event & TASKA_EVENT_POOL)
            {
                DEBUG();
                PRINTF("Receive TASKA_EVENT_POOL \n");
                TestQueMsgFromPoolHandle();
            }
        }

    }
}


/* Task Init */
INT4 TaskAInit(VOID)
{
    OsixTskIdSelf(&gTaskAId);
    DEBUG();
    PRINTF("TaskAInit gTaskAId:%d \n", gTaskAId);
    if(OsixQueCrt("TESTQ",/*queue name*/ 4 /*msg len*/, 100,/*max msgs*/ &gTestQId) == OSIX_FAILURE)
        DEBUG();

	if(OsixQueCrt("POOLQ",/*queue name*/ 4 /*msg len*/, 100,/*max msgs*/ &gPoolQId) == OSIX_FAILURE)
        DEBUG();
	
	/* Create mem pool for Message */
    if (MemCreateMemPool (100/*Block size*/, 10/*number of blocks*/, MEM_DEFAULT_MEMORY_TYPE, 
                          &(gQMsgPoolId)) == MEM_FAILURE)
    {
        /* There is not enough memory for the protocol registration entry. */
        PRINTF("Fatal Error in TaskAInit - There is no memory for LBD Message.\n");
        return OSIX_FAILURE;
    }
	
	return OSIX_SUCCESS;
}

/* Send event to task */
INT4 Event2TaskASend(VOID)
{
	/* A. Send message through string point */
	CHR1 *pchStr = "Test Message!";
	
    DEBUG();
    OsixQueSend(gTestQId, (UINT1*)&pchStr, 4 /*msg len*/);
    PRINTF("[%s][%d], str:%s, Str point:%p(Double point:%p).\n", __FUNCTION__, __LINE__, pchStr, pchStr, (UINT1*)&pchStr);
	
    if (OsixEvtSendV1(gTaskAId, TASKA_EVENTA) != OSIX_SUCCESS)
    {
        DEBUG();
        return OSIX_FAILURE;
    }

	DEBUG();
	
	/* B. Send message through memroy pool */
	CHR1      *pMesg = NULL;

    if ((pMesg = (CHR1 *) MemAllocMemBlk (gQMsgPoolId))
        == NULL)
    {
		DEBUG();
        PRINTF("Event2TaskASend"
                  "Alloc MemBlock FAILED!!!\r\n");
        return OSIX_FAILURE;
    }
	MEMCPY(pMesg, pchStr, STRLEN(pchStr));
	
    if (OsixQueSend (gPoolQId, (UINT1 *) &pMesg,
                            OSIX_DEF_MSG_LEN) != OSIX_SUCCESS)
    {
		DEBUG();
		MemReleaseMemBlock (gQMsgPoolId, (UINT1 *) pMesg);
        return OSIX_FAILURE;
    }

	if (OsixEvtSendV1 (gTaskAId, TASKA_EVENT_POOL)
		!= OSIX_SUCCESS)
	{
		DEBUG();
		PRINTF("Event2TaskASend"
				  "Osix Event send Failed!!!\r\n");
	}
	
    DEBUG();
    return OSIX_SUCCESS;
}


#endif  /* ARCH_WATCHDOG_TIMER_C */
