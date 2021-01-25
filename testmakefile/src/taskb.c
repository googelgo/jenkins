#ifndef TASKB_C 
#define TASKB_C 

#include "macro.h"
#include "proto.h"
#include "osxinc.h"
#include "osix.h"

static tOsixTaskId gTaskBId = 0;
#define TASKB_EVENTA 		  0x1

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
	
	TaskBInit();
	
    /* Indicate the status of initialization to the main routine */
    lrInitComplete (OSIX_SUCCESS);
	
    while(1)
    {
		DEBUG();
       if(OsixEvtRecv(gTaskBId, TASKB_EVENTA, OSIX_WAIT, &u4Event) == OSIX_SUCCESS)
       {
			DEBUG();
            if (u4Event & TASKB_EVENTA)
            {
				DEBUG();
                PRINTF("Receive TASKB_EVENTA \n");
            }
       }

    }
}

/* Task Init */
INT4 TaskBInit(VOID)
{
    OsixTskIdSelf(&gTaskBId);
	DEBUG();
	PRINTF("TaskBInit gTaskBId:%d \n", gTaskBId);
}

/* Send event to task */
INT4 Event2TaskBSend(VOID)
{
	DEBUG();
	if (OsixEvtSend(gTaskBId, TASKB_EVENTA) != OSIX_SUCCESS)
	{
		DEBUG();
		return OSIX_FAILURE;
	}
	
	DEBUG();
	return OSIX_SUCCESS;
}

#endif  /* TASKB_C */
